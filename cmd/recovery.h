// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023,  chris.huang<chris.huang@spacemit.com>
 */

#ifndef _SPACEMIT_RECOVERY_H
#define _SPACEMIT_RECOVERY_H

/*define max partition number*/
#define MAX_PARTITION_NUM (10)

enum from_dev_type{
	R_FROM_SD = 0,
	R_FROM_USB,
	R_FROM_NET,
};

enum to_dev_type{
	R_TO_SD = 0,
	R_TO_USB,
	R_TO_NET,
};

struct part_info {
	char part_name[16];
	char file_name[20];
	u32 crc;
	/*partition size info, such as 128MiB*/
	char size[10];
	bool flash;
};

struct gpt_info {
	char gpt_table[256];
	/*save gpt start info such as 4MiB*/
	char gpt_start[6];
	bool flash;
};

struct fsbl_info {
	u32 crc;
	/*save all fsbl offset string, such as '0x0;0x10000'*/
	char offset[36];
	bool flash;
};

struct flash_dev {
	u8 from_dev;
	u8 to_dev;
	struct part_info parts_info[MAX_PARTITION_NUM];
	struct gpt_info gptinfo;
	struct fsbl_info fsblinfo;
	struct disk_partition *d_info;
	struct blk_desc *dev_desc;

};

#endif /* _SPACEMIT_RECOVERY_H */
