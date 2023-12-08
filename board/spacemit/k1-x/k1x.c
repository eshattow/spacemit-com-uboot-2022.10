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
#include <env.h>
#include <env_internal.h>
#include <asm/arch/ddr.h>
#include <power/regulator.h>
#include <fb_spacemit.h>

DECLARE_GLOBAL_DATA_PTR;

void set_boot_mode(enum board_boot_mode boot_mode)
{
	writel(boot_mode, (void *)BOOT_DEV_FLAG_REG);
}

enum board_boot_mode get_boot_pin_select(void)
{
	/*if not set boot mode, try to return boot pin select*/
	u32 boot_select = readl((void *)BOOT_PIN_SELECT) & BOOT_STRAP_BIT_STORAGE_MASK;
	boot_select = boot_select >> BOOT_STRAP_BIT_OFFSET;
	debug("boot_select:%x\n", boot_select);

	/*select spl boot device:
 
	     b'(bit1)(bit0)
	emmc:b'00, //BOOT_STRAP_BIT_EMMC
	nor :b'10, //BOOT_STRAP_BIT_NOR
	nand:b'01, //BOOT_STRAP_BIT_NAND
	sd  :b'11, //BOOT_STRAP_BIT_SD
*/
	switch (boot_select) {
	case BOOT_STRAP_BIT_EMMC:
		return BOOT_MODE_EMMC;
	case BOOT_STRAP_BIT_NAND:
		return BOOT_MODE_NAND;
	case BOOT_STRAP_BIT_NOR:
		return BOOT_MODE_NOR;
	case BOOT_STRAP_BIT_SD:
	default:
		return BOOT_MODE_SD;
	}
}

enum board_boot_mode get_boot_mode(void)
{
	/*if usb boot or has set boot mode, return boot mode*/
	u32 boot_mode = readl((void *)BOOT_DEV_FLAG_REG);
	debug("%s, boot_mode:%x\n", __func__, boot_mode);

	switch (boot_mode) {
	case BOOT_MODE_USB:
		return BOOT_MODE_USB;
	case BOOT_MODE_EMMC:
		return BOOT_MODE_EMMC;
	case BOOT_MODE_NAND:
		return BOOT_MODE_NAND;
	case BOOT_MODE_NOR:
		return BOOT_MODE_NOR;
	case BOOT_MODE_SD:
		return BOOT_MODE_SD;
	}

	/*else return boot pin select*/
	return get_boot_pin_select();
}


int mmc_get_env_dev(void)
{
	u32 boot_mode = 0;
	boot_mode = get_boot_mode();
	debug("%s, uboot boot_mode:%x\n", __func__, boot_mode);

	if (boot_mode == BOOT_MODE_EMMC)
		return MMC_DEV_EMMC;
	else
		return MMC_DEV_SD;
}


void run_fastboot_command(void)
{
	u32 boot_mode = get_boot_mode();
	if (boot_mode == BOOT_MODE_USB){
		char *cmd_para = "fastboot 0";
		run_command(cmd_para, 0);
	}
}

void import_env_from_bootfs(void)
{
#ifdef CONFIG_MMC
	/*
	TODO:
		load env from bootfs, if bootfs is fat/ext4 at blk dev, use fatload/ext4load.
	*/
	int err, dev;
	u32 part;
	char cmd[128];
	struct mmc *mmc;
	struct disk_partition info;

	dev = mmc_get_env_dev();
	mmc = find_mmc_device(dev);
	if (!mmc) {
		printf("Cannot find mmc device\n");
		return;
	}
	if (mmc_init(mmc)){
		return;
	}

	for (part = 1; part <= MAX_SEARCH_PARTITIONS; part++) {
		err = part_get_info(mmc_get_blk_desc(mmc), part, &info);
		if (err)
			continue;
		if (!strcmp(BOOTFS_NAME, info.name)){
			debug("match info.name:%s\n", info.name);
			break;
		}
	}
	if (part > MAX_SEARCH_PARTITIONS)
		return;

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
#endif
	return;
}

void run_cardfirmware_flash_command(void)
{
	struct mmc *mmc;
	struct disk_partition info;
	int part_dev, err;
	char cmd[128] = {"\0"};

	mmc = find_mmc_device(MMC_DEV_SD);
	if (!mmc)
		return;
	if (mmc_init(mmc))
		return;

	for (part_dev = 1; part_dev <= MAX_SEARCH_PARTITIONS; part_dev++) {
		err = part_get_info(mmc_get_blk_desc(mmc), part_dev, &info);
		if (err)
			continue;
		if (!strcmp(BOOTFS_NAME, info.name))
			break;

	}

	if (part_dev > MAX_SEARCH_PARTITIONS)
		return;

	/*check json file exist or not in sd card*/
	sprintf(cmd, "fatsize mmc %d:%d %s", MMC_DEV_SD, part_dev, CARD_FLASH_FILE);
	debug("cmd:%s\n", cmd);
	if (!run_command(cmd, 0))
		run_command("spacemit_flashing mmc", 0);

	return;
}

void setenv_boot_mode(void)
{
	u32 boot_mode = get_boot_mode();
	switch (boot_mode) {
	case BOOT_MODE_NAND:
		env_set("boot_device", "nand");
		break;
	case BOOT_MODE_NOR:
		env_set("boot_device", "nor");
		break;
	case BOOT_MODE_EMMC:
		env_set("boot_device", "mmc");
		env_set("boot_devnum", simple_itoa(MMC_DEV_EMMC));
		break;
	case BOOT_MODE_SD:
		env_set("boot_device", "mmc");
		env_set("boot_devnum", simple_itoa(MMC_DEV_SD));
		break;
	default:
		env_set("boot_device", "");
		break;
	}
}

int board_init(void)
{
#ifdef CONFIG_DM_REGULATOR
	int ret;

	ret = regulators_enable_boot_on(false);
	if (ret)
		debug("%s: Cannot enable boot on regulator\n", __func__);
#endif
	return 0;
}

int board_late_init(void)
{
	ulong kernel_start;
	ofnode chosen_node;
	int ret;

	run_fastboot_command();

	run_cardfirmware_flash_command();

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
	u32 boot_mode = get_boot_mode();
	debug("boot_mode:%x\n", boot_mode);
	if (boot_mode == BOOT_MODE_USB){
		spl_boot_list[0] = BOOT_DEVICE_BOARD;
	}else{
		spl_boot_list[0] = BOOT_DEVICE_MMC1;
		if (boot_mode != BOOT_MODE_SD){
			switch (boot_mode) {
			case BOOT_MODE_EMMC:
				spl_boot_list[1] = BOOT_DEVICE_MMC2;
				break;
			case BOOT_MODE_NAND:
				spl_boot_list[1] = BOOT_DEVICE_NAND;
				break;
			case BOOT_MODE_NOR:
				spl_boot_list[1] = BOOT_DEVICE_NOR;
				break;
			default:
				spl_boot_list[1] = BOOT_DEVICE_RAM;
				break;
			}

			//reserve for fpga to load/run uboot from ram.
			spl_boot_list[2] = BOOT_DEVICE_RAM;
		}else{
			//reserve for fpga to load/run uboot from ram.
			spl_boot_list[1] = BOOT_DEVICE_RAM;
		}
	}
}


enum env_location env_get_location(enum env_operation op, int prio)
{
	if (prio >= 1)
		return ENVL_UNKNOWN;

	u32 boot_mode = get_boot_mode();
	switch (boot_mode) {
	case BOOT_MODE_NAND:
		return ENVL_NAND;
	case BOOT_MODE_NOR:
		return ENVL_SPI_FLASH;
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
	default:
		return ENVL_MMC;
	}
}

int misc_init_r(void)
{
#ifdef CONFIG_DYNAMIC_DDR_CLK_FREQ
	int ret;

	ret = ddr_freq_max();
	if(ret < 0) {
		debug("%s: Try to adjust ddr freq failed!\n", __func__);
		return ret;
	}
#endif

	return 0;
}
