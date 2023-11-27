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
#include <mtd.h>
#include <spl.h>
#include <linux/io.h>

#define EMMC_MAX_BLK_WRITE 16384

static int _write_gpt_partition(char gpt_table[256], char *response)
{
	__maybe_unused char write_part_command[300] = {"\0"};

#ifdef CONFIG_FASTBOOT_FLASH_MMC
		sprintf(write_part_command, "gpt write mmc %x '%s'",
			CONFIG_FASTBOOT_FLASH_MMC_DEV, gpt_table);
		if (run_command(write_part_command, 0)){
			fastboot_fail("write gpt fail", response);
			return -1;
		}
#endif

#ifdef CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV
		printf("mtd write gpt to dev:%s\n", CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME);
		sprintf(write_part_command, "gpt write %s %x '%s'",
			CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME, CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NUM,
			gpt_table);
		if (run_command(write_part_command, 0)){
			fastboot_fail("write gpt fail", response);
			return -1;
		}
#endif
	fastboot_okay("parse gpt/mtd table okay", response);
	return 0;
}

static int _write_mtd_partition(char mtd_table[128], char *response)
{
#ifdef CONFIG_MTD
	struct mtd_info *mtd;
	char mtd_ids[36] = {"\0"};
	char mtd_parts[128] = {"\0"};

	mtd_probe_devices();

	/*
	try to find the first mtd device, it there have mutil mtd device such as nand and nor,
	it only use the first one.
	*/
	mtd_for_each_device(mtd) {
		if (!mtd_is_partition(mtd))
			break;
	}

	if (mtd == NULL){
		fastboot_fail("can not get mtd device", response);
		return -1;
	}

	/*to mtd device, it should write mtd table to env.*/
	sprintf(mtd_ids, "%s=spi-dev", mtd->name);
	sprintf(mtd_parts, "spi-dev:%s", mtd_table);

	env_set("mtdids", mtd_ids);
	env_set("mtdparts", mtd_parts);

	fastboot_okay("parse gpt/mtd table okay", response);
#endif
	return 0;
}

/**
 * @brief transfer the string of size 'K' or 'M' to u32 type.
 * 
 * @param reserve_size , the string of size
 * @return int , return the transfer result.
 */
int transfer_string_to_ul(const char *reserve_size)
{
	char *ret, *token;
	char ch[2];
	char strnum[10] = {"\0"};
	u32 get_size = 0;
	
	if (reserve_size == NULL || strlen(reserve_size) == 0)
		return 0;

	if (!strncmp("-", reserve_size, 1)){
		return 0;
	}

	ret = strpbrk(reserve_size, "KMG");
	strncpy(ch, ret, 1);
	if (ch[0] == 'K' || ch[0] == 'M' || ch[0] == 'G'){
		strcpy(strnum, reserve_size);
		token = strtok(strnum, ch);
		get_size = simple_strtoul(token, NULL, 0);
	}else{
		printf("not support size %s, should use K/M/G\n", reserve_size);
		return 0;
	}

	switch(ch[0]){
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
 * @brief parse the flash_config and save partition info
 * 
 * @param fdev , struct flash_dev
 * @return u32 , return 0 if parse config success.
 */
int _parse_flash_config(struct flash_dev *fdev, void *load_flash_addr)
{
	u32 part_index = 0;

	bool parse_mtd_partition = false;

	cJSON *json_root;

	int result = 0;
	char *combine_str = NULL;
	int combine_len = 1;
	int combine_size = 0;
	int combine_len_extra = 0;

	/*init and would remalloc while size is increasing*/
	combine_str = malloc(combine_len);
	memset(combine_str, '\0', combine_len);

	json_root = cJSON_Parse(load_flash_addr);
	if (!json_root){
		printf("can not parse json, check your flash_config.cfg is json format or not\n");
		return -1;
	}

	/*judge if parse mtd or gpt partition*/
	cJSON *cj_format = cJSON_GetObjectItem(json_root, "format");
	if (cj_format && cj_format->type == cJSON_String){
		if (!strncmp("gpt", cj_format->valuestring, 3)){
			fdev->gptinfo.fastboot_flash_gpt = true;
			combine_len_extra = 20;
		}else if(!strncmp("mtd", cj_format->valuestring, 3)){
			parse_mtd_partition = true;
			combine_len_extra = 6;
		}
	}

	cJSON *cj_parts = cJSON_GetObjectItem(json_root, "partitions");
	if (cj_parts && cj_parts->type == cJSON_Array){
		for(int i = 0; i < cJSON_GetArraySize(cj_parts); i++){
			const char *node_part = NULL;
			const char *node_file = NULL;
			const char *node_offset = NULL;
			const char *node_size = NULL;

			cJSON *arraypart = cJSON_GetArrayItem(cj_parts, i);
			cJSON *cj_name = cJSON_GetObjectItem(arraypart, "name");
			if (cj_name && cj_name->type == cJSON_String)
				node_part = cj_name->valuestring;
			else
				node_part = "";

			cJSON *cj_filename = cJSON_GetObjectItem(arraypart, "image");
			if (cj_filename && cj_filename->type == cJSON_String)
				node_file = cj_filename->valuestring;
			else
				node_file = "";

			cJSON *cj_offset = cJSON_GetObjectItem(arraypart, "offset");
			if (cj_offset && cj_offset->type == cJSON_String)
				node_offset = cj_offset->valuestring;
			else
				node_offset = "";

			cJSON *cj_size = cJSON_GetObjectItem(arraypart, "size");
			if (cj_size && cj_size->type == cJSON_String)
				node_size = cj_size->valuestring;
			else
				node_size = "";

			/*make sure that offset would not over than previous size and offset*/
			int off = transfer_string_to_ul(node_offset);
			if (off > 0 && off < combine_size){
				printf("offset must larger then previous size and offset\n");
				return -5;
			}

			combine_len += strlen(node_part) + strlen(node_offset) + strlen(node_size) + combine_len_extra;
			printf("combine_len:%d\n", combine_len);
			combine_str = realloc(combine_str, combine_len);
			if (combine_str == NULL){
				printf("realloc combine_str fail\n");
				return -1;
			}
			combine_size += off;

			if (parse_mtd_partition){
				/*parse mtd partition*/
				if (strlen(combine_str) == 0)
					sprintf(combine_str, "%s%s@%dK(%s)", combine_str, node_size, combine_size, node_part);
				else
					sprintf(combine_str, "%s,%s@%dK(%s)", combine_str, node_size, combine_size, node_part);
			}else if (fdev->gptinfo.fastboot_flash_gpt){
				/*parse gpt partition*/
				if (strlen(node_offset) == 0)
					sprintf(combine_str, "%sname=%s,size=%s;", combine_str, node_part, node_size);
				else
					sprintf(combine_str, "%sname=%s,start=%s,size=%s;", combine_str, node_part, node_offset, node_size);
			}
			combine_size += transfer_string_to_ul(node_size);

			/*after finish recovery, it would free the malloc paramenter at func recovery_show_result*/
			fdev->parts_info[part_index].part_name = malloc(strlen(node_part));
			if (!fdev->parts_info[part_index].part_name){
				printf("malloc part_name fail\n");
				result = RESULT_FAIL;
				goto free_cjson;
			}
			strcpy(fdev->parts_info[part_index].part_name, node_part);

			fdev->parts_info[part_index].size = malloc(strlen(node_size) + 0x2000);
			if (!fdev->parts_info[part_index].size){
				printf("malloc size fail\n");
				result = RESULT_FAIL;
				goto free_cjson;
			}
			strcpy(fdev->parts_info[part_index].size, node_size);

			if (node_file == NULL){
				printf("not set file name, set to null\n");
				fdev->parts_info[part_index].file_name = NULL;
			}else{
				fdev->parts_info[part_index].file_name = malloc(strlen(node_file) + strlen(RECOVERY_FOLDER) + 2);
				if (!fdev->parts_info[part_index].file_name){
					printf("malloc file_name fail\n");
					result = RESULT_FAIL;
					goto free_cjson;
				}
				strcpy(fdev->parts_info[part_index].file_name, RECOVERY_FOLDER);
				strcat(fdev->parts_info[part_index].file_name, "/");
				strcat(fdev->parts_info[part_index].file_name, node_file);
			}

			printf("part info %s, %s\n", \
				fdev->parts_info[part_index].part_name, \
				fdev->parts_info[part_index].file_name);
			part_index++;
		}
	}else{
		printf("do not get partition info, check the input file\n");
		return -1;
	}
	if (parse_mtd_partition){
		fdev->mtd_table = realloc(fdev->mtd_table, combine_len);
		strcpy(fdev->mtd_table, combine_str);
	}
	else{
		fdev->gptinfo.gpt_table = realloc(fdev->gptinfo.gpt_table, combine_len);
		strcpy(fdev->gptinfo.gpt_table, combine_str);
	}
free_cjson:
	cJSON_free(json_root);
	free(combine_str);
	return result;
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
							char *response, struct flash_dev *fdev)
{

	int ret = 0;

	ret = _parse_flash_config(fdev, (void *)fastboot_buf_addr);
	if (ret){
		if (ret == -1)
			printf("parsing config fail\n");
		if (ret == -5)
			fastboot_fail("offset must larger then previous size and offset", response);
		return;
	}

	if (strlen(fdev->gptinfo.gpt_table) > 0 && fdev->gptinfo.fastboot_flash_gpt){
		_write_gpt_partition(fdev->gptinfo.gpt_table, response);
	}


	if (strlen(fdev->mtd_table) > 0)
		_write_mtd_partition(fdev->mtd_table, response);

	/*maybe there doesn't have gpt/mtd partition, should not return fail*/
	fastboot_okay("parse gpt/mtd table okay", response);
	return;
}

/**
 * @brief flash env to reserve partition.
 * 
 * @param cmd env
 * @param download_buffer load env.bin to addr
 * @param download_bytes env.bin size
 * @param response 
 * @param fdev 
 */
void fastboot_oem_flash_env(const char *cmd, void *download_buffer, u32 download_bytes,
							char *response, struct flash_dev *fdev)
{

	char cmdbuf[32];
	char *gpt_table_str = NULL;
	char *mtd_table_str = NULL;
	u32 boot_pin = readl((void *)BOOT_PIN_SELECT);
	memset(cmdbuf, '\0', 32);

	sprintf(cmdbuf, "env import -d -c 0x%lx 0x%lx", (ulong)download_buffer, (ulong)CONFIG_ENV_SIZE);
	if (run_command(cmdbuf, 0)){
		fastboot_fail("Cannot import env.bin", response);
		return;
	}

	if (strlen(fdev->mtd_table) > 0){
		mtd_table_str = malloc(strlen(fdev->mtd_table)+32);

		if (mtd_table_str == NULL)
			return;
		sprintf(mtd_table_str, "env set -f mtdparts 'spi-dev:%s'", fdev->mtd_table);

		run_command(mtd_table_str, 0);
		run_command("env set -f mtdids 'nor0=spi-dev'", 0);
		/*
		TODO: 
			add mtdparts/mtdids, mtdparts=spi-dev:xxx, mtdids=nor0=spi-dev.
			need to get nor/nand name.
		*/
	}

	if (strlen(fdev->gptinfo.gpt_table) > 0){
		gpt_table_str = malloc(strlen(fdev->gptinfo.gpt_table)+32);
		if (gpt_table_str == NULL)
			return;
		sprintf(gpt_table_str, "env set -f partitions '%s'", fdev->gptinfo.gpt_table);
		run_command(gpt_table_str, 0);
	}

	memset(cmdbuf, '\0', 32);
	sprintf(cmdbuf, "env export -c -s 0x%lx 0x%lx", (ulong)CONFIG_ENV_SIZE, (ulong)download_buffer);
	if (run_command(cmdbuf, 0)){
		fastboot_fail("Cannot import env.bin", response);
		return;
	}

	switch(boot_pin){
	case 0:
	case 3:
		/*write to emmc default offset*/
		printf("write to mmc offset:%lx\n", (ulong)FLASH_ENV_OFFSET_MMC);
		fastboot_mmc_flash_offset((u32)FLASH_ENV_OFFSET_MMC, download_buffer, (u32)CONFIG_ENV_SIZE);
		break;
	case 2:
		/*write to nor offset*/
		break;
	case 1:
		/*write to nand offset*/
		break;
	default:
		break;
	}

	fastboot_okay("flash env partition okay", response);
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
static __maybe_unused lbaint_t fb_mmc_blk_write(struct blk_desc *block_dev, lbaint_t start,
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
 * fastboot_mmc_flash_offset() - Write fsbl image to eMMC
 *
 * @start_offset: start offset to write.
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 */
int fastboot_mmc_flash_offset(u32 start_offset, void *download_buffer,
                             u32 download_bytes)
{
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC)
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

	printf("........ wrote 0x%lx sector bytes to blk offset 0x%lx\n", blkcnt, info.start);
#endif
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
