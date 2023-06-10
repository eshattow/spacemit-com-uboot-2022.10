// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023, kevin.z.m <zhangmeng.kevin@spacemit.com>
 */

#include <common.h>
#include <dm.h>
#include <dm/ofnode.h>
#include <env.h>
#include <fdtdec.h>
#include <image.h>
#include <log.h>
#include <spl.h>
#include <init.h>
#include <virtio_types.h>
#include <virtio.h>
#include <asm/io.h>

#define SYS_GMAC_CFG  0x2f028004

DECLARE_GLOBAL_DATA_PTR;
void k1pro_gmac_init(void);

int board_init(void)
{
	return 0;
}

int board_late_init(void)
{
	ulong kernel_start;
	ofnode chosen_node;
	int ret;

    k1pro_gmac_init();
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

#ifdef CONFIG_SPL
u32 spl_boot_device(void)
{
	/* RISC-V QEMU only supports RAM as SPL boot device */
	return BOOT_DEVICE_RAM;
}
#endif

void *board_fdt_blob_setup(int *err)
{
	*err = 0;
	/* Stored the DTB address there during our init */
	return (void *)(ulong)gd->arch.firmware_fdt_addr;
}

void k1pro_gmac_init(void)
{
	volatile unsigned int val;

    //enable rmii
    val = readl(SYS_GMAC_CFG);
	val |= BIT(0);
	writel(val, SYS_GMAC_CFG);

    //set software rst
	writel(0xffffffff, 0x2f024600);
}