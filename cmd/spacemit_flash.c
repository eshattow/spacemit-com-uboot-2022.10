// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
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
#include <fs.h>
#include <image.h>
#include <malloc.h>
#include <mapmem.h>
#include <memalign.h>
#include <mmc.h>
#include <part.h>
#include <u-boot/crc.h>
#include <usb.h>
#include <fb_spacemit.h>
#include <cJSON.h>

static int dev_emmc_num = -1;
static int dev_sdio_num = -1;
static u32 bootfs_part_index = 0;

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
	free(fdev->gptinfo.gpt_table);
	free(fdev->mtd_table);
	free(fdev);
}


/* Initialize the mmc device given its number */
static int init_mmc_device(int dev_num)
{
	struct mmc *mmc = find_mmc_device(dev_num);

	if (!mmc) {
		debug("Cannot find mmc device %d\n", dev_num);
		return RESULT_FAIL;
	}

	if (mmc_init(mmc)) {
		debug("mmc init failed for device %d\n", dev_num);
		return RESULT_FAIL;
	}
	return RESULT_OK;
}

/* Detect and classify mmc device */
static void detect_and_classify_mmc(int dev_num)
{

	int current_dev_num, err;
	struct disk_partition info;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
		return;

	current_dev_num = mmc_get_blk_desc(mmc)->devnum;
	if (IS_SD(mmc)) {
		dev_sdio_num = current_dev_num;
		for (u32 p = 1; p <= MAX_SEARCH_PARTITIONS; p++) {
			err = part_get_info(mmc_get_blk_desc(mmc), p, &info);
			if (err)
				continue;
			if (!strcmp(FLASH_IMG_PARTNAME, info.name)){
				debug("match info.name:%s\n", info.name);
				bootfs_part_index = p;
				break;
			}
		}
		debug("SDIO detected with number: %d\n", dev_sdio_num);
	} else {
		dev_emmc_num = current_dev_num;
		debug("eMMC initialized with number: %d\n", dev_emmc_num);
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
		return RESULT_FAIL;
	}
	return RESULT_OK;
}

static int load_from_device(struct cmd_tbl *cmdtp, char *load_str,
			int device_type, struct flash_dev *fdev)
{
	int retval = RESULT_OK;

	switch (device_type) {
#ifdef CONFIG_MMC
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
#ifdef CONFIG_USB_STORAGE
		int device_number = usb_stor_scan(1);
		if (device_number < 0) {
			retval = RESULT_FAIL;
			break;
		}
		fdev->dev_str = strdup(simple_itoa((ulong)device_number));
		fdev->device_name = strdup("usb");
#else
		retval = RESULT_FAIL;
#endif
		break;

	case DEVICE_NET:
		retval = RESULT_FAIL;
		break;
#endif //CONFIG_MMC
	default:
		printf("Unknown device type!\n");
		retval = RESULT_FAIL;
		break;
	}

	/* If the above operation fails, return early */
	if (retval != RESULT_OK) {
		return retval;
	}

	debug("device_name: %s\n", fdev->device_name);
	debug("dev_str: %s\n", fdev->dev_str);

	/*
		TODO:should get partition.json name by searching the folder, and match
		to the storage type.
	*/
	char *temp_fname = malloc(strlen(FLASH_CONFIG_NAME) + strlen(FLASH_IMG_FOLDER) + 2);
	if (!temp_fname){
		printf("malloc file_name fail\n");
		return RESULT_FAIL;
	}

	if (strlen(FLASH_IMG_FOLDER) > 0){
		strcpy(temp_fname, FLASH_IMG_FOLDER);
		strcat(temp_fname, "/");
	}
	strcat(temp_fname, FLASH_CONFIG_NAME);

	char *fat_argv[] = {"fatload", fdev->device_name, fdev->dev_str, ":",
						simple_itoa(bootfs_part_index), load_str, temp_fname};
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
static __maybe_unused lbaint_t mmc_blk_write(struct blk_desc *block_dev, lbaint_t start,
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
#ifdef CONFIG_MMC
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
#else
	printf("not mmc dev found\n");
	return RESULT_FAIL;
#endif
	
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
	char *fsbl_offset;

	strcpy(load_str, simple_xtoa((ulong)load_addr));

	for (int i = 0; i < MAX_PARTITION_NUM; i++) {
		char *part_name = fdev->parts_info[i].part_name;
		char *file_name = fdev->parts_info[i].file_name;
		/*
			TODO: need to caculate crc while load to ram.
			would not get crc value from partition.json
		*/
		u32 crc_value = 0;
		unsigned long time_start_flash = get_timer(0);

		if (fdev->parts_info[i].part_name == NULL) {
			printf("no more partition to flash\n");
			break;
		}

		if (fdev->parts_info[i].file_name == NULL) {
			/* if not file not exists, it mean not to flash */
			printf("part name is null\n");
			continue;
		}
		printf("\n\nflash img %s, part_name:%s\n", file_name, part_name);

		char *const argv_image_size[] = {"fatsize", fdev->device_name, fdev->dev_str, file_name};
		if (do_size(cmdtp, 0, 4, argv_image_size, FS_TYPE_FAT)) {
			printf("can not find file :%s, \n", file_name);
			return RESULT_FAIL;
		}
		debug("info->start:%lx, info->size:%lx, info->blksz:%lx\n", info.start, info.size, info.blksz);

		image_size = env_get_hex("filesize", 0);
		byte_remain = image_size;
		div_times = (image_size + RECOVERY_LOAD_IMG_SIZE - 1) / RECOVERY_LOAD_IMG_SIZE;
		debug("\n\ndev_times:%d\n", div_times);

		if (get_part_info(fdev->dev_desc, part_name, &info) < 0) {
			if (strncmp(part_name, "fsbl", 4) == 0){
				printf("try to flash fsbl\n");
				fsbl_offset = part_name;
				strsep(&fsbl_offset, ":");
				u32 f_offset = fdev->gptinfo.gpt_start_offset + simple_strtoul(fsbl_offset, NULL, 0);
				if (f_offset % info.blksz){
					printf("offset need to be align 0x200\n");
					return -1;
				}
				info.start = f_offset / info.blksz;

				char *const argv_fsbl[] = {"fatload", fdev->device_name, fdev->dev_str, load_str, file_name};
				if (do_load(cmdtp, 0, 5, argv_fsbl, FS_TYPE_FAT)) {
					return RESULT_FAIL;
				}
#ifdef FASTBOOT_FLASH_MMC
				printf("write fsbl to %x, info.start:%lx\n", f_offset, info.start);
				fastboot_mmc_flash_offset(f_offset, load_addr, image_size);
#endif
				/*has flash fsbl, */
				div_times = 0;
			}else{
				printf("can not get partition name:%s, check your config\n", part_name);
				return RESULT_FAIL;
			}
		}
		/* save the partition start cnt */
		part_start_cnt = info.start;

		for (int j = 0; j < div_times; j++) {
			debug("\ndownload and flash div %d\n", j);
			download_bytes = byte_remain > RECOVERY_LOAD_IMG_SIZE ? RECOVERY_LOAD_IMG_SIZE : byte_remain;

			crc_value = crc32_wd(0, (const uchar *)load_addr, download_bytes, CHUNKSZ_CRC32);
			strcpy(addr_str, simple_xtoa((ulong)download_bytes));
			strcpy(offset_str, simple_xtoa((ulong)download_offset));
			char *const argv_image[] = {"fatload", fdev->device_name, fdev->dev_str,
					load_str, file_name, addr_str, offset_str};
			debug("load from %x, bytes:%x\n", download_offset, download_bytes);
			debug("%s, %s, \n", addr_str, offset_str);
			if (do_load(cmdtp, 0, 7, argv_image, FS_TYPE_FAT))
				return RESULT_FAIL;
			had_download = env_get_hex("filesize", 0);
			if (had_download != download_bytes) {
				printf("download file size is not equal require\n");
				return RESULT_FAIL;
			}
			debug("had_download:%x, download byte:%x\n", had_download, download_bytes);

			info.size = (download_bytes + (info.blksz - 1)) / info.blksz;
			debug("write to mmc start_cnt:%lx, size:%lx\n", info.start, info.size);
			if (write_raw_image(fdev->dev_desc, &info, part_name, load_addr, download_bytes))
				return RESULT_FAIL;
			info.start += info.size;
			download_offset += download_bytes;
			byte_remain -= download_bytes;
		}
		/* read from device and check crc */
		debug("check crc, read %lx, imagesize:%d\n", part_start_cnt, image_size);
#ifdef FASTBOOT_FLASH_MMC
		if (check_mmc_image_crc(fdev->dev_desc, crc_value, part_start_cnt, info.blksz, image_size)) {
			printf("check image crc32 fail, \n");
			return RESULT_FAIL;
		}
#endif
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

static int parse_flash_config(struct flash_dev *fdev)
{
#ifdef FASTBOOT_FLASH_MMC
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
	return _parse_flash_config(fdev, load_addr);
#else
	return 0;
#endif
}

/*Attempt to load recovery files from all possible sources*/
static int load_recovery_file(struct cmd_tbl *cmdtp, struct flash_dev *fdev,
			int argc, char *const argv[])
{
	char load_str[13] = {"\0"};
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
	strcpy(load_str, simple_xtoa((ulong)load_addr));
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

	result = load_from_device(cmdtp, load_str, device_type, fdev);

	return result;
}

static int perform_flash_operations(struct cmd_tbl *cmdtp, struct flash_dev *fdev)
{
	/*flash gpt as default*/
	if (flash_gpt(cmdtp, fdev)) {
		return RESULT_FAIL;
	}
	if (flash_image(cmdtp, fdev)) {
		return RESULT_FAIL;
	}

	/* all flash operations successed */
	return RESULT_OK;
}

static int do_flash_image(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	printf("RECOVERY_LOAD_IMG_ADDR:%lx, RECOVERY_LOAD_IMG_SIZE:%llx\n", RECOVERY_LOAD_IMG_ADDR, RECOVERY_LOAD_IMG_SIZE);
	struct flash_dev *fdev;

	/*fdev would free after finish revocery at func recovery_show_result*/
	fdev = malloc(sizeof(struct flash_dev));
	if (!fdev) {
		printf("Memory allocation failed!\n");
		return RESULT_FAIL;
	}
	/*would realloc the size*/
	fdev->gptinfo.gpt_table = malloc(1);
	fdev->mtd_table = malloc(1);

	memset(fdev, 0, sizeof(struct flash_dev));

	unsigned long time_start_flash = get_timer(0);

	/*Load flash_config.cfg file*/
	int result = load_recovery_file(cmdtp, fdev, argc, argv);
	if (result != RESULT_OK) {
		recovery_show_result(fdev, RESULT_FAIL);
		return RESULT_FAIL;
	}

	/*Parse json file and fill in relevant data structures*/
	if (parse_flash_config(fdev)) {
		printf("Failed to parse flash config.\n");
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
	spacemit_flashing, 2, 1, do_flash_image,
	"flash image from specified source",
	"<source>\n"
	"    - <source>: mmc | usb | net\n"
	"      flash image from the specified source (e.g., mmc, usb, or net) to emmc/nand/nor.");
