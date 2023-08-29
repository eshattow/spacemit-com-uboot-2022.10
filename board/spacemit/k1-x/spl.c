// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023, Spacemit
 */


#include <dm.h>
#include <init.h>
#include <spl.h>
#include <misc.h>
#include <log.h>
#include <linux/io.h>
#include <linux/delay.h>

#define GEN_CNT         0xD5001000

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

	timer_init();

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

