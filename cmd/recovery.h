// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023,  chris.huang<chris.huang@spacemit.com>
 */

#ifndef _SPACEMIT_RECOVERY_H
#define _SPACEMIT_RECOVERY_H

DECLARE_GLOBAL_DATA_PTR;

/*define max partition number*/
#define MAX_PARTITION_NUM (10)

#define MAX_BLK_WRITE (16384)
#define RESULT_OK (0)
#define RESULT_FAIL (1)

/*recovery folder name*/
#define RECOVERY_FOLDER "recovery"
#define FLASH_CONFIG_NAME "flash_config.cfg"

typedef enum{
	DEVICE_MMC,
	DEVICE_USB,
	DEVICE_NET,
} DeviceType;

struct part_info
{
	char *part_name;
	char *file_name;
	uint32_t crc;
	/*partition size info, such as 128MiB*/
	char *size;
	/*use for fsbl, if hidden that gpt would reserve a raw memeory
	  for fsbl and the partition is not available.
	*/
	bool hidden;
};

struct gpt_info {
	char gpt_table[256];
	/*save gpt start offset*/
	u32 gpt_start_offset;
};

struct flash_dev {
	char *device_name;
	char *dev_str;
	struct part_info parts_info[MAX_PARTITION_NUM];
	struct gpt_info gptinfo;
	struct disk_partition *d_info;
	struct blk_desc *dev_desc;
};

#endif /* _SPACEMIT_RECOVERY_H */
