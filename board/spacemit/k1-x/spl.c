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
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <env.h>
#include <env_internal.h>
#include <mapmem.h>
#include <asm/global_data.h>

#define GEN_CNT         0xD5001000

enum env_location spl_env_locations[] = {
	ENVL_MMC,
	ENVL_NAND,
	ENVL_SPI_FLASH,
};

/*
	     b'(bit1)(bit0)
	emmc:b'00
	nor :b'10
	nand:b'01
	sd  :b'11
*/
static u32 spl_boot_mode = SPL_BOOT_MODE_SD;

int mmc_get_env_dev(void)
{
	u32 boot_mode = 0;
#ifdef CONFIG_SPL_BUILD
	debug("spl building, spl_boot_mode:%x\n", spl_boot_mode);
	boot_mode = spl_boot_mode;
#else
	boot_mode = readl((void *)BOOT_DEV_FLAG_REG);
	debug("%s, uboot boot_mode:%x\n", __func__, boot_mode);
#endif

	switch (boot_mode) {
	case SPL_BOOT_MODE_EMMC:
		return 1;
	case SPL_BOOT_MODE_SD:
	default:
		return 0;
	}
}


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

	debug("%s\n", __FUNCTION__);
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
	u32 boot_mode = readl((void *)BOOT_PIN_SELECT);

	/*select spl boot device
	     b'(bit1)(bit0)
	emmc:b'00
	nor :b'10
	nand:b'01
	sd  :b'11
	*/
	switch (boot_mode & 0x3) {
	case 0:
		return BOOT_DEVICE_MMC2;//emmc
	case 1:
		return BOOT_DEVICE_NAND;
	case 2:
		return BOOT_DEVICE_NOR;
	case 3:
		return BOOT_DEVICE_MMC1;//sd
	default:
		debug("not boot from storage, boot from ram: 0x%x.\n",
			  boot_mode);
		return BOOT_DEVICE_RAM;
	}
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* boot using first FIT config */
	return 0;
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

enum env_location spl_env_get_location(enum env_operation op, int prio)
{
	if (prio >= ARRAY_SIZE(spl_env_locations))
		return ENVL_UNKNOWN;

	return spl_env_locations[prio];
}


static struct env_driver *spl_env_driver_lookup(enum env_operation op, int prio)
{
	enum env_location loc = spl_env_get_location(op, prio);
	struct env_driver *drv;

	if (loc == ENVL_UNKNOWN)
		return NULL;

	drv = _spl_env_driver_lookup(loc);
	if (!drv) {
		debug("%s: No environment driver for location %d\n", __func__,
		      loc);
		return NULL;
	}

	return drv;
}



void spl_board_init(void)
{
	struct env_driver *drv;
	int prio = 0;
	int ret = 0;

	/*would try sd at first*/
	drv = spl_env_driver_lookup(ENVOP_INIT, prio);
	ret = drv->load();

	if (!ret){
		printf("has init env successful at sd, spl_boot_mode:%x\n", spl_boot_mode);
	}else{
		/*if try sd fail, it would try emmc or nor or nand*/
		u32 boot_mode = spl_boot_device();
		int prio = 0;
		switch (boot_mode) {
		case BOOT_DEVICE_MMC2:
			prio = SPL_BOOT_MODE_EMMC;
			spl_boot_mode = SPL_BOOT_MODE_EMMC;
			break;
		case BOOT_DEVICE_NAND:
			prio = SPL_BOOT_MODE_NAND;
			spl_boot_mode = SPL_BOOT_MODE_NAND;
			break;
		case BOOT_DEVICE_NOR:
			prio = SPL_BOOT_MODE_NOR;
			spl_boot_mode = SPL_BOOT_MODE_NOR;
			break;
		default:
			//not exist env location
			prio = 3;
			break;
		}
		drv = spl_env_driver_lookup(ENVOP_INIT, prio);
		ret = drv->load();
		debug("init other storage, spl_boot_mode:%x\n", spl_boot_mode);
		if (!ret){
			printf("has init env successful\n");
		}else{
			printf("load env from storage fail, would use default env\n");
		}
		const char *mtdparts = NULL;
		const char *gptparts = NULL;
		mtdparts = env_get("mtdparts");
		gptparts = env_get("partitions");
		if (mtdparts)
			debug("mtdparts:%s,\n", mtdparts);
		if (gptparts)
			debug(" gptparts:%s,\n", gptparts);
	}
}


void spl_perform_fixups(struct spl_image_info *spl_image)
{
	u32 boot_mode = 0;

	/*if boot from fastboot, should not change BOOT_DEV_FLAG_REG*/
	if (readl((void *)BOOT_DEV_FLAG_REG) == USB_DOWNLOAD_FLAG){
		printf("boot from fastboot\n");
		return;
	}
	debug("read BOOT_DEV_FLAG_REG:%x\n", readl((void *)BOOT_DEV_FLAG_REG));
	switch (spl_image->boot_device) {
	case BOOT_DEVICE_SPI:
		boot_mode = SPL_BOOT_MODE_NOR;
		break;
	case BOOT_DEVICE_NAND:
		boot_mode = SPL_BOOT_MODE_NAND;
		break;
	case BOOT_DEVICE_MMC2:
		boot_mode = SPL_BOOT_MODE_EMMC;
		break;
	case BOOT_DEVICE_MMC1:
	default:
		boot_mode = SPL_BOOT_MODE_SD;
		break;
	}
	writel(boot_mode, (void *)BOOT_DEV_FLAG_REG);
}


struct image_header *spl_get_load_buffer(ssize_t offset, size_t size)
{
	return map_sysmem(CONFIG_SPL_LOAD_FIT_ADDRESS, 0);
}


enum env_location env_get_location(enum env_operation op, int prio)
{
	if (prio >= ARRAY_SIZE(spl_env_locations))
		return ENVL_UNKNOWN;

	u32 read_reg = readl((void *)BOOT_DEV_FLAG_REG);
	switch (read_reg) {
	case SPL_BOOT_MODE_NAND:
		return spl_env_locations[1];
	case SPL_BOOT_MODE_NOR:
		return spl_env_locations[2];
	case SPL_BOOT_MODE_EMMC:
	case SPL_BOOT_MODE_SD:
	default:
		return spl_env_locations[0];
	}
}
