// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025, Spacemit
 */

#include <dm.h>
#include <init.h>
#include <spl.h>
#include <misc.h>
#include <log.h>
#include <linux/delay.h>
#include <remoteproc.h>
#include <image.h>
#include <common.h>
#include <env.h>
#include <env_internal.h>
#include <asm/io.h>
#include <i2c.h>
#include <espi.h>

#if CONFIG_IS_ENABLED(SPACEMIT_POWER)
extern int board_pmic_init(void);
#endif
enum board_boot_mode get_boot_mode(void);
static void spl_load_env(void);

int spl_board_init_f(void)
{
	int ret;
	struct udevice *dev;

	/* DDR init */
	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret) {
		debug("DRAM init failed: %d\n", ret);
		return ret;
	}
#if CONFIG_IS_ENABLED(SYS_I2C_LEGACY)
	/* init i2c */
	i2c_init_board();
#endif

#if CONFIG_IS_ENABLED(SPACEMIT_POWER)
	board_pmic_init();
#endif

#ifdef CONFIG_SPL_REMOTEPROC_K3_PROC
	rproc_init();
#endif

	spl_load_env();

#ifdef CONFIG_SPL_ESPI
	/* Probe eSPI device */
	ret = uclass_first_device(UCLASS_ESPI, &dev);
	if (ret) {
		pr_debug("eSPI: Init failed (ret=%d)\n", ret);
		return 0;
	}
	if (!dev) {
		pr_debug("eSPI: No device found\n");
		return 0;
	}
#endif

	return 0;
}

void spl_board_init(void)
{
	spl_load_env();
}

#if CONFIG_IS_ENABLED(FIT_IMAGE_POST_PROCESS)
/* load the esos firmare */
void board_fit_image_post_process(const void *fit, int node, void **p_image, size_t *p_size)
{
#ifdef CONFIG_SPL_REMOTEPROC_K3_PROC
	const char *name = fit_get_name(fit, node, NULL);

	if (name && !strncmp(name, "rcpu0-fw", 8)) {
		rproc_load(0, (ulong)*p_image, *p_size);
		rproc_start(0);
		*p_size = 0;
	} else if (name && !strncmp(name, "rcpu1-fw", 8)) {
		rproc_load(1, (ulong)*p_image, *p_size);
		rproc_start(1);
		*p_size = 0;
	}
#endif
}
#endif

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char* name)
{
	/* boot using first FIT config */
	return 0;
}
#endif

/**********************************************************
 * load env from storage
 *********************************************************/
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
		pr_debug("%s: No environment driver for location %d\n", __func__, loc);
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
#ifdef CONFIG_ENV_IS_IN_MTD
	case BOOT_MODE_NAND:
	case BOOT_MODE_NOR:
		loc = ENVL_MTD;
		break;
#endif
	default:
		return;
	}

	drv = spl_env_driver_lookup(ENVOP_INIT, loc);
	if (!drv){
		pr_err("%s, can not load env from storage\n", __func__);
		return;
	}

	ret = drv->load();
	if (!ret){
		pr_info("has init env successful\n");
	}else{
		pr_err("load env from storage fail, would use default env\n");
		/*if load env from storage fail, it should not write bootmode to reg*/
		boot_mode = BOOT_MODE_NONE;
	}
}
