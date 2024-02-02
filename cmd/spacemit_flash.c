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
#include <env.h>

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

int download_file_via_tftp(char *file_name, char *load_addr) {
	char full_path[128];
	char cmd_buffer[256];
	char *tftp_server_ip;
	char *tftp_path_prefix;

	tftp_server_ip = env_get("serverip");
	if (!tftp_server_ip) {
		printf("Error: TFTP server IP not set\n");
		return -1;
	}

	tftp_path_prefix = env_get("net_data_path");
	if (!tftp_path_prefix) {
		printf("Error: TFTP relative path not set\n");
		return -1;
	}

	sprintf(full_path, "%s%s", tftp_path_prefix, file_name);
	sprintf(cmd_buffer, "tftpboot %s %s:%s", load_addr, tftp_server_ip, full_path);

	if (run_command(cmd_buffer, 0) < 0) {
		printf("Error: TFTP download failed\n");
		return -1;
	}

	return 0;
}

static int load_from_device(struct cmd_tbl *cmdtp, char *load_str,
			int device_type, struct flash_dev *fdev)
{
	int retval = RESULT_OK;
	char blk_dev_str[10] = {"\0"};

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
		if (device_number < 0){
			printf("No USB storage devices found.\n");
			retval = RESULT_FAIL;
			break;
		}
		fdev->dev_str = strdup(simple_itoa((ulong)device_number));
		fdev->device_name = strdup("usb");

		char cmd[128];
		for (u32 p = 1; p <= MAX_SEARCH_PARTITIONS; p++){
			sprintf(cmd, "fatls usb %d:%d", device_number, p);
			if (!run_command(cmd, 0))
			{
				bootfs_part_index = p;
				break;
			}
		}

		if (bootfs_part_index == 0){
			printf("No valid filesystem found in any partition on USB.\n");
			retval = RESULT_FAIL;
			break;
		}
#else
		printf("USB storage support is not enabled.\n");
		retval = RESULT_FAIL;
#endif
		break;

#ifdef CONFIG_CMD_TFTPBOOT
	case DEVICE_NET:
		/* Initialize eMMC */
		if (check_mmc_exist_and_initialize() != RESULT_OK) {
			printf("Failed to initialize eMMC while handling NET.\n");
			retval = RESULT_FAIL;
			break;
		}
		fdev->dev_desc = blk_get_dev("mmc", dev_emmc_num);
		if (!fdev->dev_desc || fdev->dev_desc->type == DEV_TYPE_UNKNOWN) {
			printf("Failed to get eMMC device descriptor while handling USB.\n");
			retval = RESULT_FAIL;
			break;
		}
		if (run_command("dhcp", 0) < 0) {
			printf("Error: DHCP request failed\n");
			retval = RESULT_FAIL;
			break;
		}

		fdev->device_name = strdup("net");
		break;
#endif

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
	char *temp_fname = malloc(strlen(FLASH_CONFIG_FILE_NAME) + strlen(FLASH_IMG_FOLDER) + 2);
	if (!temp_fname){
		printf("malloc file_name fail\n");
		return RESULT_FAIL;
	}
	memset(temp_fname, '\0', strlen(FLASH_CONFIG_FILE_NAME) + strlen(FLASH_IMG_FOLDER) + 2);
	if (strlen(FLASH_IMG_FOLDER) > 0){
		strcpy(temp_fname, FLASH_IMG_FOLDER);
		strcat(temp_fname, "/");
		strcat(temp_fname, FLASH_CONFIG_FILE_NAME);
	}else{
		strcpy(temp_fname, FLASH_CONFIG_FILE_NAME);
	}

	if (strcmp(fdev->device_name, "mmc") == 0 || strcmp(fdev->device_name, "usb") == 0) {

		sprintf(blk_dev_str, "%s:%d", fdev->dev_str, bootfs_part_index);
		char *fat_argv[] = {"fatload", fdev->device_name, blk_dev_str, load_str, temp_fname};

		if (do_load(cmdtp, 0, 5, fat_argv, FS_TYPE_FAT)) {
			printf("do_load flash_config from %s failed\n", fdev->device_name);
			retval = RESULT_FAIL;
		} else {
			printf("do_load flash_config %s success\n", fdev->device_name);
		}
	} else if (strcmp(fdev->device_name, "net") == 0) {

		if (download_file_via_tftp(temp_fname, load_str) < 0) {
			printf("Failed to download file via TFTP\n");
			retval = RESULT_FAIL;
		} else {
			printf("Downloaded file via TFTP successfully\n");
		}
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
		/* do not retrun while flashing over! */
	}

}

int get_part_info(struct blk_desc *dev_desc, const char *name,
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

void specific_flash_mmc_opt(struct cmd_tbl *cmdtp, struct flash_dev *fdev)
{
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)
	char blk_dev_str[20] = {"\0"};
	char file_name[50] = {"\0"};
	u32 image_size = 0;
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
	sprintf(blk_dev_str, "%s:%d", fdev->dev_str, bootfs_part_index);

	/*flash emmc info to boot0*/
	fastboot_oem_flash_bootinfo(NULL, load_addr, image_size, NULL, fdev);

	/*load fsbl.bin to load_addr*/
	if (strlen(FLASH_IMG_FACTORY_FOLDER) > 0){
		strcpy(file_name, FLASH_IMG_FACTORY_FOLDER);
		strcat(file_name, "/");
		strcat(file_name, "FSBL.bin");
	}else{
		strcpy(file_name, "FSBL.bin");
	}

	struct blk_desc *dev_desc = blk_get_dev("mmc",
					   CONFIG_FASTBOOT_FLASH_MMC_DEV);

	if (strcmp(fdev->device_name, "net") == 0) {
		if (download_file_via_tftp(file_name, simple_xtoa((ulong)load_addr)) < 0) {
			printf("Failed to download file via TFTP\n");
			return;
		}
		image_size = env_get_hex("filesize", 0);
	} else {
		char *const argv_image[] = {"fatload", fdev->device_name, blk_dev_str, simple_xtoa((ulong)load_addr), file_name};

		if (do_load(cmdtp, 0, 5, argv_image, FS_TYPE_FAT)) {
			printf("Cannot load file %s\n", file_name);
			return;
		}
		image_size = env_get_hex("filesize", 0);
	}

	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid mmc device\n");
		return;
	}

	/*flash fsbl.bin to boot0*/
	if (flash_mmc_boot_op(dev_desc, load_addr, 1, image_size, BOOT_INFO_EMMC_SPL0_OFFSET)){
		printf("flash fsbl fail\n");
		return;
	}

	/*flash fsbl.bin to boot1*/
	if (flash_mmc_boot_op(dev_desc, load_addr, 2, image_size, BOOT_INFO_EMMC_SPL1_OFFSET)){
		printf("flash fsbl fail\n");
		return;
	}
#endif
}


static int flash_image(struct cmd_tbl *cmdtp, struct flash_dev *fdev)
{
	char load_str[13] = {"\0"};
	char addr_str[13] = {"\0"};
	char offset_str[13] = {"\0"};
	char blk_dev_str[10] = {"\0"};
	struct disk_partition info = {0};
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
	lbaint_t part_start_cnt;
	u32 image_size = 0;
	u32 byte_remain = 0;
	u32 div_times = 0;
	u32 download_bytes = 0;
	u32 download_offset = 0;
	u32 had_download = 0;
	u32 crc_value = 0;

	strcpy(load_str, simple_xtoa((ulong)load_addr));
	sprintf(blk_dev_str, "%s:%d", fdev->dev_str, bootfs_part_index);

	for (int i = 0; i < MAX_PARTITION_NUM; i++) {
		char *part_name = fdev->parts_info[i].part_name;
		char *file_name = fdev->parts_info[i].file_name;
		unsigned long time_start_flash = get_timer(0);
		download_offset = 0;
		crc_value = 0;

		if (fdev->parts_info[i].part_name == NULL || strlen(part_name) == 0) {
			printf("no more partition to flash\n");
			break;
		}

		if (fdev->parts_info[i].file_name == NULL || strlen(file_name) == 0) {
			/* if not file not exists, it mean not to flash */
			printf("file name is null, not to flashing, continue\n");
			continue;
		}
		printf("\n\nflash img %s, part_name:%s\n", file_name, part_name);

		if (strcmp(fdev->device_name, "mmc") == 0 || strcmp(fdev->device_name, "usb") == 0) {
			char *const argv_image[] = {"fatload", fdev->device_name, blk_dev_str, load_str, file_name};
			if (do_load(cmdtp, 0, 5, argv_image, FS_TYPE_FAT)) {
				printf("Failed to load file :%s, \n", file_name);
				return RESULT_FAIL;
			}

			char *const argv_image_size[] = {"fatsize", fdev->device_name, blk_dev_str, file_name};
			if (do_size(cmdtp, 0, 4, argv_image_size, FS_TYPE_FAT)) {
				printf("can not find file :%s, \n", file_name);
				return RESULT_FAIL;
			}

			image_size = env_get_hex("filesize", 0);
			byte_remain = image_size;
			div_times = (image_size + RECOVERY_LOAD_IMG_SIZE - 1) / RECOVERY_LOAD_IMG_SIZE;
			debug("\n\ndev_times:%d\n", div_times);

		} else if (strcmp(fdev->device_name, "net") == 0) {
			/*  Temporarily, the logic for network fragment download has not been added,
			so set the number of downloads to 1 and directly download the entire file. */
			div_times = 1;
		}

		if (get_part_info(fdev->dev_desc, part_name, &info) < 0) {
			printf("can not get part %s in gpt tabel\n", part_name);
			continue;
		}

		/* save the partition start cnt */
		part_start_cnt = info.start;
		for (int j = 0; j < div_times; j++) {
			debug("\ndownload and flash div %d\n", j);
			if (strcmp(fdev->device_name, "mmc") == 0 || strcmp(fdev->device_name, "usb") == 0) {
				download_bytes = byte_remain > RECOVERY_LOAD_IMG_SIZE ? RECOVERY_LOAD_IMG_SIZE : byte_remain;
				strcpy(addr_str, simple_xtoa((ulong)download_bytes));
				strcpy(offset_str, simple_xtoa((ulong)download_offset));

				char *const argv_image[] = {"fatload", fdev->device_name, blk_dev_str,
											load_str, file_name, addr_str, offset_str};
				printf("load from %x, bytes:%x\n", download_offset, download_bytes);
				if (do_load(cmdtp, 0, 7, argv_image, FS_TYPE_FAT))
					return RESULT_FAIL;

				had_download = env_get_hex("filesize", 0);
				printf("had_download:%d\n", had_download);
				if (had_download != download_bytes) {
					printf("download file size is not equal require\n");
					return RESULT_FAIL;
				}
			} else if (strcmp(fdev->device_name, "net") == 0) {
				if (download_file_via_tftp(file_name, load_str) < 0) {
					printf("Failed to download file via TFTP\n");
					return RESULT_FAIL;
				}
				image_size = download_bytes = env_get_hex("filesize", 0);
				had_download = download_bytes;
			}

			crc_value = crc32_wd(crc_value, (const uchar *)load_addr, had_download, CHUNKSZ_CRC32);
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
#ifdef CONFIG_FASTBOOT_FLASH_MMC
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

	/*set partition to env*/
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);

	/*update partition/mtd table to env*/
	if (_update_partinfo_to_env(load_addr, 0, fdev))
		return RESULT_FAIL;

	return RESULT_OK;
}

static int parse_flash_config(struct flash_dev *fdev)
{
#ifdef CONFIG_FASTBOOT_FLASH_MMC
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

	/*flash other image to specific offset*/
	specific_flash_mmc_opt(cmdtp, fdev);

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
