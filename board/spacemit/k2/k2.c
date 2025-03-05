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

