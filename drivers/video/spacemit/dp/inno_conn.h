/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025 Spacemit Co., Ltd.
 *
 */

#ifndef __INNO_CONN_H__
#define __INNO_CONN_H__

#include "inno_modes.h"

#define DP0_REGISTER_BASE_ADDRESS	0xcac84000
#define DP1_REGISTER_BASE_ADDRESS	0xcac88000
#define DP0_REGISTER_SIZE			0x4000
#define DP1_REGISTER_SIZE			0x4000

enum modules {
	INNO_CONN_NONE = -1,
	INNO_CONN_DP = 0,
	INNO_CONN_EDP = 1,
	INNO_CONN_MAX,
};

#define INNO_CONN_FLAG_NONE		(0x0)
#define INNO_CONN_FLAG_BIST		(0x1)

#define INNO_VIC_1920x1080		(16)
#define INNO_VIC_1920x1200		(0)
#define INNO_VIC_1280x720		(4)
#define INNO_VIC_1024x768		(11)
#define INNO_VIC_800x600		(0)
#define INNO_VIC_720x480		(3)
#define INNO_VIC_640x480		(1)

struct inno_conn_func_t {
	int (*init)(struct inno_conn_t *conn);
	void (*exit)(struct inno_conn_t *conn);
	int (*fini)(struct inno_conn_t *conn);
	int (*get_edid)(struct inno_conn_t *conn, uint8_t *buff);
	int (*show_edid)(struct inno_conn_t *conn, uint8_t *buff);
	int (*modeset)(struct inno_conn_t *conn, struct inno_mode *mode);
	bool (*detect)(struct inno_conn_t *conn);
	int (*enable)(struct inno_conn_t *conn);
	int (*disable)(struct inno_conn_t *conn);
};

struct inno_conn_t {
	int conn_id;
	int valid;
	int flag;
	uint32_t regbase;
	uint32_t regsize;
	int mem_fd;
	void *reg_mmap_addr;
	bool use_phy_board;
	int phy_i2c_id;
	int phy_i2c_fd;
	int lane_count;
	int lane_rate;

	int vic;
	uint32_t width; //need match with vic
	uint32_t height; //need match with vic

	bool edp_enable;
	struct inno_conn_func_t *func;
	struct inno_mode out_mode;
	bool edid_valid;
	uint8_t edid_data[256];

	void *priv;
	bool is_enable;
};

#endif /* __INNO_CONN_H__ */

