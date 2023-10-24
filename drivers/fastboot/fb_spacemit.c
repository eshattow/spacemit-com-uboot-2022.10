// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023,  chris.huang<chris.huang@spacemit.com>
 */

#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <config.h>
#include <fastboot.h>
#include <malloc.h>
#include <common.h>
#include <fastboot-internal.h>
#include <image-sparse.h>
#include <image.h>
#include <fdt_support.h>
#include <part.h>
#include <mmc.h>
#include <div64.h>
#include <fb_spacemit.h>
#include <mapmem.h>
#include <memalign.h>
#include <u-boot/crc.h>
#include <dm.h>
#include <dm/uclass-internal.h>

#define EMMC_MAX_BLK_WRITE 16384

/**
 * fastboot_oem_flash_gpt() - parse flash_config and write gpt table.
 *
 * @cmd: Named partition to write image to
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 * @response: Pointer to fastboot response buffer
 */
void fastboot_oem_flash_gpt(const char *cmd, void *download_buffer, u32 download_bytes,
							char *response, u32 *start_offset)
{
	int  nodeoffset;	/* node offset from libfdt */
	int  nextoffset;	/* next node offset from libfdt */
	int len = 0;	/* new length of the property */
	uint32_t tag;
	u32 part_index = 0;
	u32 get_size = 0;

	/*limit partition to 10, so that the gpt_table would no more than 256 byte*/
	char gpt_table[256] = {"\0"};
	char gpt_command[300] = {"\0"};
	char fsbl_offset_start[36] = {"\0"} ;
	u32 reserve_part_size = 0;
	char strnum[10] = {"\0"};
	char *token;


	struct fdt_header *blob = (struct fdt_header *)fastboot_buf_addr;
	if (fdt_check_header(blob) || !fdt_valid(&blob)){
		printf("not a valid fdt, need download flash_config at first\n");
		return;
	}

	nodeoffset = fdt_path_offset (blob, "/");
	fdt_next_tag(blob, nodeoffset, &nextoffset);
	nodeoffset = nextoffset;
	bool find_next_tag = true;
	while (find_next_tag) {
		tag = fdt_next_tag(blob, nodeoffset, &nextoffset);
		switch (tag) {
		case FDT_BEGIN_NODE:
			const char *node_name = fdt_get_name(blob, nodeoffset, NULL);
			if (node_name[0] == 'g' || node_name[0] == 'r'){
				const char *reserve_size = fdt_getprop(blob, nodeoffset, "size", &len);
				const char *hidden = fdt_getprop(blob, nodeoffset, "hidden", &len);

				if(node_name[0] == 'g' || hidden[0] == 't'){
					char *ret = strpbrk(reserve_size, "KM");
					if (ret[0] == 'K'){
						strcpy(strnum, reserve_size);
						token = strtok(strnum, "K");
						get_size = simple_strtoul(token, NULL, 0);
					}else if(ret[0] == 'M'){
						strcpy(strnum, reserve_size);
						token = strtok(strnum, "M");
						get_size = simple_strtoul(token, NULL, 0);
						get_size = get_size * 1024;
					}else{
						pr_err("not support size %s, should use KiB/MiB\n", reserve_size);
						fastboot_fail("not support size, should use KiB/MiB", response);
						return;
					}
					reserve_part_size += simple_strtoul(token, NULL, 0);
					if (node_name[0] == 'g')/*if hidden, it need to save the start offset*/
						*start_offset = (reserve_part_size * 1024);

					debug("reserve_part_size:%d, *start_offset:%d\n", reserve_part_size, *start_offset);

					break;
				}else{
					debug("not hidden part\n");
				}

			}else if (node_name[0] == 'm'){
				debug("parse %s config, which should not be a gpt partion\n", node_name);
				break;
			}
			const char *node_part = fdt_getprop(blob, nodeoffset, "partition", &len);
			const char *node_size = fdt_getprop(blob, nodeoffset, "size", &len);
			if (part_index == 0){
				sprintf(fsbl_offset_start, "%dKiB", reserve_part_size);
				sprintf(gpt_table, "name=%s,start=%s,size=%s;", node_part, fsbl_offset_start, node_size);
			}
			else{
				sprintf(gpt_table, "%sname=%s,size=%s;", gpt_table, node_part, node_size);
			}

			part_index++;
			break;
		case FDT_END:
			find_next_tag = false;
			break;
		}
		nodeoffset = nextoffset;
	}

	if (strlen(gpt_table) == 0) {
		fastboot_fail("miss gpt table parameter", response);
	} else {
		sprintf(gpt_command, "gpt write mmc %x '%s'",
			CONFIG_FASTBOOT_FLASH_MMC_DEV, gpt_table);
		printf("cmd:%s\n", gpt_command);
		if (run_command(gpt_command, 0))
			fastboot_fail("write gpt fail", response);
		else
			fastboot_okay(gpt_command, response);
	}
	return;
}

/**
 * fb_mmc_blk_write() - Write/erase MMC in chunks of EMMC_MAX_BLK_WRITE
 *
 * @block_dev: Pointer to block device
 * @start: First block to write/erase
 * @blkcnt: Count of blocks
 * @buffer: Pointer to data buffer for write or NULL for erase
 */
static lbaint_t fb_mmc_blk_write(struct blk_desc *block_dev, lbaint_t start,
				 lbaint_t blkcnt, const void *buffer)
{
	lbaint_t blk = start;
	lbaint_t blks_written;
	lbaint_t cur_blkcnt;
	lbaint_t blks = 0;
	int i;

	for (i = 0; i < blkcnt; i += EMMC_MAX_BLK_WRITE) {
		cur_blkcnt = min((int)blkcnt - i, EMMC_MAX_BLK_WRITE);
		if (buffer) {
			if (fastboot_progress_callback)
				fastboot_progress_callback("writing");
			blks_written = blk_dwrite(block_dev, blk, cur_blkcnt,
						  buffer + (i * block_dev->blksz));
		} else {
			if (fastboot_progress_callback)
				fastboot_progress_callback("erasing");
			blks_written = blk_derase(block_dev, blk, cur_blkcnt);
		}
		blk += blks_written;
		blks += blks_written;
	}
	return blks;
}

/**
 * fastboot_mmc_flash_fsbl() - Write fsbl image to eMMC
 *
 * @start_offset: start offset to write.
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 */
int fastboot_mmc_flash_fsbl(u32 start_offset, void *download_buffer,
                             u32 download_bytes)
{
	struct blk_desc *dev_desc;
	struct disk_partition info = {0};
	lbaint_t blkcnt;
	u32 offset = start_offset;
	lbaint_t blks;

	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc){
		return -1;
	}
	part_get_info(dev_desc, 1, &info);
	info.blksz = dev_desc->blksz;
	if(info.blksz == 0)
		return -1;
	if (!download_bytes){
		printf("it should run command 'fastboot stage fsbl.bin' before run flash fsbl\n");
		return -1;
	}

	info.start = offset / info.blksz;
	/* determine number of blocks to write */
	blkcnt = ((download_bytes + (info.blksz - 1)) & ~(info.blksz - 1));
	blkcnt = lldiv(blkcnt, info.blksz);

	blks = fb_mmc_blk_write(dev_desc, info.start, blkcnt, download_buffer);

	if (blks != blkcnt) {
			pr_err("failed writing to device %d\n", dev_desc->devnum);
			return -1;
	}

	printf("........ wrote " LBAFU " bytes to 'fsbl'\n", blkcnt * info.blksz);
	return 0;
}

int check_mmc_image_crc(struct blk_desc *dev_desc, ulong crc_compare, lbaint_t part_start_cnt,
			ulong blksz, int image_size)
{
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
	u32 div_times = (image_size + RECOVERY_LOAD_IMG_SIZE - 1) / RECOVERY_LOAD_IMG_SIZE;
	ulong crc = 0;
	int byte_remain = image_size;
	int download_bytes = 0;
	u32 blk_size, n;
	unsigned long time_start_flash = get_timer(0);

	/*if crc_compare is 0, return 0 directly*/
	if (!crc_compare)
		return 0;

	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("invalid mmc device\n");
		return -1;
	}

	for (int i = 0; i < div_times; i++) {
		debug("\ndownload and flash div %d\n", i);
		download_bytes = byte_remain > RECOVERY_LOAD_IMG_SIZE ? RECOVERY_LOAD_IMG_SIZE : byte_remain;

		blk_size = (download_bytes + (blksz - 1)) / blksz;
		n = blk_dread(dev_desc, part_start_cnt, blk_size, load_addr);
		if (n != blk_size) {
			printf("mmc read blk not equal it should be\n");
			return -1;
		}
		crc = crc32_wd(crc, (const uchar *)load_addr, download_bytes, CHUNKSZ_CRC32);
		part_start_cnt += blk_size;
		byte_remain -= download_bytes;
	}

	printf("get crc value:%lx, compare crc:%lx\n", crc, crc_compare);
	time_start_flash = get_timer(time_start_flash);
	printf("compare crc32 over, use time:%lu ms\n\n", time_start_flash);
	return (crc == crc_compare) ? 0 : -1;
}
