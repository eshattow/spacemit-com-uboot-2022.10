// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023,  chris.huang<chris.huang@spacemit.com>
 */

#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <blk.h>
#include <bootstage.h>
#include <command.h>
#include <common.h>
#include <console.h>
#include <div64.h>
#include <dm.h>
#include <dm/uclass-internal.h>
#include <fdt_support.h>
#include <fs.h>
#include <image.h>
#include <malloc.h>
#include <mapmem.h>
#include <memalign.h>
#include <mmc.h>
#include <part.h>
#include <u-boot/crc.h>
#include <usb.h>
#include "recovery.h"

static int dev_emmc_num = -1;
static int dev_sdio_num = -1;

/* Initialize the mmc device given its number */
static int init_mmc_device(int dev_num)
{
	struct mmc *mmc = find_mmc_device(dev_num);

	if (!mmc) {
		printf("Cannot find mmc device %d\n", dev_num);
		return RESULT_FAIL;
	}

	if (mmc_init(mmc)) {
		printf("mmc init failed for device %d\n", dev_num);
		return RESULT_FAIL;
	}

	return RESULT_OK;
}

static void free_flash_dev(struct flash_dev *fdev)
{
	for (int i = 0; i < MAX_PARTITION_NUM; i++){
		if (fdev->parts_info[i].part_name != NULL){
			free(fdev->parts_info[i].part_name);
			free(fdev->parts_info[i].file_name);
			free(fdev->parts_info[i].size);
		}else{
			break;
		}
	}
	free(fdev);
}

/* Detect and classify mmc device */
static void detect_and_classify_mmc(int dev_num)
{
	int current_dev_num;
	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
		return;

	current_dev_num = mmc_get_blk_desc(mmc)->devnum;
	if (IS_SD(mmc)) {
		dev_sdio_num = current_dev_num;
		printf("SDIO detected with number: %d\n", dev_sdio_num);
	} else {
		dev_emmc_num = current_dev_num;
		printf("eMMC initialized with number: %d\n", dev_emmc_num);
	}
}

int check_mmc_exist_and_initialize(void)
{
	int mmc_dev_num = get_mmc_num();
	bool has_emmc = false;

	for (int i = 0; i < mmc_dev_num; i++) {
		if (init_mmc_device(i) == RESULT_OK) {
			detect_and_classify_mmc(i);
			if (dev_emmc_num != -1)
				has_emmc = true;
		}
	}

	if (!has_emmc) {
		printf("Failed to initialize eMMC device.\n");
		return RESULT_FAIL;
	}

	if (dev_sdio_num == -1) {
		printf("SDIO not detected.\n");
	}

	return RESULT_OK;
}

static int load_from_device(struct cmd_tbl *cmdtp, char *load_str,
			struct fdt_header *blob, int device_type,
			struct flash_dev *fdev)
{
	int retval = RESULT_OK;

	strcpy(load_str, simple_xtoa((ulong)blob));
	switch (device_type) {
	case DEVICE_MMC:
		if (check_mmc_exist_and_initialize() != RESULT_OK) {
			retval = RESULT_FAIL;
			break;
		}
		fdev->dev_desc = blk_get_dev("mmc", dev_emmc_num);
		if (!fdev->dev_desc || fdev->dev_desc->type == DEV_TYPE_UNKNOWN) {
			printf("get emmc_device faild\n");
			retval = RESULT_FAIL;
			break;
		}
		fdev->dev_str = strdup(simple_itoa((ulong)dev_sdio_num));
		fdev->device_name = strdup("mmc");
		break;

	case DEVICE_USB:
		/* Initialize eMMC */
		if (check_mmc_exist_and_initialize() != RESULT_OK) {
			printf("Failed to initialize eMMC while handling USB.\n");
			retval = RESULT_FAIL;
			break;
		}
		fdev->dev_desc = blk_get_dev("mmc", dev_emmc_num);
		if (!fdev->dev_desc || fdev->dev_desc->type == DEV_TYPE_UNKNOWN) {
			printf("Failed to get eMMC device descriptor while handling USB.\n");
			retval = RESULT_FAIL;
			break;
		}
		usb_init();
		int device_number = usb_stor_scan(1);
		if (device_number < 0) {
			retval = RESULT_FAIL;
			break;
		}
		fdev->dev_str = strdup(simple_itoa((ulong)device_number));
		fdev->device_name = strdup("usb");
		break;

	case DEVICE_NET:
		retval = RESULT_FAIL;
		break;

	default:
		printf("Unknown device type!\n");
		retval = RESULT_FAIL;
		break;
	}

	/* If the above operation fails, return early */
	if (retval != RESULT_OK) {
		return retval;
	}

	printf("device_name: %s\n", fdev->device_name);
	printf("dev_str: %s\n", fdev->dev_str);


	char *temp_fname = malloc(strlen(file_image[FLASH_CONFIG]) + strlen(RECOVERY_FOLDER) + 2);
	if (!temp_fname){
		printf("malloc file_name fail\n");
		return RESULT_FAIL;
	}
	strcpy(temp_fname, RECOVERY_FOLDER);
	strcat(temp_fname, "/");
	strcat(temp_fname, file_image[FLASH_CONFIG]);

	char *fat_argv[] = {"fatload", fdev->device_name, fdev->dev_str, load_str, temp_fname};

	if (do_load(cmdtp, 0, 5, fat_argv, FS_TYPE_FAT)) {
		printf("do_load flash_config from %s failed\n", fdev->device_name);
		retval = RESULT_FAIL;
	} else {
		printf("do_load flash_config %s success\n", fdev->device_name);
	}

	free(temp_fname);
	return retval;
}

void recovery_show_result(struct flash_dev *fdev, int ret)
{
	if (ret) {
		printf("!!!!!!!!!!!!!!!!!!! recovery flash false !!!!!!!!!!!!!!!!!!!\n");
	} else {
		printf("################### recovery flash success ###################\n");
	}

	/*free the malloc paramenter*/
	free_flash_dev(fdev);

	while(1){
		/*do not retrun while flashing over!*/
	}
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
	ret = part_get_info_by_dev_and_name_or_num("mmc", name, &dev_desc, info, true);
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

	printf("........ wrote " LBAFU " bytes to '%s'\n", \
		blkcnt * info->blksz,part_name);
	return RESULT_OK;
}

static int check_image_crc(ulong crc_compare, lbaint_t part_start_cnt,
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
		return RESULT_OK;

	struct blk_desc *dev_desc = blk_get_dev("mmc", dev_emmc_num);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("invalid mmc device\n");
		return RESULT_FAIL;
	}

	for (int i = 0; i < div_times; i++) {
		printf("\ndownload and flash div %d\n", i);
		download_bytes = byte_remain > RECOVERY_LOAD_IMG_SIZE ? RECOVERY_LOAD_IMG_SIZE : byte_remain;

		blk_size = (download_bytes + (blksz - 1)) / blksz;
		n = blk_dread(dev_desc, part_start_cnt, blk_size, load_addr);
		if (n != blk_size) {
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

static int flash_image(struct cmd_tbl *cmdtp, struct flash_dev *fdev)
{
	char load_str[13] = {"\0"};
	char addr_str[13] = {"\0"};
	char offset_str[13] = {"\0"};
	struct disk_partition info = {0};
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
	lbaint_t part_start_cnt;
	u32 image_size = 0;
	u32 byte_remain = 0;
	u32 div_times = 0;
	u32 download_bytes = 0;
	u32 download_offset = 0;
	u32 had_download = 0;

	strcpy(load_str, simple_xtoa((ulong)load_addr));

	for (int i = 0; i < MAX_PARTITION_NUM; i++) {
		char *part_name = fdev->parts_info[i].part_name;
		char *file_name = fdev->parts_info[i].file_name;
		u32 crc_value = fdev->parts_info[i].crc;
		unsigned long time_start_flash = get_timer(0);

		if (fdev->parts_info[i].file_name == NULL || strlen(fdev->parts_info[i].file_name) == 0) {
			/* no more part to flash */
			printf("part name is null\n");
			break;
		}
		if (!fdev->parts_info[i].flash) {
			/* should not flash */
			printf("should not flash part:%s\n", fdev->parts_info[i].part_name);
			continue;
		}
		printf("\n\nflash img %s, part_name:%s\n", file_name, part_name);

		if (get_part_info(fdev->dev_desc, part_name, &info) < 0) {
			printf("can not get partition name:%s, check your config\n", part_name);
			return RESULT_FAIL;
		}
		char *const argv_image_size[] = {"fatsize", fdev->device_name, fdev->dev_str, file_name};
		if (do_size(cmdtp, 0, 4, argv_image_size, FS_TYPE_FAT)) {
			printf("can not find file :%s, \n", file_name);
			return RESULT_FAIL;
		}
		printf("info->start:%lx, info->size:%lx, info->blksz:%lx\n", info.start, info.size, info.blksz);

		/* save the partition start cnt */
		part_start_cnt = info.start;
		image_size = env_get_hex("filesize", 0);
		byte_remain = image_size;
		div_times = (image_size + RECOVERY_LOAD_IMG_SIZE - 1) / RECOVERY_LOAD_IMG_SIZE;
		printf("\n\ndev_times:%d\n", div_times);
		for (int j = 0; j < div_times; j++) {
			printf("\ndownload and flash div %d\n", j);
			download_bytes = byte_remain > RECOVERY_LOAD_IMG_SIZE ? RECOVERY_LOAD_IMG_SIZE : byte_remain;

			strcpy(addr_str, simple_xtoa((ulong)download_bytes));
			strcpy(offset_str, simple_xtoa((ulong)download_offset));
			char *const argv_image[] = {"fatload", fdev->device_name, fdev->dev_str,
					load_str, file_name, addr_str, offset_str};
			printf("load from %x, bytes:%x\n", download_offset, download_bytes);
			printf("%s, %s, \n", addr_str, offset_str);
			if (do_load(cmdtp, 0, 7, argv_image, FS_TYPE_FAT))
				return RESULT_FAIL;
			had_download = env_get_hex("filesize", 0);
			if (had_download != download_bytes) {
				printf("download file size is not equal require\n");
				return RESULT_FAIL;
			}
			printf("had_download:%x, download byte:%x\n", had_download, download_bytes);

			info.size = (download_bytes + (info.blksz - 1)) / info.blksz;
			printf("write to mmc start_cnt:%lx, size:%lx\n", info.start, info.size);
			if (write_raw_image(fdev->dev_desc, &info, part_name, load_addr, download_bytes))
				return RESULT_FAIL;
			info.start += info.size;
			download_offset += download_bytes;
			byte_remain -= download_bytes;
		}
		/* read from device and check crc */
		if (check_image_crc(crc_value, part_start_cnt, info.blksz, image_size)) {
			printf("check image crc32 fail, \n");
			return RESULT_FAIL;
		}
		time_start_flash = get_timer(time_start_flash);
		printf("flashing image %s over, use time:%lu ms\n", file_name, time_start_flash);
	}
	return RESULT_OK;
}

static int flash_gpt(struct cmd_tbl *cmdtp, struct flash_dev *fdev)
{
	char gpt_command[250] = {"\0"};
	sprintf(gpt_command, "gpt write mmc %x '%s'", dev_emmc_num, fdev->gptinfo.gpt_table);
	printf("gpt table command :%s\n", gpt_command);
	if (run_command(gpt_command, 0))
		return RESULT_FAIL;
	return RESULT_OK;
}

/*flash fsbl*/
static int flash_fsbl(struct cmd_tbl *cmdtp, struct flash_dev *fdev)
{
	char load_str[13] = {"\0"};
	struct disk_partition info = {0};
	char *flash_offset;
	char *s;
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
	int result = RESULT_OK;
	u32 download_bytes = 0;

	info.blksz = fdev->dev_desc->blksz;
	info.start = 0;
	info.size = 0;

	flash_offset = malloc(35);
	if (!flash_offset) {
		printf("Memory allocation failed for flash_offset\n");
		return RESULT_FAIL;
	}
	char *original_flash_offset = flash_offset;

	printf("%x, %s, \n", fdev->fsblinfo.crc, fdev->fsblinfo.offset);
	strcpy(load_str, simple_xtoa((ulong)load_addr));
	strcpy(flash_offset, fdev->fsblinfo.offset);


	char *temp_fname = malloc(strlen(file_image[FSBL_BIN]) + strlen(RECOVERY_FOLDER) + 2);
	if (!temp_fname){
		printf("malloc temp_fname fail\n");
		return RESULT_FAIL;
	}
	strcpy(temp_fname, RECOVERY_FOLDER);
	strcat(temp_fname, "/");
	strcat(temp_fname, file_image[FSBL_BIN]);

	char *const argv_fsbl[] = {"fatload", fdev->device_name, fdev->dev_str, load_str, temp_fname};
	if (do_load(cmdtp, 0, 5, argv_fsbl, FS_TYPE_FAT)) {
		result = RESULT_FAIL;
		goto cleanup;
	}

	download_bytes = env_get_hex("filesize", 0);
	info.size = (download_bytes + (info.blksz - 1)) / info.blksz;
	printf("download_bytes:%x, info->blksz:%lx, info->size:%lx\n", download_bytes, info.blksz, info.size);

	for (int i = 0; i < 6; i++) {
		s = strsep(&flash_offset, ";");
		if (s == NULL)
			break;
		if (simple_strtoul(s, NULL, 0))
			info.start = simple_strtoul(s, NULL, 0) / info.blksz;
		else
			info.start = 0;

		printf("flash fsbl offset :%s, %lx\n", s, info.start);
		if (write_raw_image(fdev->dev_desc, &info, "fsbl", load_addr, download_bytes)) {
			printf("write fsbl to %lx fail\n", info.start * info.blksz);
			result = RESULT_FAIL;
			goto cleanup; /*jump to cleanup before exiting*/
		}
	}

	// only check the first fsbl.bin
	if (check_image_crc(fdev->fsblinfo.crc, 0x100, info.blksz, download_bytes)) {
		printf("check image crc32 fail, \n");
		result = RESULT_FAIL;
	}

cleanup:
	/* Always free the original allocated memory */
	free(original_flash_offset);
	free(temp_fname);
	return result;
}

static int parse_fdt(struct flash_dev *fdev)
{
	char root[2] = "/";
	int nodeoffset; /* node offset from libfdt */
	int nextoffset; /* next node offset from libfdt */
	int len = 0;	/* new length of the property */
	uint32_t tag;
	u32 part_index = 0;
	char gpt_table[256] = {"\0"};
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0); /*use to save hex to string*/
	struct fdt_header *blob = (struct fdt_header *)load_addr;

	nodeoffset = fdt_path_offset(blob, root);
	if (nodeoffset < 0) {
		/*
		 * Not found or something else bad happened.
		 */
		printf("libfdt fdt_path_offset() returned %s\n", fdt_strerror(nodeoffset));
		return RESULT_FAIL;
	}
	fdt_next_tag(blob, nodeoffset, &nextoffset);
	nodeoffset = nextoffset;
	bool find_next_tag = true;
	while (find_next_tag) {
		tag = fdt_next_tag(blob, nodeoffset, &nextoffset);
		switch (tag) {
		case FDT_BEGIN_NODE:
			const char *node_name = fdt_get_name(blob, nodeoffset, NULL);
			const char *flash_mark = fdt_getprop(blob, nodeoffset, "flash", &len);
			const char *node_crc = fdt_getprop(blob, nodeoffset, "crc", &len);

			if (node_name[0] == 'g') {
				fdev->gptinfo.flash = (flash_mark[0] == 't') ? true : false;
				break;
			} else if (node_name[0] == 'f') {
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

			/*after finish recovery, it would free the malloc paramenter at func recovery_show_result*/
			fdev->parts_info[part_index].part_name = malloc(strlen(node_part));
			if (!fdev->parts_info[part_index].part_name){
				printf("malloc part_name fail\n");
				return RESULT_FAIL;
			}
			strcpy(fdev->parts_info[part_index].part_name, node_part);

			fdev->parts_info[part_index].size = malloc(strlen(node_size));
			if (!fdev->parts_info[part_index].size){
				printf("malloc size fail\n");
				return RESULT_FAIL;
			}
			strcpy(fdev->parts_info[part_index].size, node_size);

			fdev->parts_info[part_index].file_name = malloc(strlen(node_file) + strlen(RECOVERY_FOLDER) + 2);
			if (!fdev->parts_info[part_index].file_name){
				printf("malloc file_name fail\n");
				return RESULT_FAIL;
			}
			strcpy(fdev->parts_info[part_index].file_name, RECOVERY_FOLDER);
			strcat(fdev->parts_info[part_index].file_name, "/");
			strcat(fdev->parts_info[part_index].file_name, node_file);

			fdev->parts_info[part_index].crc = simple_strtoul(node_crc, NULL, 0);
			printf("fdt_get_name:%s, %s, %s, %x\n", node_name, \
				fdev->parts_info[part_index].part_name, \
				fdev->parts_info[part_index].file_name, \
				fdev->parts_info[part_index].crc);
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

/*Attempt to load recovery files from all possible sources*/
static int load_recovery_file(struct cmd_tbl *cmdtp, struct flash_dev *fdev,
			int argc, char *const argv[])
{
	char load_str[13] = {"\0"};
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
	struct fdt_header *blob = (struct fdt_header *)load_addr;
	int device_type, result;

	if (argc < 2) {
		printf("Error: Missing source argument. Expected 'mmc', 'usb', or 'net'.\n");
		return CMD_RET_USAGE;
	}
	if (strcmp(argv[1], "mmc") == 0) {
		device_type = DEVICE_MMC;
	} else if (strcmp(argv[1], "usb") == 0) {
		device_type = DEVICE_USB;
	} else if (strcmp(argv[1], "net") == 0) {
		device_type = DEVICE_NET;
	} else {
		printf("Error: Invalid source '%s'. Expected 'mmc', 'usb', or 'net'.\n", argv[1]);
		return CMD_RET_USAGE;
	}

	result = load_from_device(cmdtp, load_str, blob, device_type, fdev);

	return result;
}

static int perform_flash_operations(struct cmd_tbl *cmdtp, struct flash_dev *fdev)
{
	if (fdev->gptinfo.flash && flash_gpt(cmdtp, fdev)) {
		return RESULT_FAIL;
	}
	if (flash_image(cmdtp, fdev)) {
		return RESULT_FAIL;
	}
	if (fdev->fsblinfo.flash && flash_fsbl(cmdtp, fdev)) {
		return RESULT_FAIL;
	}

	/* all flash operations successed */
	return RESULT_OK;
}

static int do_recovery(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	printf("RECOVERY_LOAD_IMG_ADDR:%lx, RECOVERY_LOAD_IMG_SIZE:%llx\n", RECOVERY_LOAD_IMG_ADDR, RECOVERY_LOAD_IMG_SIZE);
	struct flash_dev *fdev;

	/*fdev would free after finish revocery at func recovery_show_result*/
	fdev = malloc(sizeof(struct flash_dev));
	if (!fdev) {
		printf("Memory allocation failed!\n");
		return RESULT_FAIL;
	}
	memset(fdev, 0, sizeof(struct flash_dev));

	unsigned long time_start_flash = get_timer(0);

	/*Load flash_config.cfg file*/
	int result = load_recovery_file(cmdtp, fdev, argc, argv);
	if (result != RESULT_OK) {
		recovery_show_result(fdev, RESULT_FAIL);
		return RESULT_FAIL;
	}

	/*Parse FDT and fill in relevant data structures*/
	if (parse_fdt(fdev)) {
		printf("Failed to parse FDT.\n");
		recovery_show_result(fdev, RESULT_FAIL);
		return RESULT_FAIL;
	}

	/*Perform programming operation based on the provided information*/
	if (perform_flash_operations(cmdtp, fdev)) {
		printf("Failed to flash the device.\n");
		recovery_show_result(fdev, RESULT_FAIL);
		return RESULT_FAIL;
	}

	ulong time_enc_flash = get_timer(0);
	printf("flashing over, use time:%lu ms\n", time_enc_flash - time_start_flash);
	recovery_show_result(fdev, RESULT_OK);
	return 0;
}

U_BOOT_CMD(
	recovery, 2, 1, do_recovery,
	"flash image from specified source",
	"<source>\n"
	"    - <source>: mmc | usb | net\n"
	"      flash image from the specified source (e.g., mmc, usb, or net) to emmc/nand/nor.");
