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

#ifdef CONFIG_K1_X_BOARD_ASIC

extern void lpddr4_silicon_init(uint32_t base);

extern uint8_t pmic_read(uint8_t i2c_bus, uint8_t addr, uint8_t reg);
extern uint8_t pmic_write_with_check(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint8_t reg_val, uint8_t check_val);
extern uint8_t pmic_write(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint8_t reg_val);

void init_pmic(void)
{
	int	i;
	uint8_t val;
	uint8_t i2c_bus = 8;
	uint8_t base_addr = 0x30;
	uint8_t power_addr = 0x31;
	uint8_t id_reg = 0x0;
	uint8_t buck1_reg = 0x30;
	uint8_t dvc_reg = 0x41;
	uint8_t pwr_reg_en1 = 0x11;
	uint8_t pwr_reg_en3 = 0x13;

	/* read pmic version */
	val = pmic_read(i2c_bus, base_addr, id_reg);
	printf("%s chip_id = 0x%x\n", __func__, val);

	/* disable dvc */
	printf("disable pmic dvc function!\n");
	pmic_write(i2c_bus, power_addr, dvc_reg, 0x0);

	/* enable buck5, default 1.5v */
	printf("enable buck-5, default voltage is 1.5v\n");
	val = pmic_read(i2c_bus, power_addr, pwr_reg_en1);
	val |= (1 << 4);
	pmic_write(i2c_bus, power_addr, pwr_reg_en1, val);

	/* enable ldo19, default value is 1.1v */
	printf("enable ldo-19, default voltage is 1.1v\n");
	val = pmic_read(i2c_bus, power_addr, pwr_reg_en3);
	val |= 1 << 7;
	pmic_write(i2c_bus, power_addr, pwr_reg_en3, val);

	/* enable buck4, default 0.6v */
	printf("enable buck-4, default voltage is 0.6v\n");
	val = pmic_read(i2c_bus, power_addr, pwr_reg_en1);
	val |= (1 << 3);
	pmic_write(i2c_bus, power_addr, pwr_reg_en1, val);

	/* enable ldo4, default 3.1v */
	printf("enable ldo-4, default voltage is 3.1v\n");
	val = pmic_read(i2c_bus, power_addr, 0x12);
	val |= (1 << 0);
	pmic_write(i2c_bus, power_addr, 0x12, val);

	/* adjust buck1 to 0.95v */
	printf("adust buck-1 voltage to 0.95v\n");
	val = pmic_read(i2c_bus, power_addr, buck1_reg);
	val = 0x2f;
	pmic_write_with_check(i2c_bus, power_addr, buck1_reg, val, (val | (1 << 7)));

	/* set ext-dcdc to 1.0v */
//	printf("adust ext-dcdc voltage to 1.0v\n");
//	pmic_write(i2c_bus, 0x70, 0x0, 0x28);

	/* delay some time to wait power stable */
	for(i=0; i<0x1000000; i++) {
		nop();
		nop();
		nop();
		nop();
		nop();
	}
}
#endif

static int spacemit_ddr_probe(struct udevice *dev)
{
	int ret;

#ifdef CONFIG_K1_X_BOARD_FPGA
	void (*ddr_init)(void);
#else
	uint32_t val;
	fdt_addr_t ddrc_base;
	ddrc_base = dev_read_addr(dev);
#endif

	printf("%s(%d): here\n", __func__, __LINE__);

#ifdef CONFIG_K1_X_BOARD_FPGA
	ddr_init = (void(*)(void))(lpddr4_init_fpga_data + 0x144);
	ddr_init();
#else
	writel(0x2dffff, (void __iomem *)0xd4051024);

	/* ajdust power supply */
	init_pmic();

	/* change cpu cluster0 frequency to 1248Mhz */
	printf("adjust cpu0 freqency to 1248Mhz\n");
	val = readl((void __iomem *)(K1X_MPMU_BASE + 0x1024));
	val |= BIT(16);
	writel(val, (void __iomem *)(K1X_MPMU_BASE + 0x1024));

	val = readl((void __iomem *)(K1X_APMU_BASE + 0x38c));
	val &= ~0x07;
	val |= 0x04;
	writel(val, (void __iomem *)(K1X_APMU_BASE + 0x38c));

	val = readl((void __iomem *)(K1X_APMU_BASE + 0x38c));
	val |= BIT(12);

	writel(val, (void __iomem *)(K1X_APMU_BASE + 0x38c));
	while(readl((void __iomem *)(K1X_APMU_BASE + 0x38c)) & BIT(12));

	lpddr4_silicon_init(ddrc_base);
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
