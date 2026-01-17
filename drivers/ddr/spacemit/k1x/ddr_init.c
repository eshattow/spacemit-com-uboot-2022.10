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
#include <linux/lzo.h>
#ifdef CONFIG_K1_X_BOARD_FPGA
#include "ddr_init_fpga.h"
#endif

#define DDR_CHECK_SIZE			(0x2000)
#define DDR_CHECK_STEP			(0x2000)
#define DDR_CHECK_CNT			(0x1000)
#define TOP_DDR_NUM				1

extern u32 ddr_cs_num, ddr_tx_odt, ddr_datarate;
extern const char *ddr_type;
extern char _image_binary_end[], __ddr_training_start[], __ddr_training_end[], __ddr1_training_end[];
extern uint8_t k1_lpddr3_training_img[], k1_lpddr4_training_img[];

extern int ddr_freq_change(const char *ddr_type, u32 data_rate);
extern void qos_set_default(void);
extern uint32_t lpddr4_silicon_init(uint32_t base, const char *ddr_type, uint32_t data_rate);
extern uint32_t lpddr3_silicon_init(uint32_t base, const char *ddr_type, uint32_t data_rate);

int test_pattern(fdt_addr_t base, fdt_size_t size)
{
	fdt_addr_t addr;
	uint32_t offset;
	int err = 0;

	/* may overwrite important data as image or ramdump before reboot */
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

	/* writeback and invalid cache */
	flush_dcache_range(base,base+size);
	invalidate_dcache_range(base,base+size);

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
	if (err != 0) {
		pr_emerg("dram pattern test failed!\n");
	}

	return err;
}

int decompress_ddr_code(const char* ddr_type)
{
	void *dtb;
	unsigned long flush_lenth;
	size_t dtb_length, code_size, decompress_size;
	int ret;
	uint8_t *code_start;

	// dtb was copied to AUDIO_BUFFER_ADDRESS during eraly boot stage
	dtb = (void *)AUDIO_BUFFER_ADDRESS;
	dtb_length = round_up(fdt_totalsize(dtb), 16);

	// move compressed DDR training code to audio buffer
	if (0 == strcasecmp(ddr_type, "LPDDR3")) {
		code_start = (uint8_t*)k1_lpddr3_training_img;
		code_size = (size_t)__ddr1_training_end - (size_t)code_start;
	}
	else {
		code_start = (uint8_t*)k1_lpddr4_training_img;
		code_size = (size_t)__ddr_training_end - (size_t)code_start;
	}
	if ((code_size + dtb_length) > AUDIO_BUFFER_SIZE) {
		pr_err("Error: DDR training code or dtb is too large: 0x%lx + 0x%lx > 0x%x\n",
			code_size, dtb_length, AUDIO_BUFFER_SIZE);
		return -EFBIG;
	}
	memcpy((void*)AUDIO_BUFFER_ADDRESS + dtb_length, code_start, code_size);

	// uint64_t start = get_timer_us(0);
	decompress_size = CONFIG_SPL_BSS_START_ADDR - DDR_TRAINING_DATA_BASE;
	ret = lzop_decompress((uint8_t*)AUDIO_BUFFER_ADDRESS + dtb_length, code_size,
		(uint8_t*)DDR_TRAINING_DATA_BASE, &decompress_size);

	if (0 != ret) {
		return ret;
	}
	// start = get_timer_us(start);
	// printf("Decompress consume %lldus\n", start);

	pr_debug("Decompressed code size is %ld\n", decompress_size);
	flush_lenth = round_up(decompress_size, CONFIG_RISCV_CBOM_BLOCK_SIZE);
	flush_dcache_range(DDR_TRAINING_DATA_BASE, DDR_TRAINING_DATA_BASE + flush_lenth);

	return 0;
}

static int spacemit_ddr_probe(struct udevice *dev)
{
	int ret;
	uint32_t data_rate;

#ifdef CONFIG_K1_X_BOARD_FPGA
	void (*ddr_init)(void);
#else
	fdt_addr_t ddrc_base;

	ddrc_base = dev_read_addr(dev);
#endif

#ifdef CONFIG_K1_X_BOARD_FPGA
	ddr_init = (void(*)(void))(lpddr4_init_fpga_data + 0x144);
	ddr_init();
#else

	/* if dram data-rate is NOT configued in eeprom or in dts, use default value */
	if ((0 == ddr_datarate) && (dev_read_u32u(dev, "datarate", &ddr_datarate))) {
		pr_info("ddr data rate not configed in dts, use 1200 as default!\n");
		ddr_datarate = 1200;
	} else {
		pr_info("ddr data rate is configured as %dMT/s\n", ddr_datarate);
	}

	/* if DDR cs number is NOT configued in eeprom or in dts, use default value */
	if ((0 == ddr_cs_num) && dev_read_u32u(dev, "cs-num", &ddr_cs_num)) {
		pr_info("ddr cs number not configed in dts!\n");
		ddr_cs_num = DDR_CS_NUM;
	}

	/* if DDR tx odt is NOT configued in eeprom or in dts, use default value */
	if ((0 == ddr_tx_odt) && dev_read_u32u(dev, "tx-odt", &ddr_tx_odt)) {
		pr_info("ddr tx odt not configed in dts!\n");
	}

	if (NULL == ddr_type) {
		ddr_type = dev_read_string(dev, "type");
	}
	printf("DDR type %s\n", ddr_type);

	ret = decompress_ddr_code(ddr_type);
	if (0 != ret) {
		pr_emerg("decompress ddr code failed(%d)!\n", ret);
		return ret;
	}

	/* init dram */
	uint64_t start = get_timer(0);
	if (0 == strcasecmp(ddr_type, "LPDDR3")) {
		data_rate = lpddr3_silicon_init(ddrc_base, ddr_type, ddr_datarate);
	}
	else {
		data_rate = lpddr4_silicon_init(ddrc_base, ddr_type, ddr_datarate);
	}
	start = get_timer(start);
	printf("lpddr3_silicon_init consume %lldms\n", start);
#endif
	ddr_freq_change(ddr_type, data_rate);

	ret = test_pattern(CONFIG_SYS_SDRAM_BASE, DDR_CHECK_SIZE);
	if (ret < 0) {
		pr_err("dram init failed!\n");
		return -EIO;
	}
#ifdef CONFIG_DDR_QOS
	qos_set_default();
#endif
	pr_info("dram init done\n");

	return 0;
}

static const struct udevice_id spacemit_ddr_ids[] = {
	{ .compatible = "spacemit,ddr-ctl" },
	{}
};

U_BOOT_DRIVER(spacemit_ddr) = {
	.name = "spacemit_ddr_ctrl",
	.id = UCLASS_RAM,
	.of_match = spacemit_ddr_ids,
	.probe = spacemit_ddr_probe,
};
