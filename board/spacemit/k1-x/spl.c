// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023, Spacemit
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

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* boot using first FIT config */
	return 0;
}
#endif

