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

#define dcache_en() asm volatile("csrsi 0x7c0, 0x1 \n\t")
#define dcache_dis() asm volatile("csrci 0x7c0, 0x1 \n\t")
#define dcache_clean() asm volatile("csrwi 0x7c2, 0x1 \n\t")
#define dcache_valid() asm volatile("csrwi 0x7c2, 0x2 \n\t")
#define dcache_flush() asm volatile("csrwi 0x7c2, 0x3 \n\t")

#define icache_en() asm volatile("csrsi 0x7c0, 0x2 \n\t")
#define icache_dis() asm volatile("csrci 0x7c0, 0x2 \n\t")
#define icache_valid() asm volatile("csrwi 0x7c2, 0x11\n\t")


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

	dcache_en();
	icache_en();
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

