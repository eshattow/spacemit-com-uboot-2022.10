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

int spl_board_init_f(void)
{
	int ret;
	struct udevice *dev;

	printf("%s\n", __FUNCTION__);
	/* DDR init */
	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret) {
		debug("DRAM init failed: %d\n", ret);
		return ret;
	}

	return 0;
}

u32 spl_boot_device(void)
{
	return BOOT_DEVICE_RAM;
}

#if CONFIG_IS_ENABLED(FIT_IMAGE_POST_PROCESS)
#include <remoteproc.h>
#include <image.h>
/* load the esos firmare */
void board_fit_image_post_process(const void *fit, int node, void **p_image,
				  size_t *p_size)
{
	const char *name = fit_get_name(fit, node, NULL);

	if (name && !strcmp(name, "rcpu-fw")) {
		rproc_init();
		rproc_load(0, (ulong)*p_image, *p_size);
		rproc_start(0);
		*p_size = 0;
	}
}
#endif

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* boot using first FIT config */
	return 0;
}
#endif

