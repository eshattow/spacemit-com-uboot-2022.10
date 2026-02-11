// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Spacemit
 */

#ifndef _K3_DDR_H_
#define _K3_DDR_H_

#include <stdio.h>

// current only support 4GB, 8GB or 16GB
// default is 8GB
#define DDR_SIZE_GB		(4)
// support 4266MT/s, 5500MT/s, 6000MT/s, 6400MT/s
#define CONFIG_DDR_DATARATE	(6400)

// 2D training configuration
#define DISABLE_DDR_2D_TRAINING	(0)

#if (DDR_SIZE_GB != 4) && (DDR_SIZE_GB != 8) && (DDR_SIZE_GB != 16)
#error "Unsupported DDR size"
#endif

#if (CONFIG_DDR_DATARATE != 6400) && (CONFIG_DDR_DATARATE != 6000) \
	&& (CONFIG_DDR_DATARATE != 5500) && (CONFIG_DDR_DATARATE != 4266)
#error "Unsupported DDR datarate"
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

typedef enum {
	DDR_TYPE_LPDDR4X = 0,
	DDR_TYPE_LPDDR5,
	DDR_TYPE_UNKNOWN
} ddr_part_type;

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
	uint16_t count;
	uint16_t is_linear_increase;

	union {
		discrete_sequence a;
		linear_sequence b;
	} sequence[];
} phy_init_config;

typedef struct {
	uint32_t offset;
	uint32_t value;
} ddr_phy_reg_config;

typedef struct {
	const char *part_number;
	uint32_t crc32_value;
	uint32_t type;
	uint32_t size_mb;
	uint32_t data_rate_mtps;
} ddr_part_info;

extern ddr_part_info* part_info;
extern const phy_init_config *lp5_pre_train_table[];
extern const phy_init_config *lp5_train_table[];
extern const phy_init_config *lp5_4g_pre_train_table[], *lp5_8g_pre_train_table[];
extern const phy_init_config *lp5_16g_5500_pre_train_table[], *lp5_16g_pre_train_table[];
extern const phy_init_config *lp5_4g_train_table[], *lp5_8g_train_table[];
extern const phy_init_config *lp5_16g_5500_train_table[], *lp5_16g_train_table[];

extern void fpga_ddr_init(void);
extern void lpddr5_silicon_init(uint64_t ddrc_reg_base, ddr_part_info* part_info);
extern int get_tlvinfo(uint8_t id, uint8_t *buffer, int max_size);
extern void lpddr_training_table_init(unsigned int ddrc_base,
	const phy_init_config* train_table[], ddr_phy_reg_config* override_table);
#endif /* _K3_DDR_H_ */
