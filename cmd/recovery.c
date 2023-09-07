// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023,  chris.huang<chris.huang@spacemit.com>
 */

#include <common.h>
#include <command.h>
#include <fdt_support.h>
#include <fs.h>
#include <malloc.h>
#include <mapmem.h>
#include <div64.h>
#include <part.h>
#include <blk.h>
#include <mmc.h>
#include <u-boot/crc.h>
#include <image.h>
#include <dm.h>
#include "recovery.h"

/*
	load file can not use malloc memory, use a temporary addr for
	loading file.if img file size more than 500MB, try to divide to
	multiple times to flash.
*/
#define LOAD_IMG_ADDR CONFIG_RECOVERY_LOAD_ADDR
#define LOAD_IMG_SIZE CONFIG_RECOVERY_LOAD_IMAGE_SIZE
#define MAX_BLK_WRITE (16384)
#define RESULT_OK (0)
#define RESULT_FAIL (1)

static int dev_emmc_num = -1;
static int dev_sdio_num = -1;

int check_mmc_exist(void){
	int mmc_dev_num = get_mmc_num();
	if (mmc_dev_num < 2)
		return RESULT_FAIL;

	for (int i = 0; i < mmc_dev_num; i++){
		struct mmc *mmc;
		mmc = find_mmc_device(i);
		if (!mmc){
			printf("no mmc device can find\n");
			return RESULT_FAIL;
		}
		if (mmc_init(mmc)){
			printf("mmc init fail\n");
			return RESULT_FAIL;
		}
		if (IS_SD(mmc)){
			dev_sdio_num = mmc_get_blk_desc(mmc)->devnum;
		}else{
			dev_emmc_num = mmc_get_blk_desc(mmc)->devnum;
		}

		if (dev_emmc_num != -1 && dev_sdio_num != -1){
			printf("emmc number:%d, sdio number:%d\n", dev_emmc_num, dev_sdio_num);
			return RESULT_OK;
		}
	}
	printf("it should has two mmc device to flash\n");
	return RESULT_FAIL;
}

void recovery_show_result(int ret){
	if (ret){
		printf("!!!!!!!!!!!!!!!!!!! recovery flash false !!!!!!!!!!!!!!!!!!!\n");
	}else{
		printf("################### recovery flash success ###################\n");
	}
	/*need to add while release*/
	// while(1){
	// 	/*do not retrun while flashing over!*/
	// }
}

static int get_part_info(struct blk_desc *dev_desc, const char *name,
			    struct disk_partition *info)
{
	int ret;

	if (dev_desc) {
		ret = part_get_info_by_name(dev_desc, name, info);
		if (ret >= 0)
			return ret;
	}

	/* Then try dev.hwpart:part */
	ret = part_get_info_by_dev_and_name_or_num("mmc", name, &dev_desc,
						   info, true);
	return ret;
}

/**
 * mmc_blk_write() - Write/erase MMC in chunks of MAX_BLK_WRITE
 *
 * @block_dev: Pointer to block device
 * @start: First block to write/erase
 * @blkcnt: Count of blocks
 * @buffer: Pointer to data buffer for write or NULL for erase
 */
static lbaint_t mmc_blk_write(struct blk_desc *block_dev, lbaint_t start,
				 lbaint_t blkcnt, const void *buffer)
{
	lbaint_t blk = start;
	lbaint_t blks_written;
	lbaint_t cur_blkcnt;
	lbaint_t blks = 0;
	int i;

	for (i = 0; i < blkcnt; i += MAX_BLK_WRITE) {
		cur_blkcnt = min((int)blkcnt - i, MAX_BLK_WRITE);
		if (buffer) {
			blks_written = blk_dwrite(block_dev, blk, cur_blkcnt,
						  buffer + (i * block_dev->blksz));
		} else {
			blks_written = blk_derase(block_dev, blk, cur_blkcnt);
		}
		blk += blks_written;
		blks += blks_written;
	}
	return blks;
}

static int write_raw_image(struct blk_desc *dev_desc,
			    struct disk_partition *info, const char *part_name,
			    void *buffer, u32 download_bytes)
{
	lbaint_t blkcnt;
	lbaint_t blks;

	/* determine number of blocks to write */
	blkcnt = ((download_bytes + (info->blksz - 1)) & ~(info->blksz - 1));
	blkcnt = lldiv(blkcnt, info->blksz);

	if (blkcnt > info->size) {
		printf("too large for partition: '%s'\n", part_name);
		return RESULT_FAIL;
	}

	puts("Flashing Raw Image\n");

	blks = mmc_blk_write(dev_desc, info->start, blkcnt, buffer);

	if (blks != blkcnt) {
		printf("failed writing to device %d\n", dev_desc->devnum);
		return RESULT_FAIL;
	}

	printf("........ wrote " LBAFU " bytes to '%s'\n", blkcnt * info->blksz,
	       part_name);
	return RESULT_OK;
}

static int check_image_crc(ulong crc_compare, lbaint_t part_start_cnt, ulong blksz, int image_size){
	void *load_addr = (void *)map_sysmem(LOAD_IMG_ADDR, 0);
	u32 div_times = (image_size + LOAD_IMG_SIZE -1)/LOAD_IMG_SIZE;
	ulong crc = 0;
	int byte_remain = image_size;
	int download_bytes = 0;
	u32 blk_size, n;
	unsigned long time_start_flash = get_timer(0);
	/*if crc_compare is 0, return 0 directly*/
	if (!crc_compare)
		return RESULT_OK;

	struct blk_desc *dev_desc = blk_get_dev("mmc", dev_emmc_num);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("invalid mmc device\n");
		return RESULT_FAIL;
	}

	for (int i = 0; i < div_times; i++){
		printf("\ndownload and flash div %d\n", i);
		download_bytes = byte_remain > LOAD_IMG_SIZE ? LOAD_IMG_SIZE : byte_remain;

		blk_size = (download_bytes + (blksz - 1)) / blksz;
		n = blk_dread(dev_desc, part_start_cnt, blk_size, load_addr);
		if (n != blk_size){
			printf("mmc read blk not equal it should be\n");
			return RESULT_FAIL;
		}
		crc = crc32_wd(crc, (const uchar *)load_addr, download_bytes, CHUNKSZ_CRC32);
		part_start_cnt += blk_size;
		byte_remain -= download_bytes;
	}

	printf("get crc value:%lx, compare crc:%lx\n", crc, crc_compare);
	time_start_flash = get_timer(time_start_flash);
	printf("compare crc32 over, use time:%lu ms\n\n", time_start_flash);
	return (crc == crc_compare) ? RESULT_OK : RESULT_FAIL;
}

static int flash_image(struct cmd_tbl *cmdtp, struct flash_dev *fdev){

	char load_str[13] = {"\0"}; /*use to save hex to string*/
	char addr_str[13] = {"\0"};
	char offset_str[13] = {"\0"};
	char mmc_str[2] = {"\0"};
	struct disk_partition info = {0};

	void *load_addr = (void *)map_sysmem(LOAD_IMG_ADDR, 0);
	strcpy(load_str, simple_xtoa((ulong)load_addr));

	for(int i = 0; i < MAX_PARTITION_NUM; i++){
		char *part_name = fdev->parts_info[i].part_name;
		char *file_name = fdev->parts_info[i].file_name;
		u32 crc_value = fdev->parts_info[i].crc;
		unsigned long time_start_flash = get_timer(0);

		printf("%ld, %ld\n", strlen(part_name), strlen(file_name));
		if (strlen(part_name) == 0 || strlen(file_name) == 0){
			printf("part name is null\n");
			break;/*no more part to flash*/
		}
		if (!fdev->parts_info[i].flash){
			printf("should not flash\n");
			continue;/*should not flash*/
		}
		printf("\n\nflash img %s, \n", file_name);
		if (get_part_info(fdev->dev_desc, part_name, &info) < 0){
			printf("can not get partition name:%s, check your config\n", part_name);
			return RESULT_FAIL;
		}

		strcpy(mmc_str, simple_itoa((ulong)dev_sdio_num));
		char *const argv_image_size[] = {"fatsize", "mmc", mmc_str, file_name};
		if (do_size(cmdtp, 0, 4, argv_image_size, FS_TYPE_FAT)){
			printf("can not find file :%s, \n", file_name);
			return RESULT_FAIL;
		}
		printf("info->start:%lx, info->size:%lx, info->blksz:%lx\n", info.start, info.size, info.blksz);
		lbaint_t part_start_cnt = info.start;/*save the partition start cnt*/

		u32 image_size = env_get_hex("filesize", 0);
		u32 byte_remain = image_size;
		u32 div_times = (image_size + LOAD_IMG_SIZE -1)/LOAD_IMG_SIZE;
		u32 download_bytes = 0;
		u32 download_offset = 0;
		printf("\n\ndev_times:%d\n", div_times);
		for (int i = 0; i < div_times; i++){
			printf("\ndownload and flash div %d\n", i);
			download_bytes = byte_remain > LOAD_IMG_SIZE ? LOAD_IMG_SIZE : byte_remain;

			strcpy(addr_str, simple_xtoa((ulong)download_bytes));
			strcpy(offset_str, simple_xtoa((ulong)download_offset));
			char *const argv_image[] = {"fatload", "mmc", mmc_str,
										load_str, file_name,
										addr_str,
										offset_str};
			printf("load from %x, bytes:%x\n", download_offset, download_bytes);
			printf("%s, %s, \n", addr_str, offset_str);
			if (do_load(cmdtp, 0, 7, argv_image, FS_TYPE_FAT))
				return RESULT_FAIL;
			u32 had_download = env_get_hex("filesize", 0);
			if (had_download != download_bytes){
				printf("download file size is not equal require\n");
				return RESULT_FAIL;
			}
			printf("had_download:%x, download byte:%x\n", had_download, download_bytes);

			info.size = (download_bytes + (info.blksz - 1)) / info.blksz;
			printf("write to mmc start_cnt:%lx, size:%lx\n", info.start, info.size);
			if (write_raw_image(fdev->dev_desc, &info, part_name, load_addr, download_bytes)){
				return RESULT_FAIL;
			}
			info.start += info.size;
			download_offset += download_bytes;
			byte_remain -= download_bytes;
		}
		/*read from device and check crc*/
		if (check_image_crc(crc_value, part_start_cnt, info.blksz, image_size)){
			printf("check image crc32 fail, \n");
			return RESULT_FAIL;
		}
		time_start_flash = get_timer(time_start_flash);
		printf("flashing image %s over, use time:%lu ms\n", file_name, time_start_flash);
	}
	return RESULT_OK;
}

static int flash_gpt(struct cmd_tbl *cmdtp, struct flash_dev *fdev){
	char gpt_command[250] = {"\0"};
	sprintf(gpt_command, "gpt write mmc %x '%s'", dev_emmc_num, fdev->gptinfo.gpt_table);
	printf("gpt table command :%s\n", gpt_command);

	if (run_command(gpt_command, 0))
		return RESULT_FAIL;

	return RESULT_OK;
}

/*flash fsbl*/
static int flash_fsbl(struct cmd_tbl *cmdtp, struct flash_dev *fdev){
	char load_str[13] = {"\0"}; /*use to save hex to string*/
	char mmc_str[2] = {"\0"};
	struct disk_partition info = {0};
	char *flash_offset;
	char *s;
	void *load_addr = (void *)map_sysmem(LOAD_IMG_ADDR, 0);
	int ret = 0;

	info.blksz = fdev->dev_desc->blksz;
	info.start = 0;
	info.size = 0;
	flash_offset = malloc(35);
	printf("%x, %s, %x, \n", fdev->fsblinfo.crc, fdev->fsblinfo.offset, fdev->fsblinfo.flash);
	strcpy(load_str, simple_xtoa((ulong)load_addr));
	strcpy(mmc_str, simple_itoa((ulong)dev_sdio_num));
	strcpy(flash_offset, fdev->fsblinfo.offset);

	char *const argv_fsbl[] = {"fatload", "mmc", mmc_str, load_str, "FSBL_REL.bin"};
	if (do_load(cmdtp, 0, 5, argv_fsbl, FS_TYPE_FAT)){
		ret = RESULT_FAIL;
		goto out;
	}

	u32 download_bytes = env_get_hex("filesize", 0);
	printf("download_bytes:%x, info->blksz:%lx\n", download_bytes, info.blksz);
	info.size = (download_bytes + (info.blksz - 1)) / info.blksz;

	for(int i = 0; i < 6; i++){
		s = strsep(&flash_offset, ";");
		if (s == NULL)
			break;
		if (simple_strtoul(s, NULL, 0))
			info.start = simple_strtoul(s, NULL, 0) / info.blksz;
		else{
			info.start = 0;
		}

		printf("flash fsbl offset :%s, %lx, %lx\n", s, info.start, info.size);
		if (write_raw_image(fdev->dev_desc, &info, "fsbl", load_addr, download_bytes)){
			printf("write fsbl to %lx fail\n", info.start * info.blksz);
			ret = RESULT_FAIL;
			goto out;
		}
	}

	/*only check the first fsbl.bin*/
	if (check_image_crc(fdev->fsblinfo.crc, 0x100, info.blksz, download_bytes)){
		printf("check image crc32 fail, \n");
		ret = RESULT_FAIL;
		goto out;
	}
	ret = RESULT_OK;
out:
	free(flash_offset);
	return ret;
}

static int parse_flash_info(struct cmd_tbl *cmdtp, int flag, struct flash_dev *fdev)
{
	int  nodeoffset;	/* node offset from libfdt */
	int  nextoffset;	/* next node offset from libfdt */
	uint32_t tag;		/* tag */
	const char root[2] = "/";
	int  len = 0;		/* new length of the property */
	char load_str[13] = {"\0"}; /*use to save hex to string*/
	char mmc_str[2] = {"\0"};
	char gpt_table[256] = {"\0"};

	struct blk_desc *dev_desc;
	void *load_addr = (void *)map_sysmem(LOAD_IMG_ADDR, 0);
	/* use to save fdt */
	struct fdt_header *blob = (struct fdt_header *)load_addr;

	/*try to get emmc/sdio dev number*/
	if (check_mmc_exist())
		return 1;

	dev_desc = blk_get_dev("mmc", dev_emmc_num);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("invalid mmc device\n");
		return RESULT_FAIL;
	}
	fdev->dev_desc = dev_desc;

	strcpy(load_str, simple_xtoa((ulong)blob));
	strcpy(mmc_str, simple_itoa((ulong)dev_sdio_num));
	char *const fat_argv[] = {"fatload", "mmc", mmc_str, load_str, "flash_config"};
	if (do_load(cmdtp, flag, 5, fat_argv, FS_TYPE_FAT))
		return RESULT_FAIL;

	if (fdt_check_header(blob) || !fdt_valid(&blob)){
		printf("not a valid fdt\n");
		return RESULT_FAIL;
	}
	nodeoffset = fdt_path_offset (blob, root);
	if (nodeoffset < 0) {
		/*
		 * Not found or something else bad happened.
		 */
		printf ("libfdt fdt_path_offset() returned %s\n",
			fdt_strerror(nodeoffset));
		return RESULT_FAIL;
	}

	fdt_next_tag(blob, nodeoffset, &nextoffset);
	nodeoffset = nextoffset;

	u32 part_index = 0;
	bool find_next_tag = true;
	while(find_next_tag) {
		tag = fdt_next_tag(blob, nodeoffset, &nextoffset);
		switch(tag) {
			case FDT_BEGIN_NODE:
				const char *node_name = fdt_get_name(blob, nodeoffset, NULL);
				const char *flash_mark = fdt_getprop(blob, nodeoffset, "flash", &len);
				const char *node_crc = fdt_getprop(blob, nodeoffset, "crc", &len);

				if (node_name[0] == 'g'){
					fdev->gptinfo.flash = (flash_mark[0] == 't') ? true : false;
					break;
				}else if(node_name[0] == 'f'){
					fdev->fsblinfo.flash = (flash_mark[0] == 't') ? true : false;
					fdev->fsblinfo.crc = simple_strtoul(node_crc, NULL, 0);
					const char *fsbl_offset = fdt_getprop(blob, nodeoffset, "mmc-offset", &len);
					const char *fsbl_offset_start = fdt_getprop(blob, nodeoffset, "size", &len);
					strcpy(fdev->fsblinfo.offset, fsbl_offset);
					strcpy(fdev->gptinfo.gpt_start, fsbl_offset_start);
					printf("fdev->gpt_info->gpt_start:%s\n", fdev->gptinfo.gpt_start);
					break;
				}

				const char *node_part = fdt_getprop(blob, nodeoffset, "partition", &len);
				const char *node_file = fdt_getprop(blob, nodeoffset, "filename", &len);
				const char *node_size = fdt_getprop(blob, nodeoffset, "size", &len);

				/*save gpt info to a string*/
				printf("save gpt_table\n");
				if (part_index == 0)
					sprintf(gpt_table, "name=%s,start=%s,size=%s;", node_part, fdev->gptinfo.gpt_start, node_size);
				else
					sprintf(gpt_table, "%sname=%s,size=%s;", gpt_table, node_part, node_size);

				fdev->parts_info[part_index].flash = (flash_mark[0] == 't') ? true : false;
				strcpy(fdev->parts_info[part_index].part_name, node_part);
				strcpy(fdev->parts_info[part_index].file_name, node_file);
				strcpy(fdev->parts_info[part_index].size, node_size);
				fdev->parts_info[part_index].crc = simple_strtoul(node_crc, NULL, 0);

				printf("fdt_get_name:%s, %s, %s, %x\n", node_name
													, fdev->parts_info[part_index].part_name
													, fdev->parts_info[part_index].file_name
													, fdev->parts_info[part_index].crc);
				part_index++;
				break;

			case FDT_END:
				find_next_tag = false;
				break;
		}
		nodeoffset = nextoffset;
	}

	strcpy(fdev->gptinfo.gpt_table, gpt_table);
	return 0;
}

static int mmc_recovery(struct cmd_tbl *cmdtp, int flag, int argc,
		   char *const argv[], struct flash_dev *fdev)
{
	printf("%s, \n", __func__);

	if (parse_flash_info(cmdtp, flag, fdev))
		return RESULT_FAIL;

	/*flash gpt.img*/
	if (fdev->gptinfo.flash){
		if (flash_gpt(cmdtp, fdev))
			return RESULT_FAIL;
	}

	if (flash_image(cmdtp, fdev))
		return RESULT_FAIL;

	if (fdev->fsblinfo.flash){
		if (flash_fsbl(cmdtp, fdev))
			return RESULT_FAIL;
	}

	return RESULT_OK;
	return 0;
}

static int net_recovery(struct cmd_tbl *cmdtp, int flag, int argc,
		   char *const argv[], struct flash_dev *fdev)
{
	/*
	TODO:
		net boot
	*/
	return 0;
}

static int do_recovery(struct cmd_tbl *cmdtp, int flag, int argc,
		   char *const argv[])
{
	struct flash_dev *fdev;
	int ret = 0;
	fdev = malloc(sizeof(struct flash_dev));
	if (!fdev){
		ret = RESULT_FAIL;
	}

	ulong time_start_flash = get_timer(0);
	printf("time_start_flash:%ld\n", time_start_flash);

	if (argc == 1){
		ret = mmc_recovery(cmdtp, flag, argc, argv, fdev);
	}else{
		ret = net_recovery(cmdtp, flag, argc, argv, fdev);
	}


	printf("recover test end\n");
	ulong time_enc_flash = get_timer(0);
	printf("flashing over, use time:%lu ms\n", time_enc_flash - time_start_flash);
	free(fdev);

	recovery_show_result(ret);
	return 0;
}

U_BOOT_CMD(
	recovery,	3,	0,	do_recovery,
	"use to flash image to emmc/nor/nand from sd/usb/net",
	"\n"
	"	- flash image from sd card\n"
	"help command ...\n"
	"	- flash image from sd card to emmc/nand/nor"
);
