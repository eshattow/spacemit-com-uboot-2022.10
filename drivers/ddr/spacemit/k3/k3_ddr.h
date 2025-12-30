// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Spacemit
 */

#ifndef _K3_DDR_H_
#define _K3_DDR_H_

#include <stdio.h>

#define DDR_CTRL_REG_BASE	(0xCB000000)
#define DDR_CTRL1_REG_BASE	(0xCC000000)

// current only support 8GB or 16GB
#define DDR_SIZE_GB		(8)
// 2D training configuration
#define DISABLE_DDR_2D_TRAINING	(0)

#if (DISABLE_DDR_2D_TRAINING == 0)
#define AC_VREFDAC		(0x2F)
#define DQ_VREFDAC		(0x2F)
#else
#define AC_VREFDAC		(0x35)
#define DQ_VREFDAC		(0x35)
#endif

#if (DDR_SIZE_GB == 8)
// only support 1066MT/s @8GB
#define CONFIG_DDR_DATARATE	(1066)
#elif (DDR_SIZE_GB == 16)
// support 4266MT/s, 5120MT/s, 5500MT/s, 6000MT/s, 6400MT/s @16GB
#define CONFIG_DDR_DATARATE	(6400)
#else
#error "Unsupported DDR size"
#endif

#ifndef REG32
#define REG32(x) (*((volatile uint32_t*)((uintptr_t)(x))))
#endif

#define TRAINING_DEBUG	0
#define LOGLEVEL	0

#define LogMsg(level, format, args...)		\
	do {					\
		if (level < LOGLEVEL)		\
			printf(format, ##args);	\
	} while (0)

typedef struct {
	uint16_t offset;
	uint16_t value;
} discrete_sequence;

typedef struct {
	uint16_t value0;
	uint16_t value1;
} linear_sequence;

typedef struct {
	uint32_t base;
	uint32_t count;
	bool is_linear_increase;

	union {
		discrete_sequence a;
		linear_sequence b;
	} sequence[];
} phy_init_config;

extern const phy_init_config* lp5_pre_train_table[];
extern const phy_init_config* lp5_train_table[];

#endif /* _K3_DDR_H_ */
