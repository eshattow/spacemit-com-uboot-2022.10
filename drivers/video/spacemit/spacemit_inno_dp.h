/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025 Spacemit Co., Ltd.
 *
 */

#ifndef _SPACEMIT_DP_H_
#define _SPACEMIT_DP_H_

#include <clk.h>
#include <reset.h>
#include "./dp/inno_dp_api.h"

enum spacemit_inno_dp_types {
	INNO_DP = 0,
	INNO_EDP
};

struct spacemit_inno_dp_priv {
	void __iomem *base;
	struct inno_conn_t *dp_conn;
	struct clk dp_mclk;
	struct reset_ctl dp_reset;
	enum spacemit_inno_dp_types dp_type;

	struct clk pxclk;
	struct clk mclk;
	struct clk hclk;
	struct clk dscclk;
	struct clk aclk;
	struct clk edp0pxclk;

	struct reset_ctl aclk_reset;
	struct reset_ctl mclk_reset;
	struct reset_ctl dscclk_reset;
	struct reset_ctl lcd_reset;
	struct reset_ctl edp0_reset;
};

#endif /* _SPACEMIT_INNO_DP_H_ */
