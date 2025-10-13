// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025, Spacemit
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
#include <power/regulator.h>

bool is_video_connected = false;

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
#ifdef CONFIG_DM_REGULATOR_SPM8XX
	int ret;

	ret = regulators_enable_boot_on(true);
	if (ret)
		pr_debug("%s: Cannot enable boot on regulator\n", __func__);
#endif
	return 0;
}

int board_late_init(void)
{
	ulong kernel_start;
	ofnode chosen_node;
	int ret;

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

int misc_init_r(void)
{
	return 0;
}
extern u32 ddr_get_density(void);

int dram_init(void)
{
#if CONFIG_K3_BOARD_FPGA
	gd->ram_size = SZ_2GB - SEC_IMG_SIZE;
#else
	u64 dram_size = (u64)ddr_get_density() * SZ_1MB;
	gd->ram_size = dram_size;
#endif
	gd->ram_base = CONFIG_SYS_SDRAM_BASE;

	return 0;
}

int dram_init_banksize(void)
{
#if CONFIG_K3_BOARD_FPGA
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = SZ_2G - SEC_IMG_SIZE;
#else
	u64 dram_size = (u64)ddr_get_density() * SZ_1MB;

	memset(gd->bd->bi_dram, 0, sizeof(gd->bd->bi_dram));
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = dram_size;
#endif
	return 0;
}

ulong board_get_usable_ram_top(ulong total_size)
{
#if CONFIG_K3_BOARD_FPGA
	return 0x180000000;
#else
	u64 dram_size = (u64)ddr_get_density() * SZ_1MB;

		/* Some devices (like the EMAC) have a 32-bit DMA limit. */
	if(dram_size > SZ_2GB) {
		return 0x80000000;
	} else {
		return dram_size;
	}
#endif
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

/*May be not used*/
void set_boot_mode(enum board_boot_mode boot_mode)
{
	writel(boot_mode, (void *)BOOT_DEV_FLAG_REG);
}

/*
 * Boot mode strap pins (4 bits total)
 *
 * bit3 bit2 bit1-0  Meaning
 *  1     0     XX   Update from USB
 *  1     1     XX   Update from UART
 *  0     X     00   Normal boot from eMMC
 *  0     X     10   Normal boot from SPI Nand
 *  0     X     01   Normal boot from SPI Nor
 *  0     X     11   Normal boot from UFS
 */
enum board_boot_mode get_boot_pin_select(void)
{
    /* Decode full 4-bit strap field starting at BOOT_STRAP_BIT_OFFSET */
    u32 strap = readl((void *)BOOT_PIN_SELECT);
    u32 pins = (strap >> BOOT_STRAP_BIT_OFFSET) & 0xF; /* bit3:bit0 */
    pr_debug("strap_pins:%#x\n", pins);

    /* Update modes when bit3 == 1 */
    if (pins & 0x8) {
        if (pins & 0x4)
            return BOOT_MODE_UART; /* bit3=1, bit2=1 */
        else
            return BOOT_MODE_USB;  /* bit3=1, bit2=0 */
    }

    /* Normal boot by storage when bit3 == 0, select by bit1-0 */
    switch (pins & 0x3) {
    case BOOT_STRAP_BIT_EMMC: /* 00 */
        return BOOT_MODE_EMMC;
    case BOOT_STRAP_BIT_NAND: /* 10 */
        return BOOT_MODE_NAND;
    case BOOT_STRAP_BIT_NOR: /* 01 */
        return BOOT_MODE_NOR;
    case BOOT_STRAP_BIT_UFS: /* 11 */
        return BOOT_MODE_UFS;
    default:
        return BOOT_MODE_SD;
    }
}

/* Get boot mode based on bootrom implementation */
enum board_boot_mode get_boot_mode(void)
{
	u32 boot_flag_reg;
	enum board_boot_mode mode = BOOT_MODE_USB;

	/* Read bootrom boot flag from BOOT_DEV_FLAG_REG */
	boot_flag_reg = readl((void*)BOOT_DEV_FLAG_REG);
	pr_debug("%s boot_flag_reg:0x%x\n", __func__, boot_flag_reg);

	/* Check bootrom set boot type */
	u32 boot_type = boot_flag_reg & BOOT_TYPE_MASK;
	switch (boot_type) {
	case BOOT_MODE_USB:
		mode = BOOT_MODE_USB;
		break;
	case BOOT_MODE_UART:
		mode = BOOT_MODE_UART;
		break;
	case BOOT_MODE_EMMC:
		mode = BOOT_MODE_EMMC;
		break;
	case BOOT_MODE_NAND:
		mode = BOOT_MODE_NAND;
		break;
	case BOOT_MODE_NOR:
		mode = BOOT_MODE_NOR;
		break;
	case BOOT_MODE_UFS:
		mode = BOOT_MODE_UFS;
		break;
	case BOOT_MODE_SD:
		mode = BOOT_MODE_SD;
		break;
	default:
		mode = BOOT_MODE_SHELL;  /* Default to shell if unknown */
		break;
	}

	pr_debug("Final boot mode: 0x%x\n", mode);
	return mode;
}

void board_boot_order(u32* spl_boot_list)
{
	u32 boot_mode = get_boot_mode();
	pr_debug("boot_mode:0x%x\n", boot_mode);
	if (boot_mode == BOOT_MODE_USB) {
		spl_boot_list[0] = BOOT_DEVICE_BOARD;
	} else {
		switch (boot_mode) {
		case BOOT_MODE_SD:
			spl_boot_list[0] = BOOT_DEVICE_MMC1;
			break;
		case BOOT_MODE_EMMC:
			spl_boot_list[0] = BOOT_DEVICE_MMC2;
			break;
		case BOOT_MODE_NAND:
			spl_boot_list[0] = BOOT_DEVICE_NAND;
			break;
		case BOOT_MODE_NOR:
			spl_boot_list[0] = BOOT_DEVICE_NOR;
			break;
		case BOOT_MODE_UFS:
			spl_boot_list[0] = BOOT_DEVICE_UFS;
			break;
		default:
			spl_boot_list[0] = BOOT_DEVICE_RAM;
			break;
		}

		// reserve for debug/test to load/run uboot from ram.
		spl_boot_list[1] = BOOT_DEVICE_RAM;
	}
}
