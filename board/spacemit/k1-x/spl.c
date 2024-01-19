// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#include <common.h>
#include <dm.h>
#include <init.h>
#include <spl.h>
#include <misc.h>
#include <log.h>
#include <i2c.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <env.h>
#include <env_internal.h>
#include <mapmem.h>
#include <asm/global_data.h>
#include <fb_spacemit.h>
#include <tlv_eeprom.h>
#include <stdlib.h>

#define GEN_CNT			(0xD5001000)
#define STORAGE_API_P_ADDR	(0xC0838498)
#define SDCARD_API_ENTRY	(0xFFE0A548)

extern int k1x_eeprom_init(void);
extern int spacemit_eeprom_read(uint8_t chip, uint8_t *buffer, uint8_t id);
char *product_name;

int timer_init(void)
{
	/* enable generic cnt */
	u32 read_data;
	void __iomem *reg;

	reg = ioremap(GEN_CNT, 0x20);
	read_data = readl(reg);
	read_data |= BIT(0);
	writel(read_data, reg);

	return 0;
}

enum board_boot_mode get_boot_storage(void)
{
	size_t *api = (size_t*)STORAGE_API_P_ADDR;
	size_t address = *api;
	// Did NOT select sdcard boot, but sdcard always has first boot priority
	if (SDCARD_API_ENTRY == address)
		return BOOT_MODE_SD;
	else
		return get_boot_pin_select();
}

void fix_boot_mode(void)
{
	if (0 == readl((void *)BOOT_DEV_FLAG_REG))
		set_boot_mode(get_boot_storage());
}

#if CONFIG_IS_ENABLED(SPACEMIT_K1X_EFUSE)
int load_board_config_from_efuse(int *eeprom_i2c_index,
				 int *eeprom_pin_group, int *pmic_type)
{
	struct udevice *dev;
	uint8_t fuses[2];
	int ret;

	/* retrieve the device */
	ret = uclass_get_device_by_driver(UCLASS_MISC,
			DM_DRIVER_GET(spacemit_k1x_efuse), &dev);
	if (ret) {
		return ret;
	}

	// read from efuse, each bank has 32byte efuse data
	ret = misc_read(dev, 9 * 32 + 0, fuses, sizeof(fuses));
	if ((0 == ret) && (0 != fuses[0])) {
		// byte0 bit0~3 is eeprom i2c controller index
		*eeprom_i2c_index = fuses[0] & 0x0F;
		// byte0 bit4~5 is eeprom pin group index
		*eeprom_pin_group = (fuses[0] >> 4) & 0x03;
		// byte1 bit0~3 is pmic type
		*pmic_type = fuses[1] & 0x0F;
	}

	return ret;
}
#endif

static void load_default_board_config(int *eeprom_i2c_index,
		int *eeprom_pin_group, int *pmic_type)
{
	char *temp;

	temp = env_get("eeprom_i2c_index");
	if (NULL != temp)
		*eeprom_i2c_index = dectoul(temp, NULL);
	else
		*eeprom_i2c_index = K1_DEFALT_EEPROM_I2C_INDEX;

	temp = env_get("eeprom_pin_group");
	if (NULL != temp)
		*eeprom_pin_group = dectoul(temp, NULL);
	else
		*eeprom_pin_group = K1_DEFALT_EEPROM_PIN_GROUP;

	temp = env_get("pmic_type");
	if (NULL != temp)
		*pmic_type = dectoul(temp, NULL);
	else
		*pmic_type = K1_DEFALT_PMIC_TYPE;
}

#if CONFIG_IS_ENABLED(SPACEMIT_POWER)
extern int board_pmic_init(void);
#endif

void load_board_config(int *eeprom_i2c_index, int *eeprom_pin_group, int *pmic_type)
{
	load_default_board_config(eeprom_i2c_index, eeprom_pin_group, pmic_type);

#if CONFIG_IS_ENABLED(SPACEMIT_K1X_EFUSE)
	/* update env from efuse data */
	load_board_config_from_efuse(eeprom_i2c_index, eeprom_pin_group, pmic_type);
#endif

	printf("eeprom_i2c_index :%d\n", *eeprom_i2c_index);
	printf("eeprom_pin_group :%d\n", *eeprom_pin_group);
	printf("pmic_type :%d\n", *pmic_type);
}

int spl_board_init_f(void)
{
	int ret;
	struct udevice *dev;

	debug("%s\n", __FUNCTION__);

#if CONFIG_IS_ENABLED(SYS_I2C_LEGACY)
	/* init i2c */
	i2c_init_board();
#endif

#if CONFIG_IS_ENABLED(SPACEMIT_POWER)
	board_pmic_init();
#endif

	/* DDR init */
	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret) {
		debug("DRAM init failed: %d\n", ret);
		return ret;
	}

	timer_init();

	return 0;
}

void board_init_f(ulong dummy)
{
	int ret;

	// fix boot mode after boot rom
	fix_boot_mode();
	ret = spl_early_init();
	if (ret)
		panic("spl_early_init() failed: %d\n", ret);

	riscv_cpu_setup(NULL, NULL);

	preloader_console_init();

	ret = spl_board_init_f();
	if (ret)
		panic("spl_board_init_f() failed: %d\n", ret);
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	if (NULL == product_name)
		product_name = env_get("product_name");

	if ((NULL != product_name) && (0 == strcmp(product_name, name))) {
		printf("Boot from fit configuration %s\n", name);
		return 0;
	}
	else
		return -1;
}
#endif

static struct env_driver *_spl_env_driver_lookup(enum env_location loc)
{
	struct env_driver *drv;
	const int n_ents = ll_entry_count(struct env_driver, env_driver);
	struct env_driver *entry;

	drv = ll_entry_start(struct env_driver, env_driver);
	for (entry = drv; entry != drv + n_ents; entry++) {
		if (loc == entry->location)
			return entry;
	}

	/* Not found */
	return NULL;
}

static struct env_driver *spl_env_driver_lookup(enum env_operation op, enum env_location loc)
{
	struct env_driver *drv;

	if (loc == ENVL_UNKNOWN)
		return NULL;

	drv = _spl_env_driver_lookup(loc);
	if (!drv) {
		debug("%s: No environment driver for location %d\n", __func__, loc);
		return NULL;
	}

	return drv;
}

static void spl_load_env(void)
{
	struct env_driver *drv;
	int ret = -1;
	u32 boot_mode = get_boot_mode();

	/*if boot from usb, spl should not find env*/
	if (boot_mode == BOOT_MODE_USB){
		return;
	}

	/*
	only load env from mtd dev, because only mtd dev need
	env mtdparts info to load image.
	*/
	enum env_location loc = ENVL_UNKNOWN;
	switch (boot_mode) {
#ifdef CONFIG_ENV_IS_IN_NAND
	case BOOT_MODE_NAND:
		loc = ENVL_NAND;
		break;
#endif
#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
	case BOOT_MODE_NOR:
		loc = ENVL_SPI_FLASH;
		break;
#endif
	default:
		return;
	}

	drv = spl_env_driver_lookup(ENVOP_INIT, loc);
	if (!drv){
		printf("%s, can not load env from storage\n", __func__);
		return;
	}

	ret = drv->load();
	if (!ret){
		printf("has init env successful\n");
	}else{
		printf("load env from storage fail, would use default env\n");
		/*if load env from storage fail, it should not write bootmode to reg*/
		boot_mode = BOOT_MODE_NONE;
	}
}

char *get_product_name(void)
{
	char *name = NULL;
	int eeprom_addr;

	eeprom_addr = k1x_eeprom_init();
	name = calloc(1, 64);
	if ((eeprom_addr >= 0) && (NULL != name) && (0 == spacemit_eeprom_read(
		eeprom_addr, name, TLV_CODE_PRODUCT_NAME))) {
		printf("Get product name from eeprom %s\n", name);
		return name;
	}

	if (NULL != name)
		free(name);

	printf("Use default product name %s\n", env_get("product_name"));
	return NULL;
}

void spl_board_init(void)
{
	/*load env*/
	spl_load_env();
	product_name = get_product_name();
}

void spl_perform_fixups(struct spl_image_info *spl_image)
{
	u32 boot_mode = get_boot_mode();

	/*if boot from fastboot, should not change BOOT_DEV_FLAG_REG*/
	if (boot_mode == BOOT_MODE_USB){
		printf("boot from fastboot\n");
		return;
	}
	debug("read BOOT_DEV_FLAG_REG:%x\n", boot_mode);
	switch (spl_image->boot_device) {
	case BOOT_DEVICE_NOR:
		boot_mode = BOOT_MODE_NOR;
		break;
	case BOOT_DEVICE_NAND:
		boot_mode = BOOT_MODE_NAND;
		break;
	case BOOT_DEVICE_MMC2:
		boot_mode = BOOT_MODE_EMMC;
		break;
	case BOOT_DEVICE_MMC1:
	default:
		boot_mode = BOOT_MODE_SD;
		break;
	}
	set_boot_mode(boot_mode);
}

struct image_header *spl_get_load_buffer(ssize_t offset, size_t size)
{
	return map_sysmem(CONFIG_SPL_LOAD_FIT_ADDRESS, 0);
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
