/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2022, Kevin.z.m <zhangmeng.kevin@spacemit.com>
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>


#ifdef CONFIG_K1_PRO_BOARD_QEMU
    #define RISCV_MMODE_TIMERBASE		0x2000000
    #define RISCV_MMODE_TIMER_FREQ		1000000
    #define RISCV_SMODE_TIMER_FREQ		1000000
#elif defined(CONFIG_K1_PRO_BOARD_FPGA) || defined(CONFIG_K1_PRO_BOARD_SIMULATION)
    #define RISCV_MMODE_TIMERBASE		0x2000000
    #define RISCV_MMODE_TIMER_FREQ		1000000
    #define RISCV_SMODE_TIMER_FREQ		1000000
#else
    #error "unknown k1-pro board defined"
#endif

#define CONFIG_IPADDR    10.0.92.253
#define CONFIG_SERVERIP  10.0.92.134
#define CONFIG_GATEWAYIP 10.0.92.1
#define CONFIG_NETMASK   255.255.255.0

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
	"kernel_addr_r=0x84000000\0" \
	"kernel_comp_addr_r=0x88000000\0" \
	"kernel_comp_size=0x4000000\0" \
	"fdt_addr_r=0x8c000000\0" \
	"scriptaddr=0x8c100000\0" \
	"pxefile_addr_r=0x8c200000\0" \
	"ramdisk_addr_r=0x8c300000\0" \
	"ethaddr=02:f6:c3:67:27:55\0" \
	BOOTENV

#endif /* __CONFIG_H */
