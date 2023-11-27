/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2023,  chris.huang<chris.huang@spacemit.com>
 */

#ifndef _FB_SPACEMIT_H_
#define _FB_SPACEMIT_H_

// DECLARE_GLOBAL_DATA_PTR;

/*define max partition number*/
#define MAX_PARTITION_NUM (20)

#define MAX_BLK_WRITE (16384)
#define RESULT_OK (0)
#define RESULT_FAIL (1)

/*recovery folder name*/
#define RECOVERY_FOLDER ("recovery")
#define FLASH_CONFIG_NAME ("flash_config.cfg")

#define FLASH_FSBL0_OFFSET (0x20000)
#define FLASH_FSBL1_OFFSET (0x60000)

#define FLASH_ENV_OFFSET_MMC (0x50000-0x8000)
#define FLASH_ENV_OFFSET_NOR (0x30000-0x8000)
#define FLASH_ENV_OFFSET_NAND (0x40000)

typedef enum{
	DEVICE_MMC,
	DEVICE_USB,
	DEVICE_NET,
} DeviceType;

struct recovery_parts_info
{
	char *part_name;
	char *file_name;
	/*partition size info, such as 128MiB*/
	char *size;
	/*use for fsbl, if hidden that gpt would reserve a raw memeory
	  for fsbl and the partition is not available.
	*/
	bool hidden;
};

struct gpt_info {
	char *gpt_table;
	/*save gpt start offset*/
	u32 gpt_start_offset;
	bool fastboot_flash_gpt;
};

struct flash_dev {
	char *device_name;
	char *dev_str;
	struct recovery_parts_info parts_info[MAX_PARTITION_NUM];
	struct gpt_info gptinfo;
	struct disk_partition *d_info;
	struct blk_desc *dev_desc;
	char *mtd_table;
};

/**
 * fastboot_oem_flash_gpt() - parse flash config and write gpt table.
 *
 * @cmd: Named partition to write image to
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 * @response: Pointer to fastboot response buffer
 */
void fastboot_oem_flash_gpt(const char *cmd, void *download_buffer, u32 download_bytes,
			char *response, struct flash_dev *fdev);

/**
 * fastboot_mmc_flash_offset() - Write fsbl image to eMMC
 *
 * @start_offset: start offset to write.
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 */
int fastboot_mmc_flash_offset(u32 start_offset, void *download_buffer, u32 download_bytes);


/**
 * @brief check image crc at emmc. if crc is same it would return RESULT_OK(0).
 *
 * @param dev_desc struct blk_desc.
 * @param crc_compare need to be compare crc.
 * @param part_start_cnt read from emmc offset.
 * @param blksz normally is 0x200.
 * @param image_size
 * @return int
 */
int check_mmc_image_crc(struct blk_desc *dev_desc, ulong crc_compare, lbaint_t part_start_cnt,
			ulong blksz, int image_size);

/**
 * @brief transfer the string of size 'KiB' or 'MiB' to u32 type.
 *
 * @param reserve_size , the string of size 'xiB'
 * @return int , return the transfer result.
 */
int transfer_string_to_ul(const char *reserve_size);


/**
 * @brief parse the flash_config and save partition info
 *
 * @param fdev , struct flash_dev
 * @return int , return 0 if parse config success.
 */
int _parse_flash_config(struct flash_dev *fdev, void *load_flash_addr);


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
			char *response, struct flash_dev *fdev);


#endif
