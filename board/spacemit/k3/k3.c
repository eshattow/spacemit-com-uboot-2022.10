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


DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
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

