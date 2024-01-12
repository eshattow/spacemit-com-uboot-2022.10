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
#include <cpu.h>
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

extern void lpddr4_silicon_init(uint32_t base, uint32_t data_rate);

extern uint8_t pmic_read(uint8_t i2c_bus, uint8_t addr, uint8_t reg);
extern uint8_t pmic_write_with_check(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint8_t reg_val, uint8_t check_val);
extern uint8_t pmic_write(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint8_t reg_val);
extern int pmic_detect(uint8_t i2c_bus, uint8_t addr, uint8_t reg_addr);

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
	struct udevice *cpu;
	uint32_t cpu_voltage;

	/* read pmic version */
	val = pmic_read(i2c_bus, base_addr, id_reg);
	printf("%s chip_id = 0x%x\n", __func__, val);

	/* disable dvc */
	printf("disable pmic dvc function!\n");
	pmic_write(i2c_bus, power_addr, dvc_reg, 0x0);

	/* enable buck5, default 1.5v */
	printf("enable buck-5, default voltage is 1.5v ...");
	val = pmic_read(i2c_bus, power_addr, pwr_reg_en1);
	val |= (1 << 4);
	pmic_write(i2c_bus, power_addr, pwr_reg_en1, val);
	printf("	[succeed]\n");

	/* enable ldo19, default value is 1.1v */
	printf("enable ldo-19, default voltage is 1.1v ...");
	val = pmic_read(i2c_bus, power_addr, pwr_reg_en3);
	val |= 1 << 7;
	pmic_write(i2c_bus, power_addr, pwr_reg_en3, val);
	printf("	[succeed]\n");

	/* enable buck4, default 0.6v */
	printf("enable buck-4, default voltage is 0.6v ...");
	val = pmic_read(i2c_bus, power_addr, pwr_reg_en1);
	val |= (1 << 3);
	pmic_write(i2c_bus, power_addr, pwr_reg_en1, val);
	printf("	[succeed]\n");

	/* enable ldo4, default 2.8v */
	printf("enable ldo-4, default voltage is 2.8v ...");
	val = pmic_read(i2c_bus, power_addr, 0x12);
	val |= (1 << 0);
	pmic_write(i2c_bus, power_addr, 0x12, val);
	printf("	[succeed]\n");

	/* adjust ldo-4 to 3.3v */
	printf("adust ldo-4 voltage to 3.3v ...");
	val = pmic_read(i2c_bus, power_addr, 0xBA);
	val |= 0x0f;
	pmic_write(i2c_bus, power_addr, 0xBA, val);
	printf("			[succeed]\n");

	/* enable ld9, default 3.1v */
	printf("enable ldo-9, default voltage is 3.1v ...");
	val = pmic_read(i2c_bus, power_addr, 0x12);
	val |= (1 << 5);
	pmic_write(i2c_bus, power_addr, 0x12, val);
	printf("	[succeed]\n");

	cpu = cpu_get_current_dev();
	if(dev_read_u32u(cpu, "boot_voltage", &cpu_voltage)) {
		printf("boot_voltage not configured, use 0.95v as default!\n");
		cpu_voltage = 950;
	}
	/* check if cpu voltage is valid */
	cpu_voltage = cpu_voltage < 800? 800:cpu_voltage;
	cpu_voltage = cpu_voltage > 1200? 1200:cpu_voltage;

	/* adjust buck1 voltage */
	printf("adust buck-1 voltage to %u.%03uv ...", cpu_voltage/1000, cpu_voltage%1000);
	val = pmic_read(i2c_bus, power_addr, buck1_reg);
	val = 0x20 + (cpu_voltage - 800)/10;
	pmic_write_with_check(i2c_bus, power_addr, buck1_reg, val, (val | (1 << 7)));
	printf("		[succeed]\n");

	if(pmic_detect(i2c_bus, 0x70, 0x0) == 0) {
		printf("try to detect ext-cpu-dcdc ..."	\
			"			[succeed]\n");
		val = (cpu_voltage - 600)/10;
		/* set ext-dcdc to 1.0v */
		if(pmic_write(i2c_bus, 0x70, 0x0, val) != val) {
			printf("try adjust ext-dcdc voltage to %u.%03uv "\
				"...	[failed]\n", cpu_voltage/1000, cpu_voltage%1000);
		} else {
			printf("try adjust ext-dcdc voltage to %u.%03uv "\
				"...	[succeed]\n", cpu_voltage/1000, cpu_voltage%1000);
		}
	} else {
		printf("try to detect ext-dcdc ..."\
			"			[failed]\n");
	}

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

__maybe_unused static void fix_ddr_data(unsigned long long start, unsigned long long end)
{
	unsigned long long value;

	printf("try write 0x%llx ~ 0x%llx ...\n", start, end);
	for(value=start; value<end; value +=8) {
		*(unsigned long long *)value = value;
	}
}

__maybe_unused static void check_ddr_data(unsigned long long start, unsigned long long end)
{
	unsigned long long value;

	printf("checking 0x%llx ~ 0x%llx ...\n", start, end);
	for(value=start; value<end; value +=8) {
		if(*(unsigned long long *)value != value) {
			printf("addr:0x%llx err:0x%llx, should be:0x%llx \n",
				 value, *(unsigned long long *)value, value);
		}
	}
	printf("checking 0x%llx ~ 0x%llx done\n", start, end);
}

static uint32_t adjust_cpu_freq(uint64_t cluster, uint32_t freq)
{
	uint32_t freq_act=freq, val;

	/* switch cpu clock source */
	val = readl((void __iomem *)(K1X_APMU_BASE + 0x38c + cluster*4));
	val &= ~(0x07 | BIT(13));
	switch(freq) {
	case 1600000:
		val |= 0x07;
		break;

	case 1228000:
		val |= 0x04;
		break;

	case 819000:
		val |= 0x01;
		break;

	case 614000:
	default:
		freq_act = 614000;
		val |= 0x00;
		break;
	}
	writel(val, (void __iomem *)(K1X_APMU_BASE + 0x38c + cluster*4));

	/* set cluster frequency change request, and wait done */
	val = readl((void __iomem *)(K1X_APMU_BASE + 0x38c + cluster*4));
	val |= BIT(12);
	writel(val, (void __iomem *)(K1X_APMU_BASE + 0x38c + cluster*4));
	while(readl((void __iomem *)(K1X_APMU_BASE + 0x38c + cluster*4)) & BIT(12));

	return freq_act;
}

static int spacemit_ddr_probe(struct udevice *dev)
{
	int ret;

#ifdef CONFIG_K1_X_BOARD_FPGA
	void (*ddr_init)(void);
#else
	uint32_t val, cpu_freq, ddr_datarate, ddr_voltage=0;
	fdt_addr_t ddrc_base;
	struct udevice *cpu;

	ddrc_base = dev_read_addr(dev);
#endif

#ifdef CONFIG_K1_X_BOARD_FPGA
	ddr_init = (void(*)(void))(lpddr4_init_fpga_data + 0x144);
	ddr_init();
#else
	writel(0x2dffff, (void __iomem *)0xd4051024);

	/* ajdust power supply */
	init_pmic();

	/* enable CLK_1228M */
	val = readl((void __iomem *)(K1X_MPMU_BASE + 0x1024));
	val |= BIT(16) | BIT(15) | BIT(14) | BIT(13);
	writel(val, (void __iomem *)(K1X_MPMU_BASE + 0x1024));

	/* enable PLL3(3200Mhz) */
	val = readl((void __iomem *)(K1X_APB_SPARE_BASE + 0x12C));
	val |= BIT(31);
	writel(val, (void __iomem *)(K1X_APB_SPARE_BASE + 0x12C));
	/* enable PLL3_DIV2 */
	val = readl((void __iomem *)(K1X_APB_SPARE_BASE + 0x128));
	val |= BIT(1);
	writel(val, (void __iomem *)(K1X_APB_SPARE_BASE + 0x128));

	cpu = cpu_get_current_dev();
	if(dev_read_u32u(cpu, "boot_freq_cluster0", &cpu_freq)) {
		printf("boot_freq_cluster0 not configured, use 1228000 as default!\n");
		cpu_freq = 1228000;
	}
	cpu_freq = adjust_cpu_freq(0, cpu_freq);
	printf("adjust cluster-0 frequency to %u ...	[done]\n", cpu_freq);

	if(dev_read_u32u(cpu, "boot_freq_cluster1", &cpu_freq)) {
		printf("boot_freq_cluster1 not configured, use 1228000 as default!\n");
		cpu_freq = 614000;
	}
	cpu_freq = adjust_cpu_freq(1, cpu_freq);
	printf("adjust cluster-1 frequency to %u ...	[done]\n", cpu_freq);

	/* check if need adjust ddr voltage */
	if(dev_read_u32u(dev, "dram_voltage", &ddr_voltage)) {
		printf("dram voltage not configed in dts, use pmic output default!\n");
	} else {
		if ((ddr_voltage < 600) || (ddr_voltage > 800)) {
			printf("dram voltage 0.%uv configured in dts is invalid!\n ", ddr_voltage/10);
		} else {
			printf("adust buck-4 voltage to 0.%uv ...", ddr_voltage/10);
			val = (ddr_voltage - 480)/10;
			/* i2c_bus:8, i2c_addr:0x31, buck4_reg:0x80 */
			pmic_write_with_check(8, 0x31, 0x80, val, (val | (1 << 7)));
			printf("		[succeed]\n");
		}
	}

	/* check if dram data-rate is configued in dts */
	if(dev_read_u32u(dev, "datarate", &ddr_datarate)) {
		printf("ddr data rate not configed in dts, use 1200 as default!\n");
		ddr_datarate = 1200;
	} else {
		printf("ddr data rate is %u configured in dts\n", ddr_datarate);
	}

	/* init dram */
	lpddr4_silicon_init(ddrc_base, ddr_datarate);
#endif

	ret = test_pattern(CONFIG_SYS_SDRAM_BASE, DDR_CHECK_SIZE);
	if (ret < 0) {
		log_err("dram init failed!\n");
		return -EIO;
	}
	log_debug("dram init done\n");

/* check dram space */

//#define CHECK_4GB_DDR_ACCESS
//#define CHECK_8GB_DDR_ACCESS
#if defined(CHECK_4GB_DDR_ACCESS) || defined (CHECK_8GB_DDR_ACCESS)
	/* check 0GB~2GB rw */
	fix_ddr_data(0x1000, 0x7fffffff);
	check_ddr_data(0x1000, 0x7fffffff);

	/* check 2GB~4GB rw */
	fix_ddr_data(0x100000000, 0x17fffffff);
	check_ddr_data(0x100000000, 0x17fffffff);

#if defined(CHECK_8GB_DDR_ACCESS)
	/* check 4GB~6GB rw */
	fix_ddr_data(0x180000000, 0x1ffffffff);
	check_ddr_data(0x180000000, 0x1ffffffff);

	/* check 6GB~8GB rw */
	fix_ddr_data(0x200000000, 0x27fffffff);
	check_ddr_data(0x200000000, 0x27fffffff);
#endif

	/* check 0GB~2GB rw */
	check_ddr_data(0x1000, 0x7fffffff);

	/* check 2GB~4GB rw */
	check_ddr_data(0x100000000, 0x17fffffff);

#if defined(CHECK_8GB_DDR_ACCESS)
	/* check 4GB~8GB rw */
	check_ddr_data(0x180000000, 0x27fffffff);
#endif

#endif

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
