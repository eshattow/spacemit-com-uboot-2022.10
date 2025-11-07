// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#include <linux/kernel.h>
#include "../../include/spacemit_dsi_common.h"
#include "../../include/spacemit_video_tx.h"
#include <linux/delay.h>

#define UNLOCK_DELAY 0

struct spacemit_mode_modeinfo co5300_spacemit_modelist[] = {
	{
		.name = "466x466-60",
		.refresh = 60,
		.xres = 466,
		.yres = 466,
		.real_xres = 466,
		.real_yres = 466,
		.left_margin = 120,
		.right_margin = 40,
		.hsync_len = 10,
		.upper_margin = 10,
		.lower_margin = 5,
		.vsync_len = 2,
		.hsync_invert = 0,
		.vsync_invert = 0,
		.invert_pixclock = 0,
		.pixclock_freq = 20*1000,
		.pix_fmt_out = OUTFMT_RGB888,
		.width = 34,
		.height = 34,
	},
};

struct spacemit_mipi_info co5300_mipi_info = {
	.height = 466,
	.width = 466,
	.hfp = 40, /* unit: pixel */
	.hbp = 120,
	.hsync = 10,
	.vfp = 5, /* unit: line */
	.vbp = 10,
	.vsync = 2,
	.fps = 60,

	// .work_mode = SPACEMIT_DSI_MODE_CMD, /* command_mode */
	.work_mode = SPACEMIT_DSI_MODE_VIDEO, /* video_mode */
	.rgb_mode = DSI_INPUT_DATA_RGB_MODE_888,
	.lane_number = 1,
	.phy_bit_clock = 500000000,
	.phy_esc_clock = 51200000,
	.split_enable = 0,
	.eotp_enable = 0,

	.burst_mode = DSI_BURST_MODE_NON_BURST_SYNC_PULSE,

	.te_enable = 0,
	.vsync_pol = 0,
	.te_pol = 0,
	.te_mode = 0,
};

static struct spacemit_dsi_cmd_desc co5300_set_id_cmds[] = {
	{SPACEMIT_DSI_SET_MAX_PKT_SIZE, SPACEMIT_DSI_LP_MODE, UNLOCK_DELAY, 1, {0x01}},
};

static struct spacemit_dsi_cmd_desc co5300_read_id_cmds[] = {
	{SPACEMIT_DSI_GENERIC_READ1, SPACEMIT_DSI_LP_MODE, UNLOCK_DELAY, 3, {0xDA, 0xDB, 0xDC}},
};

static struct spacemit_dsi_cmd_desc co5300_set_power_cmds[] = {
	{SPACEMIT_DSI_SET_MAX_PKT_SIZE, SPACEMIT_DSI_HS_MODE, UNLOCK_DELAY, 1, {0x1}},
};

static struct spacemit_dsi_cmd_desc co5300_read_power_cmds[] = {
	{SPACEMIT_DSI_GENERIC_READ1, SPACEMIT_DSI_HS_MODE, UNLOCK_DELAY, 1, {0xA}},
};

static struct spacemit_dsi_cmd_desc co5300_init_cmds[] = {
	{SPACEMIT_DSI_DCS_SWRITE1, SPACEMIT_DSI_LP_MODE, 0,   2, {0xFE,0x00}},
	{SPACEMIT_DSI_DCS_SWRITE1, SPACEMIT_DSI_LP_MODE, 0,   2, {0x35,0x00}},
	{SPACEMIT_DSI_DCS_SWRITE1, SPACEMIT_DSI_LP_MODE, 0,   2, {0x53,0x20}},
	{SPACEMIT_DSI_DCS_SWRITE1, SPACEMIT_DSI_LP_MODE, 0,   2, {0x51,0xFF}},
	{SPACEMIT_DSI_DCS_SWRITE1, SPACEMIT_DSI_LP_MODE, 0,   2, {0x63,0xFF}},
	{SPACEMIT_DSI_DCS_LWRITE, SPACEMIT_DSI_LP_MODE, 0,   5, {0x2A,0x00,0x06,0x01,0xD7}},
	{SPACEMIT_DSI_DCS_LWRITE, SPACEMIT_DSI_LP_MODE, 0,   5, {0x2B,0x00,0x00,0x01,0xD1}},

	{SPACEMIT_DSI_DCS_SWRITE, SPACEMIT_DSI_LP_MODE, 120, 1, {0x11}},
	{SPACEMIT_DSI_DCS_SWRITE, SPACEMIT_DSI_LP_MODE,  50, 1, {0x29}},
};

static struct spacemit_dsi_cmd_desc co5300_sleep_out_cmds[] = {
	{SPACEMIT_DSI_DCS_SWRITE,SPACEMIT_DSI_LP_MODE,120,1,{0x11}},
	{SPACEMIT_DSI_DCS_SWRITE,SPACEMIT_DSI_LP_MODE,50,1,{0x29}},
};

static struct spacemit_dsi_cmd_desc co5300_sleep_in_cmds[] = {
	{SPACEMIT_DSI_DCS_SWRITE,SPACEMIT_DSI_LP_MODE,120,1,{0x28}},
	{SPACEMIT_DSI_DCS_SWRITE,SPACEMIT_DSI_LP_MODE,50,1,{0x10}},
};


struct lcd_mipi_panel_info lcd_co5300 = {
	.lcd_name = "co5300",
	.lcd_id = 0x5300,
	.panel_id0 = 0x00,
	.power_value = 0x9c,
	.panel_type = LCD_MIPI,
	.width_mm = 34,
	.height_mm = 34,
	.dft_pwm_bl = 128,
	.set_id_cmds_num = ARRAY_SIZE(co5300_set_id_cmds),
	.read_id_cmds_num = ARRAY_SIZE(co5300_read_id_cmds),
	.init_cmds_num = ARRAY_SIZE(co5300_init_cmds),
	.set_power_cmds_num = ARRAY_SIZE(co5300_set_power_cmds),
	.read_power_cmds_num = ARRAY_SIZE(co5300_read_power_cmds),
	.sleep_out_cmds_num = ARRAY_SIZE(co5300_sleep_out_cmds),
	.sleep_in_cmds_num = ARRAY_SIZE(co5300_sleep_in_cmds),
	//.drm_modeinfo = co5300_modelist,
	.spacemit_modeinfo = co5300_spacemit_modelist,
	.mipi_info = &co5300_mipi_info,
	.set_id_cmds = co5300_set_id_cmds,
	.read_id_cmds = co5300_read_id_cmds,
	.set_power_cmds = co5300_set_power_cmds,
	.read_power_cmds = co5300_read_power_cmds,
	.init_cmds = co5300_init_cmds,
	.sleep_out_cmds = co5300_sleep_out_cmds,
	.sleep_in_cmds = co5300_sleep_in_cmds,
	.bitclk_sel = 3,
	.bitclk_div = 1,
	.pxclk_sel = 2,
	.pxclk_div = 6,
};

int lcd_co5300_init(void)
{
	int ret;

	ret = lcd_mipi_register_panel(&lcd_co5300);
	return ret;
}
