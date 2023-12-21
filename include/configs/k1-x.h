/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2023, Spacemit
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#define SYS_DRAM_OFFS               0x00000000
#define SZ_1MB                      0x00100000
#define SZ_2GB                      0x80000000
#define SZ_4GB                      0x100000000ULL
#define SEC_IMG_SIZE                0x0000000
#define CONFIG_SYS_SDRAM_BASE       (SYS_DRAM_OFFS + SEC_IMG_SIZE)

#define RISCV_MMODE_TIMERBASE       0xE4000000
#define RISCV_MMODE_TIMER_FREQ      1000000
#define RISCV_SMODE_TIMER_FREQ      1000000

#define CONFIG_IPADDR               10.0.92.253
#define CONFIG_SERVERIP             10.0.92.134
#define CONFIG_GATEWAYIP            10.0.92.1
#define CONFIG_NETMASK              255.255.255.0

#define K1X_SPL_BOOT_LOAD_ADDR      (0x20200000)

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
#define BOOT_DEV_FLAG_REG (0xD4282D10)
#define BOOT_PIN_SELECT (0xD4282c20)

#define BOOT_STRAP_BIT_OFFSET (9)
#define BOOT_STRAP_BIT_STORAGE_MASK (0x3 << BOOT_STRAP_BIT_OFFSET)
#define BOOT_STRAP_BIT_EMMC (0x0)
#define BOOT_STRAP_BIT_NAND (0x1)
#define BOOT_STRAP_BIT_NOR  (0x2)
#define BOOT_STRAP_BIT_SD   (0x3)

#ifndef __ASSEMBLY__
enum board_boot_mode {
    BOOT_MODE_NONE = 0,
    BOOT_MODE_USB = 0x55a,
    BOOT_MODE_EMMC,
    BOOT_MODE_NAND,
    BOOT_MODE_NOR,
    BOOT_MODE_SD,
};
#endif

#define MMC_DEV_EMMC (1)
#define MMC_DEV_SD (0)

#define BOOTFS_NAME ("bootfs")

/* Environment options */

#define BOOT_TARGET_DEVICES(func) \
    func(QEMU, qemu, na)

#include <config_distro_bootcmd.h>

#define BOOTENV_DEV_QEMU(devtypeu, devtypel, instance) \
    "bootcmd_qemu=" \
    "if env exists kernel_start; then " \
    "bootm ${kernel_start} - ${fdtcontroladdr};" \
    "fi;\0"

#define BOOTENV_DEV_NAME_QEMU(devtypeu, devtypel, instance) \
    "qemu "

#define CONFIG_EXTRA_ENV_SETTINGS \
    "fdt_high=0xffffffffffffffff\0" \
    "initrd_high=0xffffffffffffffff\0" \
    "kernel_addr_r=0x24000000\0" \
    "kernel_comp_addr_r=0x28000000\0" \
    "kernel_comp_size=0x4000000\0" \
    "fdt_addr_r=0x2c000000\0" \
    "scriptaddr=0x2c100000\0" \
    "pxefile_addr_r=0x2c200000\0" \
    "ramdisk_addr_r=0x2c300000\0" \
    "ethaddr=02:f6:c3:67:27:55\0" \
    BOOTENV

#endif /* __CONFIG_H */
