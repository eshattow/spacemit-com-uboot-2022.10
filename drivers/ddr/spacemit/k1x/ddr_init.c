// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Spacemit
 */

#include <common.h>
#include <cpu_func.h>
#include <dm.h>
#include <errno.h>
#include <div64.h>
#include <fdtdec.h>
#include <init.h>
#include <log.h>
#include <ram.h>
#include <asm/cache.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <linux/sizes.h>
#include <dt-bindings/soc/spacemit-k1x.h>
#ifdef CONFIG_K1_X_BOARD_FPGA
#include "ddr_init_fpga.h"
#else
#include "ddr_init_asic.h"
#endif

#define DDR_CHECK_SIZE			(0x4000)
#define DDR_CHECK_STEP			(0x2000)
#define DDR_CHECK_CNT			(0x1000)
#define TOP_DDR_NUM				1

static int test_pattern(fdt_addr_t base, fdt_size_t size)
{
	fdt_addr_t addr;
	fdt_size_t check_size;
	uint32_t offset;
	uint32_t *ddr_data = NULL;
	uint32_t *save_data;
	int err = 0;

	check_size = (DDR_CHECK_SIZE / DDR_CHECK_STEP) * DDR_CHECK_CNT;
	ddr_data = malloc(check_size);
	if (!ddr_data) {
		printf("test zone malloc fail size 0x%llx\n", check_size);
		return -1;
	}

	save_data = ddr_data;
	/* to avoid overlap important data as image or ramdump  */
	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			*save_data = readl((void*)addr + offset);
			save_data++;
		}
	}

	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			writel((uint32_t)(addr + offset), (void*)addr + offset);
		}
	}

	/* writeback and invalid cache */
	flush_dcache_range(base,base+size);
	invalidate_dcache_range(base,base+size);

	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			if (readl((void*)addr + offset) != (uint32_t)(addr + offset)) {
				printf("ddr check error %x vs %x\n", (uint32_t)(addr + offset), readl((void*)addr + offset));
				err++;
				if (err > 10)
					goto ERR_HANDLE;
			}
		}
	}

	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			writel((~(uint32_t)(addr + offset)), (void*)addr + offset);
		}
	}

	/* writeback and invalid cache */
	flush_dcache_range(base,base+size);
        invalidate_dcache_range(base,base+size);

	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			if (readl((void*)addr + offset) != (~(uint32_t)(addr + offset))) {
				printf("ddr check error %x vs %x\n", (uint32_t)(~(addr + offset)), readl((void*)addr + offset));
				err++;
				if (err > 10)
					goto ERR_HANDLE;

			}
		}
	}

ERR_HANDLE:
	save_data = ddr_data;
	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			writel(*save_data, (void*)addr + offset);
			save_data++;
		}
	}
	if (err != 0) {
		log_err("dram pattern test failed!\n");
	}

	free(ddr_data);

	return err;
}

static int spacemit_ddr_probe(struct udevice *dev)
{
	int ret;
	void (*ddr_init)(void);

#ifdef CONFIG_K1_X_BOARD_FPGA
	ddr_init = (void(*)(void))(lpddr4_init_fpga_data + 0x144);
	ddr_init();
#else
	uint32_t val;

	ddr_init = (void(*)(void))(lpddr4_init_asic_data + 0x462);
	ddr_init();

	val = readl(K1X_APMU_BASE + 0x00A0);
	val &= ~0x10;
	writel(val, K1X_APMU_BASE + 0x00A0);

	val = readl(K1X_APMU_BASE + 0x0098);
	val &= ~0x10;
	writel(val, K1X_APMU_BASE + 0x0098);
#endif

	ret = test_pattern(CONFIG_SYS_SDRAM_BASE, DDR_CHECK_SIZE);
	if (ret < 0) {
		log_err("dram init failed!\n");
		return -EIO;
	}
	log_debug("dram init done\n");

	return 0;
}

static const struct udevice_id spacemit_ddr_ids[] = {
	{ .compatible = "spacemit,ddr-ctl" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(spacemit_ddr) = {
	.name = "spacemit_ddr_ctrl",
	.id = UCLASS_RAM,
	.of_match = spacemit_ddr_ids,
	.probe = spacemit_ddr_probe,
};
