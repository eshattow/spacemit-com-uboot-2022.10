// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Spacemit
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include "k3_ddr.h"

#define DDR_CHECK_SIZE			(0x4000)
#define DDR_CHECK_STEP			(0x2000)
#define DDR_CHECK_CNT			(0x1000)

static int test_pattern(fdt_addr_t base, fdt_size_t size)
{
	fdt_addr_t addr;
	fdt_size_t check_size;
	uint32_t offset;
	uint32_t* ddr_data = NULL;
	uint32_t* save_data;
	int err;

	err = 0;

	check_size = (DDR_CHECK_SIZE / DDR_CHECK_STEP) * DDR_CHECK_CNT;
	ddr_data = malloc(check_size);
	if (!ddr_data) {
		pr_err("test zone malloc fail size 0x%llx\n", check_size);
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
	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			if (readl((void*)addr + offset) != (uint32_t)(addr + offset)) {
				pr_err("ddr check error %x vs %x\n", (uint32_t)(addr + offset), readl((void*)addr + offset));
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
	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			if (readl((void*)addr + offset) != (~(uint32_t)(addr + offset))) {
				pr_err("ddr check error %x vs %x\n", (uint32_t)(~(addr + offset)), readl((void*)addr + offset));
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
	if (err == 0)
		printf("memory verify pass\n");
	else
		pr_err("memory verify fail!\n");

	free(ddr_data);

	return err;
}

static int spacemit_ddr_probe(struct udevice *dev)
{
	int ret;
#ifdef CONFIG_K3_BOARD_FPGA
	fpga_ddr_init();
#else
	uint64_t ddrc0, ddrc1;

	ddrc0 = dev_read_addr_index(dev, 0);
	ddrc1 = dev_read_addr_index(dev, 1);
	if ((FDT_ADDR_T_NONE == ddrc0) || (FDT_ADDR_T_NONE == ddrc1)) {
		pr_err("failed to get register address of DDRC\n");
		return 1;
	}

	printf("Init LPDDR5 with %dMT/s\n", CONFIG_DDR_DATARATE);
	lpddr5_silicon_init(ddrc0, CONFIG_DDR_DATARATE);
	lpddr5_silicon_init(ddrc1, CONFIG_DDR_DATARATE);
#endif
	ret = test_pattern(CONFIG_SYS_SDRAM_BASE, DDR_CHECK_SIZE);
	if (ret < 0) {
		while (1);
	}
	pr_info("init done\n");

	return 0;
}

static const struct udevice_id spacemit_ddr_ids[] = {
	{ .compatible = "spacemit,snps-lp45" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(spacemit_ddr) = {
	.name = "spacemit_ddr_ctrl",
	.id = UCLASS_RAM,
	.of_match = spacemit_ddr_ids,
	.probe = spacemit_ddr_probe,
};
