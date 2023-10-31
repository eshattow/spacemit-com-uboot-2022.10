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
#include <part.h>
#include <mmc.h>
#include <div64.h>
#include <fb_spacemit.h>
#include <mapmem.h>
#include <memalign.h>
#include <u-boot/crc.h>
#include <dm.h>
#include <dm/uclass-internal.h>
#include <cJSON.h>

#define EMMC_MAX_BLK_WRITE 16384

static __maybe_unused void _write_gpt_partition(char gpt_table[256], char *response)
{
	char gpt_command[300] = {"\0"};

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC)
		sprintf(gpt_command, "gpt write mmc %x '%s'",
			CONFIG_FASTBOOT_FLASH_MMC_DEV, gpt_table);
		printf("cmd:%s\n", gpt_command);
		if (run_command(gpt_command, 0)){
			fastboot_fail("write gpt fail", response);
			return;
		}
#elif CONFIG_IS_ENABLED(FASTBOOT_MTD_SUPPORT_BLK_DEV)
		printf("mtd write gpt to dev:%s\n", CONFIG_FASTBOOT_MTD_SUPPORT_BLK_DEV_NAME);
		sprintf(gpt_command, "gpt write %s %x '%s'",
			CONFIG_FASTBOOT_MTD_SUPPORT_BLK_DEV_NAME, 0,
			gpt_table);
		printf("cmd:%s\n", gpt_command);
		if (run_command(gpt_command, 0)){
			fastboot_fail("write gpt fail", response);
			return;
		}
#endif
	fastboot_okay("parse gpt/mtd table okay", response);
	return;
}

static __maybe_unused void _write_mtd_partitino(char mtd_table[128], char *response)
{
	printf("mtd tabel:%s\n", mtd_table);
	return;
}





/**
 * @brief transfer the string of size 'KiB' or 'MiB' to u32 type.
 * 
 * @param reserve_size , the string of size 'xiB'
 * @return int , return the transfer result.
 */
int transfer_string_to_ul(const char *reserve_size)
{
	char *ret, *token;
	char ch;
	char strnum[10] = {"\0"};
	u32 get_size = 0;

	if (reserve_size == NULL || strlen(reserve_size) == 0)
		return 0;

	printf("reserve_size:%s\n", reserve_size);
	ret = strpbrk(reserve_size, "KMG");
	ch = ret[0];
	if (ch == 'K' || ch == 'M' || ch == 'G'){
		strcpy(strnum, reserve_size);
		token = strtok(strnum, &ch);
		get_size = simple_strtoul(token, NULL, 0);
	}else{
		pr_err("not support size %s, should use KiB/MiB/GiB\n", reserve_size);
		return 0;
	}

	switch(ch){
	case 'K':
		return get_size;
	case 'M':
		return get_size * 1024;
	case 'G':
		return get_size * 1024 * 1024;
	}
	return 0;
}

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
	u32 part_index = 0;
	u32 get_size = 0;
	u32 reserve_part_size = 0;

	/*limit partition to 10, so that the gpt_table would no more than 256 byte*/
	char gpt_table[256] = {"\0"};
	char fsbl_offset_start[36] = {"\0"} ;
	char mtd_table[128] = {"\0"};

	cJSON *json_root;
	int array_size;

	json_root = cJSON_Parse((void *)fastboot_buf_addr);
	if (!json_root){
		printf("can not parse json, check your flash_config.cfg is json format or not\n");
		return;
	}

	array_size = cJSON_GetArraySize(json_root);
	for (int i = 0; i < array_size; i++){
		cJSON *item = cJSON_GetArrayItem(json_root, i);
		/*only matche the level 2*/
		if(item->type == cJSON_Object){

			const char *node_name = item->string;
			const char *reserve_size = NULL;
			const char *hidden = NULL;

			if (!strncmp(node_name, "gpt", 3) || !strncmp(node_name, "reserve", 7)){
				cJSON *cj_size = cJSON_GetObjectItem(item, "size");
				if (cj_size && cj_size->type == cJSON_String)
					reserve_size = cj_size->valuestring;
				else
					continue;

				cJSON *cj_hidden = cJSON_GetObjectItem(item, "hidden");
				if (cj_hidden && cj_hidden->type == cJSON_String)
					hidden = cj_hidden->valuestring;
				else
					hidden = "false";

				if(!strncmp(node_name, "gpt", 3) || hidden[0] == 't'){
					get_size = transfer_string_to_ul(reserve_size);
					reserve_part_size += get_size;
					/*if hidden, it need to save the start offset*/
					if (!strncmp(node_name, "gpt", 3))
						*start_offset = (reserve_part_size * 1024);

					debug("reserve_part_size:%d, *start_offset:%d\n", reserve_part_size, *start_offset);
					continue;
				}else{
					debug("not hidden part\n");
				}
			}

			const char *node_part = NULL;
			const char *node_size = NULL;
			cJSON *cj_partition = cJSON_GetObjectItem(item, "partition");
			if (cj_partition && cj_partition->type == cJSON_String)
				node_part = cj_partition->valuestring;
			cJSON *cj_size = cJSON_GetObjectItem(item, "size");
			if (cj_size && cj_size->type == cJSON_String)
				node_size = cj_size->valuestring;

			if (!strncmp(node_name, "mtd", 3)){
				printf("parse mtd config, %sn\n", node_name);
				sprintf(mtd_table, "%s%s(%s),", mtd_table, node_size, node_part);

			}else{
				if (part_index == 0){
					sprintf(fsbl_offset_start, "%dKiB", reserve_part_size);
					sprintf(gpt_table, "name=%s,start=%s,size=%s;", node_part, fsbl_offset_start, node_size);
				}
				else{
					sprintf(gpt_table, "%sname=%s,size=%s;", gpt_table, node_part, node_size);
				}
				part_index++;
			}
		}
	}

	if (strlen(gpt_table) > 0) {
		_write_gpt_partition(gpt_table, response);
		goto free_cjson;
	}
	if (strlen(mtd_table) > 0){
		printf("mtd table:%s\n", mtd_table);
		_write_mtd_partitino(mtd_table, response);
		goto free_cjson;
	}

	/*maybe there doesn't have gpt/mtd partition, should not return fail*/
	fastboot_okay("parse gpt/mtd table okay", response);
free_cjson:
	cJSON_free(json_root);
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
