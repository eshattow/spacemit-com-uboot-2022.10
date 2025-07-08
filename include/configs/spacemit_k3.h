/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2025, Kevin.z.m <zhangmeng.kevin@spacemit.com>
 */

 #ifndef __SPACEMIT_K3_CONFIG_H
 #define __SPACEMIT_K3_CONFIG_H

#include <linux/sizes.h>

#define PHY_ANEG_TIMEOUT	    20000

#define SYS_DRAM_OFFS               0x100000000ULL
#define SZ_1MB			0x00100000
#define SZ_2GB                      0x80000000
#define SZ_4GB                      0x100000000ULL
#define SZ_8GB                      0x200000000ULL
#define SZ_16GB                     0x400000000ULL
#define SEC_IMG_SIZE                0x2000000
#define CONFIG_SYS_SDRAM_BASE       (SYS_DRAM_OFFS + SEC_IMG_SIZE)
#define CONFIG_I2C_MULTI_BUS        1
#define KERNEL_DTB_ADDR             0x128000000

#define PMIC_I2C_BUS                8

#define CONFIG_STANDALONE_LOAD_ADDR 0x120200000

#define RISCV_MMODE_TIMERBASE		0xf1810000
#ifdef CONFIG_ASR_FPGA
#define RISCV_MMODE_TIMER_FREQ		5000000
#else
#define RISCV_MMODE_TIMER_FREQ		24000000
#endif
#define RISCV_SMODE_TIMER_FREQ		RISCV_MMODE_TIMER_FREQ

#ifndef CONFIG_FASTBOOT_FLASH_MMC_DEV
#define CONFIG_FASTBOOT_FLASH_MMC_DEV 0
#endif

/*reserve addr 0x2d000000   0x100000  1M memory for ethernet */
#define ETH_MEM_POOL_ADDR 	    0x2D000000
#define ETH_MEM_POOL_LEN	    0x100000

#define IMAGE_DDR_ADDRESS           0x130000000
#define BOOTIMG_LOAD_ADDRESS        IMAGE_DDR_ADDRESS
#define BOOTIMG_HDR_LEN             0 // (SZ_32)
#define BOOTIMG_SIG_LEN             0 // (0x100)
#define BOOTIMG_SIG_LOAD_ADDRESS    (BOOTIMG_LOAD_ADDRESS - BOOTIMG_HDR_LEN)
#define BOOTIMG_PARTITION_NAME      "boot"

#define ASR_DDR_TRAINING_DATA_BASE 0xc0829000

#define CONFIG_EXTRA_ENV_SETTINGS \
	"fdt_high=0xffffffffffffffff\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"kernel_addr_r=0x124000000\0" \
	"kernel_comp_addr_r=0x128000000\0" \
	"kernel_comp_size=0x4000000\0" \
	"fdt_addr_r=0x12c000000\0" \
	"scriptaddr=0x12c100000\0" \
	"pxefile_addr_r=0x12c200000\0" \
	"ramdisk_addr_r=0x12c300000\0"

#endif /* __SPACEMIT_K3_CONFIG_H */
