// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#include <common.h>
#include <dm.h>
#include <dm/ofnode.h>
#include <env.h>
#include <fdtdec.h>
#include <image.h>
#include <log.h>
#include <mapmem.h>
#include <spl.h>
#include <init.h>
#include <virtio_types.h>
#include <virtio.h>
#include <asm/io.h>
#include <asm/sections.h>
#include <stdlib.h>
#include <linux/io.h>
#include <asm/global_data.h>
#include <part.h>

DECLARE_GLOBAL_DATA_PTR;

void run_fastboot_command(void)
{
	u32 read_reg = readl((void *)BOOT_DEV_FLAG_REG);
	if (read_reg == USB_DOWNLOAD_FLAG){
		char *cmd_para = "fastboot 0";
		run_command(cmd_para, 0);
	}
}

void import_env_from_bootfs(void)
{
	/*
	TODO:
		load env from bootfs, if bootfs is fat/ext4 at blk dev, use fatload/ext4load.
	*/
	int err, dev;
	u32 part;
	char cmd[128];
	static struct mmc *mmc;
	struct disk_partition info;

	dev = mmc_get_env_dev();
	mmc = find_mmc_device(dev);
	if (mmc){
		if (mmc_init(mmc)){
			return;
		}
	}

	for (u32 p = 1; p <= MAX_SEARCH_PARTITIONS; p++) {
		err = part_get_info(mmc_get_blk_desc(mmc), p, &info);
		if (err)
			continue;
		if (!strcmp(BOOTFS_NAME, info.name)){
			debug("match info.name:%s\n", info.name);
			part = p;
			break;
		}
	}
	env_set("bootfs_part", simple_itoa(part));

	/*load env.txt and import to uboot*/
	sprintf(cmd, "fatload mmc %d:%d 0x%x env_%s.txt",
			dev, part, CONFIG_SPL_LOAD_FIT_ADDRESS, CONFIG_SYS_CONFIG_NAME);
	debug("cmd:%s\n", cmd);
	if (run_command(cmd, 0))
		return;

	memset(cmd, '\0', 128);
	sprintf(cmd, "env import -t 0x%x", CONFIG_SPL_LOAD_FIT_ADDRESS);
	debug("cmd:%s\n", cmd);
	if (!run_command(cmd, 0))
		printf("load env%s.txt from bootfs successful\n", CONFIG_SYS_CONFIG_NAME);
	return;
}

void run_cardfirmware_flash_command(void)
{
	/*
	TODO:
		try to find partition in sd card, if it has partition name 'flashing_image',
		it would try to excute command 'spacemit_flashing', and it would flash image
		to emmc/nor/nand.
	*/
	return;
}

void setenv_boot_mode(void)
{
	u32 read_reg = readl((void *)BOOT_DEV_FLAG_REG);
	switch (read_reg) {
	case SPL_BOOT_MODE_NAND:
		env_set("boot_device", "nand");
		break;
	case SPL_BOOT_MODE_NOR:
		env_set("boot_device", "nor");
		break;
	case SPL_BOOT_MODE_EMMC:
		env_set("boot_device", "mmc");
		env_set("boot_devnum", "1");
		break;
	case SPL_BOOT_MODE_SD:
		env_set("boot_device", "mmc");
		env_set("boot_devnum", "0");
		break;
	default:
		env_set("boot_device", "");
		break;
	}
}

int board_init(void)
{
	return 0;
}

int board_late_init(void)
{
	ulong kernel_start;
	ofnode chosen_node;
	int ret;

	run_fastboot_command();

	/*import env.txt from bootfs*/
	import_env_from_bootfs();

	setenv_boot_mode();

	chosen_node = ofnode_path("/chosen");
	if (!ofnode_valid(chosen_node)) {
		debug("No chosen node found, can't get kernel start address\n");
		return 0;
	}

	ret = ofnode_read_u64(chosen_node, "riscv,kernel-start",
			      (u64 *)&kernel_start);
	if (ret) {
		debug("Can't find kernel start address in device tree\n");
		return 0;
	}

	env_set_hex("kernel_start", kernel_start);

	return 0;
}

void *board_fdt_blob_setup(int *err)
{
	*err = 0;

	/* Stored the DTB address there during our init */
	if (IS_ENABLED(CONFIG_OF_SEPARATE) || IS_ENABLED(CONFIG_OF_BOARD)) {
		if (gd->arch.firmware_fdt_addr){
			if (!fdt_check_header((void *)(ulong)gd->arch.firmware_fdt_addr)){
				return (void *)(ulong)gd->arch.firmware_fdt_addr;
			}
		}
	}
	return (ulong *)&_end;
}


void board_boot_order(u32 *spl_boot_list)
{
	u32 sec_boot = spl_boot_device();

	u32 read_reg = readl((void *)BOOT_DEV_FLAG_REG);
	printf("read_reg:%x\n", read_reg);
	if (read_reg == USB_DOWNLOAD_FLAG){
		spl_boot_list[0] = BOOT_DEVICE_BOARD;
	}
	else if (sec_boot == BOOT_DEVICE_RAM){
		spl_boot_list[0] = BOOT_DEVICE_RAM;
	}else{
		spl_boot_list[0] = BOOT_DEVICE_MMC1;
		if (sec_boot != BOOT_DEVICE_MMC1){
			spl_boot_list[1] = sec_boot;

			//reserve for fpga to load/run uboot from ram.
			spl_boot_list[2] = BOOT_DEVICE_RAM;
		}else{
			//reserve for fpga to load/run uboot from ram.
			spl_boot_list[1] = BOOT_DEVICE_RAM;
		}
	}
}
