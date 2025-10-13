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
#include <asm/io.h>

void spl_load_env(void) { /* TODO: load environment */ }
#include <i2c.h>

#if CONFIG_IS_ENABLED(SPACEMIT_POWER)
extern int board_pmic_init(void);
#endif

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
