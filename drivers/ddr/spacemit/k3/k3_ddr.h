// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Spacemit
 */

#ifndef _K3_DDR_H_
#define _K3_DDR_H_

#include <common.h>
#include <linux/delay.h>
#include <linux/lzo.h>

#define DDR_CONFIG_BYPASS_MAGIC	(0xdeadbeef)

// support 4266MT/s, 5500MT/s, 6000MT/s, 6400MT/s
#define CONFIG_DDR_DATARATE	(6400)

// 2D training configuration
#define DISABLE_DDR_2D_TRAINING	(0)

#define MAX_MODIFIED_IO_PARA_ITEMS (256)

#define DDR_TRAINING_FIRMWARE_TABLE_ADDR (DDR_TRAINING_INFO_BUFF)

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
	uint8_t type;
	uint8_t ranks;
	uint8_t x8_mode;
	uint32_t size_mb;
	uint32_t data_rate_mtps;
} ddr_part_info;

typedef enum {
	PHY_R_OFF = 0,
	PHY_R_120 = 0x08,
	PHY_R_60 = 0x0C,
	PHY_R_40 = 0x0E,
	PHY_R_30 = 0x0F,
} phy_odt_e;

typedef enum {
	R_OFF = 0,
	R_240,
	R_120,
	R_80,
	R_60,
	R_48,
	R_40,
} ddr_odt_e;

typedef struct {
	uint8_t phy_write_ds;
	uint8_t phy_rx_odt;
	uint8_t dq_odt;
	uint8_t ca_odt;
	uint8_t nt_odt;
	uint8_t soc_odt;
	uint8_t pdds;
} ddr_config_t;

extern ddr_part_info* part_info;
extern const phy_init_config *lp5_pre_train_table[];
extern const phy_init_config *lp5_4g_train_table[], *lp5_train_table[];
extern const ddr_phy_reg_config phy_override_pre_seq_lp5_4g[], phy_override_pre_seq_lp5_16g[];
extern const ddr_phy_reg_config phy_override_seq_lp5_16g[];

extern const phy_init_config *lp4x_pre_train_table[];
extern const phy_init_config *lp4x_4g_train_table[], *lp4x_8g_train_table[], *lp4x_16g_train_table[];
extern const ddr_phy_reg_config phy_override_seq_lp4x_8g[], phy_override_seq_lp4x_16g[];

extern void lpddr_init_prepare(ddr_part_info* part_info);
extern int lp5_training_prepare(void);
extern int lp4x_training_prepare(void);

extern void fpga_ddr_init(void);
extern void lpddr_silicon_init(uint64_t ddrc_reg_base, ddr_part_info* part_info);
extern int get_tlvinfo(uint8_t id, uint8_t *buffer, int max_size);

extern uint32_t major_message_all(unsigned int dphy_base);
extern void lpddr_training_table_init(unsigned int ddrc_base, const phy_init_config* train_table[],
	const ddr_phy_reg_config* override_table, ddr_phy_reg_config* io_table);
extern void init_snps_lp4x_ddrc(unsigned DDRC_BASE, unsigned int rst_code, unsigned int ddr_size_mbyte);
#endif /* _K3_DDR_H_ */
