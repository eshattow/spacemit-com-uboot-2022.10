// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Spacemit Co., Ltd.
 *
 */

#include <stdio.h>
#include "inno_dp_api.h"
#include "inno_conn.h"
#include "inno_dp_reg.h"
#include "inno_dp.h"
#include "inno_edid.h"
#include "inno_dp_common.h"

extern struct inno_conn_func_t g_inno_dp_func;

struct inno_conn_t g_inno_conn_table[INNO_CONN_MAX] = {
	[INNO_CONN_DP0] = {
		.conn_id = INNO_CONN_DP0,
		.valid = true,
		.flag = INNO_CONN_FLAG_NONE,
		.regbase = DP_REGISTER_BASE_ADDRESS,
		.regsize = DP_REGISTER_SIZE,
		.use_phy_board = false,
		.phy_i2c_id = 0,
		.lane_count = 2, //support 2lanes
		.lane_rate = INNODP_LINK_BW_2_7,
		.vic  = INNO_VIC_1920x1200, //use vic=1080p when edid valid.
		.width = 1920,
		.height = 1200,
		.func = &g_inno_dp_func,
	},
};

struct inno_conn_t *inno_get_conn_module(enum modules module_id)
{
	if (module_id >= INNO_CONN_MAX)
		return NULL;

	return &g_inno_conn_table[module_id];
}

int inno_conn_init(struct inno_conn_t *conn)
{
	if (conn->func->init)
		conn->func->init(conn);
	return 0;
}

int inno_conn_prepare(struct inno_conn_t *conn)
{
	uint8_t  edid[256];
	struct list_head probed_modes;
	struct inno_mode *mode = NULL;
	int ret = 0;

	//get edid
	INIT_LIST_HEAD(&probed_modes);
	osal_memset(edid, 0x55, sizeof(edid));
	/* read edid to buff*/
	if (!conn->func->get_edid) {
		inno_mode_copy_cea(&conn->out_mode, conn->vic);
		return -1;
	}

	ret = conn->func->get_edid(conn, edid);
	if (ret || !inno_edid_is_valid((struct edid *)edid)) {
		inno_mode_copy_cea(&conn->out_mode, conn->vic);
		osal_printf_func("[%d]Edid Invalid\n", conn->conn_id);
		return -1;
	}
#if 0
	if (conn->func->show_edid) {
		if (conn->func->show_edid(conn, edid) < 0) {
			osal_printf_func("[%d]Show Edid failed\n", conn->conn_id);
			return -1;
		}
	}
#endif
	/* parse edid to mode list*/
	inno_edid_mode_add_list(edid, &probed_modes);

	/* sort it */
	inno_mode_sort(&probed_modes);

	mode = inno_mode_find_out_mode(conn->width, conn->height, &probed_modes);
	/* copy out_mode to conn */
	if (NULL == mode) {
	osal_printf_func("use default vic....\n");
	inno_mode_copy_cea(&conn->out_mode, conn->vic);
	} else {
	inno_mode_copy(&conn->out_mode, mode);
	}
	/* free mode list */
	inno_edid_mode_free_list(&probed_modes);

	osal_printf_func("vic mode clock: %d, h:%d, v:%d, vfresh:%d vtotal:%d, htotal: %d \n", conn->out_mode.clock,
		conn->out_mode.hdisplay, conn->out_mode.vdisplay, conn->out_mode.vrefresh,
		conn->out_mode.vtotal, conn->out_mode.htotal);

	if (!conn->is_enable && conn->func->modeset) {
		ret = conn->func->modeset(conn, &conn->out_mode);
		if (ret != 0) {
			osal_printf_func("[%d]modeset failed\n\n", conn->conn_id);
			return -1;
		}
	}

	return ret;
}

int inno_conn_enable(struct inno_conn_t *conn)
{
	int ret = 0;
	if (conn->func->disable)
		conn->func->disable(conn);

	if (!conn->is_enable && conn->func->enable) {
		ret = conn->func->enable(conn);
		if (ret != 0)
			osal_printf_func("[%d]enable failed\n\n", conn->conn_id);
	}

	return ret;
}

bool inno_conn_detect(struct inno_conn_t *conn)
{
	bool result = false;
	if (conn->func->detect)
		result = conn->func->detect(conn);

	return result;
}

int inno_conn_disable(struct inno_conn_t *conn)
{
	if (conn->is_enable && conn->func->disable)
		conn->func->disable(conn);
	return 0;
}

int inno_conn_unprepare(struct inno_conn_t *conn)
{
	return 0;
}

int inno_do_display(struct inno_conn_t *conn)
{
	uint8_t  edid[256];
	struct list_head probed_modes;
	struct inno_mode *mode = NULL;
	int ret = 0;


	//if (conn->func->exit)
	//  conn->func->exit(conn);

	//init modules
	if (conn->func->init)
		conn->func->init(conn);

	//get edid
	INIT_LIST_HEAD(&probed_modes);
	osal_memset(edid, 0x55, sizeof(edid));
	/* read edid to buff*/
	if (!conn->func->get_edid) {
		inno_mode_copy_cea(&conn->out_mode, conn->vic);
		return -1;
	}

	ret = conn->func->get_edid(conn, edid);
	if (ret || !inno_edid_is_valid((struct edid *)edid)) {
		inno_mode_copy_cea(&conn->out_mode, conn->vic);
		osal_printf_func("[%d]Edid Invalid\n", conn->conn_id);
		return -1;
	}
#if 0
	if (conn->func->show_edid) {
		if (conn->func->show_edid(conn, edid) < 0) {
			osal_printf_func("[%d]Show Edid failed\n", conn->conn_id);
			return -1;
		}
	}
#endif
	/* parse edid to mode list*/
	inno_edid_mode_add_list(edid, &probed_modes);

	/* sort it */
	inno_mode_sort(&probed_modes);

	mode = inno_mode_find_out_mode(conn->width, conn->height, &probed_modes);
	/* copy out_mode to conn */
	if (NULL == mode) {
		osal_printf_func("use default vic....\n");
		inno_mode_copy_cea(&conn->out_mode, conn->vic);
	} else {
		inno_mode_copy(&conn->out_mode, mode);
	}
	/* free mode list */
	inno_edid_mode_free_list(&probed_modes);

	osal_printf_func("vic mode clock: %d, h:%d, v:%d, vfresh:%d vtotal:%d, htotal: %d \n", conn->out_mode.clock,
			 conn->out_mode.hdisplay, conn->out_mode.vdisplay, conn->out_mode.vrefresh,
			 conn->out_mode.vtotal, conn->out_mode.htotal);

	if (!conn->is_enable && conn->func->modeset) {
		ret = conn->func->modeset(conn, &conn->out_mode);
		if (ret != 0) {
			osal_printf_func("[%d]modeset failed\n\n", conn->conn_id);
			return -1;
		}
	}

	if (conn->func->disable)
		conn->func->disable(conn);

	if (!conn->is_enable && conn->func->enable) {
		ret = conn->func->enable(conn);
		if (ret != 0) {
			osal_printf_func("[%d]enable failed\n\n", conn->conn_id);
			return -1;
		}
	}

	return 0;
}

void inno_conn_exit(struct inno_conn_t *conn)
{
	if (conn->func->exit)
		conn->func->exit(conn);
}
