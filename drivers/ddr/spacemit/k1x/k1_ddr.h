// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Spacemit
 */

#ifndef __K1_DDR_H__
#define __K1_DDR_H__

#ifndef REG32
#define REG32(x) (*((volatile uint32_t*)((uintptr_t)(x))))
#endif

#define	BOOT_PP			0

#define PMUA_REG_BASE		0xd4282800
#define PMUA_MCK_CTRL		(PMUA_REG_BASE + 0xe8)
#define PMUA_MC_HW_SLP_TYPE	(PMUA_REG_BASE + 0xb0)

#define MC_CH0_BASE		0x200
#define	MC_CH0_PHY_BASE		0x1000
#define	ANALOG_CONTROL_OFFSET	0x0
#define	OTHER_CONTROL_OFFSET	0x10000
#define	TRAINING_CONTROL_OFFSET	0x18000

#define	subPHY_A_OFFSET		0x0
#define	subPHY_B_OFFSET		0x200
#define	subPHY_C_OFFSET		0x400
#define	subPHY_D_OFFSET		0x600

#define	CS0_OFFSET		0x0
#define	CS1_OFFSET		0x100

#define	CAPHY_OFFSET		0x2000
#define	DATAPHY0_OFFSET		0x0
#define	DATAPHY1_OFFSET		0x1000
#define	COMMON_OFFSET		0x3000
#define	FREQ_POINT_OFFSET	0x4000

#define TOP_EXT_CLK_DIV_OFFSET		0
#define TOP_EXT_CLK_SEL_OFFSET		1
#define TOP_PLL2_DIV_OFFSET		2
#define TOP_PLL1_DIV_OFFSET		4
#define TOP_PLL1_2_SEL_OFFSET		6
#define TOP_PLL2_EN_OFFSET		8
#define TOP_PLL1_EN_OFFSET		9
#define TOP_DDRPHY1_EN_OFFSET		31
#define TOP_DDRPHY0_EN_OFFSET		30
#define TOP_DCLK_BYPASS_FC_REQ_OFFSET	23
#define TOP_DCLK_BYPASS_CLK_EN_OFFSET	22
#define TOP_DCLK_BYPASS_RST_OFFSET	21
#define TOP_DCLK_BYPASS_SEL_OFFSET	19
#define TOP_DCLK_BYPASS_DIV_OFFSET	16
#define TOP_MC_REG_TABLE_EN_OFFSET	10
#define TOP_FREQ_PLL_CHG_MODE_OFFSET	9
#define TOP_MC_REQ_TABLE_NUM_OFFSET	3
#define TOP_AP_ALLOW_SPD_CHG		18
#define TOP_DDR_FREQ_CHG_REQ		22

// emu for device density
typedef enum {
	BANK_2 = 0,
	BANK_4,
	BANK_8,
	BANK_RESERVED,
} bank_num;

typedef enum {
	ROW_11 = 1,
	ROW_12,
	ROW_13,
	ROW_14,
	ROW_15,
	ROW_16,
	ROW_17,
	ROW_18,
} row_num;

typedef enum {
	COL_8 = 1,
	COL_9,
	COL_10,
	COL_11,
	COL_12,
} col_num;

typedef enum {
	IO_X16 = 0,
	IO_X8,
} io_width;

// emu for IO parameter
typedef enum {
	DDR_MID_SAMSUNG		= 0x01, // 0000 0001B
	DDR_MID_QIMONDA		= 0x02, // 0000 0010B
	DDR_MID_ELPIDA		= 0x03, // 0000 0011B
	DDR_MID_NANYA		= 0x05, // 0000 0101B
	DDR_MID_SK_HYNIX	= 0x06, // 0000 0110B
	DDR_MID_WINBOND		= 0x08, // 0000 1000B
	DDR_MID_RESERVED	= 0x0A, // 0000 1010B
	DDR_MID_SPANSION	= 0x0B, // 0000 1011B
	DDR_MID_SST		= 0x0C, // 0000 1100B
	DDR_MID_CMXT		= 0x13, // 0001 0011B
	DDR_MID_UNIC		= 0x1A, // 0001 1010B
	DDR_MID_JSC		= 0x1C, // 0001 1100B
	DDR_MID_ESMT2		= 0xFD, // 1111 1101B
	DDR_MID_MICRON		= 0xFF, // 1111 1111B
	DDR_MID_TOSHIBA		= 0x03  // seems same as elpida ID
}ddr_manufacturer_id;

typedef enum {
	R_240 = 1,
	R_120,
	R_80,
	R_60,
	R_48,
	R_40,
}tx_ds_odt_rx_odt;

typedef enum {
	VOH_0P6 = 0x0,
	VOH_0P5,
}pu_cal;

typedef enum {
	LPDDR4X = 0x0,
	LPDDR4,
}device_type;

#define LOGLEVEL 0
#if (LOGLEVEL > 0)
#define LogMsg(level, format, args...)		\
	do {					\
		if (level < LOGLEVEL)		\
			printf(format, ##args);	\
	} while (0)
#else
#define LogMsg(level, format, args...)
#endif

#endif //__K1_DDR_H__
