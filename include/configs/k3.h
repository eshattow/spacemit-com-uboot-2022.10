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
#define BOOTFS_NAME	("bootfs")

// sram buffer address that save the DDR software training result
#define DDR_TRAINING_INFO_BUFF	(0xC0800000)
#define DDR_TRAINING_INFO_SAVE_ADDR	(0)
// magic string: "DDRT"
#define DDR_TRAINING_INFO_MAGIC	(0x54524444)
// ddr training software version: xx.xx.xxxx
#define DDR_TRAINING_INFO_VER	(0x00010000)
// default ddr channel number
#define DDR_CS_NUM	(1)

/*
use (ram_base+4MB offset) as the address to loading image.
 use ram_size-32MB as the max size to loading image, if
 (ram_size-32MB) more than 500MB, set load image size as
 500MB.
*/
#define RECOVERY_RAM_SIZE (gd->ram_size - 0x2000000)
#define RECOVERY_LOAD_IMG_SIZE_MAX (RECOVERY_RAM_SIZE > 0x1f400000 ? 0x1f400000 : RECOVERY_RAM_SIZE)
#define RECOVERY_LOAD_IMG_ADDR (gd->ram_base + 0x400000)
#define RECOVERY_LOAD_IMG_SIZE (RECOVERY_LOAD_IMG_SIZE_MAX)

/* boot mode configs */
#define ASR_CIU_BASE		(0xD4282000)
#define SYS_BOOT_CNTRL		(ASR_CIU_BASE + 0x020)		/* System boot control register */
#define SQU_WR_SEC_ST		(ASR_CIU_BASE + 0x110)		/* Boot flag dummy register */
#define BOOT_DEV_FLAG_REG	SQU_WR_SEC_ST				/* For compatibility */
#define BOOT_PIN_SELECT		SQU_WR_SEC_ST
#define BOOT_STRAP_BIT_OFFSET	(9)
#define BOOT_STRAP_BIT_EMMC	(0x0)
#define BOOT_STRAP_BIT_NOR	(0x1)
#define BOOT_STRAP_BIT_NAND	(0x2)
#define BOOT_STRAP_BIT_UFS	(0x3)

/* Boot type mask and values */
#define BOOT_TYPE_MASK		0xfff

/* TLV code */
#define TLV_CODE_SDK_VERSION		0x40
#define TLV_CODE_DDR_CSNUM		0x41
#define TLV_CODE_DDR_TYPE		0x42
#define TLV_CODE_DDR_DATARATE		0x43
#define TLV_CODE_DDR_TX_ODT		0x44
#define TLV_CODE_WIFI_MAC_ADDR		0x60
#define TLV_CODE_BLUETOOTH_ADDR	0x61
#define TLV_CODE_PMIC_TYPE		0x80
#define TLV_CODE_EEPROM_I2C_INDEX	0x81
#define TLV_CODE_EEPROM_PIN_GROUP	0x82

#if defined(CONFIG_SPL_BUILD)
#define MMC_DEV_EMMC	(1)
#else
#define MMC_DEV_EMMC    (2)
#endif
#define MMC_DEV_SD	(0)

#define DEFAULT_PRODUCT_NAME	"k3_fpga_1x1"
#define BOOTFS_NAME	("bootfs")

#ifndef __ASSEMBLY__
enum board_boot_mode {
	BOOT_MODE_NONE = 0,
	BOOT_MODE_USB = 0x55a,
	BOOT_MODE_SHELL = 0x560,
	BOOT_MODE_UART = 0x66b,
	BOOT_MODE_EMMC = 0xb00,
	BOOT_MODE_NOR = 0xb01,
	BOOT_MODE_NAND = 0xb02,
	BOOT_MODE_UFS = 0xb03,
	BOOT_MODE_SD = 0xb10,
	BOOT_MODE_BOOTSTRAP,
};
#endif

#define ASR_DDR_TRAINING_DATA_BASE 0xc0829000

/* ****************************************************************************************
 * Environment
 * ***************************************************************************************/
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
