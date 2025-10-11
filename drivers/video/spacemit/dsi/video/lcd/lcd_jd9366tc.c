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

struct spacemit_mode_modeinfo jd9366tc_spacemit_modelist[] = {
	{
		.name = "800x1280-60",
		.refresh = 60,
		.xres = 800,
		.yres = 1280,
		.real_xres = 800,
		.real_yres = 1280,
		.left_margin = 28,
		.right_margin = 40,
		.hsync_len = 8,
		.upper_margin = 38,
		.lower_margin = 140,
		.vsync_len = 8,
		.hsync_invert = 0,
		.vsync_invert = 0,
		.invert_pixclock = 0,
		.pixclock_freq = 78*1000,
		.pix_fmt_out = OUTFMT_RGB888,
		.width = 108,
		.height = 172,
	},
};

struct spacemit_mipi_info jd9366tc_mipi_info = {
	.height = 1280,
	.width = 800,
	.hfp = 40, /* unit: pixel */
	.hbp = 28,
	.hsync = 8,
	.vfp = 140, /* unit: line */
	.vbp = 38,
	.vsync = 8,
	.fps = 60,

	.work_mode = SPACEMIT_DSI_MODE_VIDEO, /*command_mode, video_mode*/
	.rgb_mode = DSI_INPUT_DATA_RGB_MODE_888,
	.lane_number = 2,
	.phy_bit_clock = 1000000000,
	.phy_esc_clock = 51200000,
	.split_enable = 0,
	.eotp_enable = 0,

	.burst_mode = DSI_BURST_MODE_BURST,
};

static struct spacemit_dsi_cmd_desc jd9366tc_set_id_cmds[] = {
	{SPACEMIT_DSI_SET_MAX_PKT_SIZE, SPACEMIT_DSI_LP_MODE, UNLOCK_DELAY, 1, {0x01}},
};

static struct spacemit_dsi_cmd_desc jd9366tc_read_id_cmds[] = {
	{SPACEMIT_DSI_GENERIC_READ1, SPACEMIT_DSI_LP_MODE, UNLOCK_DELAY, 1, {0x04}},
};

static struct spacemit_dsi_cmd_desc jd9366tc_set_power_cmds[] = {
	{SPACEMIT_DSI_SET_MAX_PKT_SIZE, SPACEMIT_DSI_HS_MODE, UNLOCK_DELAY, 1, {0x1}},
};

static struct spacemit_dsi_cmd_desc jd9366tc_read_power_cmds[] = {
	{SPACEMIT_DSI_GENERIC_READ1, SPACEMIT_DSI_HS_MODE, UNLOCK_DELAY, 1, {0xA}},
};

static struct spacemit_dsi_cmd_desc jd9366tc_init_cmds[] = {
	{SPACEMIT_DSI_DCS_LWRITE, SPACEMIT_DSI_LP_MODE, 0, 2, {0x30, 0x01}},
	{SPACEMIT_DSI_DCS_LWRITE, SPACEMIT_DSI_LP_MODE, 0, 5, {0x78, 0x49, 0x61, 0x02, 0x00}},
	{SPACEMIT_DSI_DCS_LWRITE, SPACEMIT_DSI_LP_MODE, 0, 2, {0x30, 0x00}},
	{SPACEMIT_DSI_DCS_LWRITE, SPACEMIT_DSI_LP_MODE, 120, 2, {0x11, 0x00}},
	{SPACEMIT_DSI_DCS_LWRITE, SPACEMIT_DSI_LP_MODE, 0, 2, {0x30, 0x08}},
	{SPACEMIT_DSI_DCS_LWRITE, SPACEMIT_DSI_LP_MODE, 0, 2, {0x5D, 0x0D}},
	{SPACEMIT_DSI_DCS_LWRITE, SPACEMIT_DSI_LP_MODE, 0, 2, {0x5F, 0x01}},
	{SPACEMIT_DSI_DCS_LWRITE, SPACEMIT_DSI_LP_MODE, 0, 2, {0x30, 0x00}},
	{SPACEMIT_DSI_DCS_LWRITE, SPACEMIT_DSI_LP_MODE, 20, 2, {0x29, 0x00}},
};

static struct spacemit_dsi_cmd_desc jd9366tc_sleep_out_cmds[] = {
	{SPACEMIT_DSI_DCS_SWRITE,SPACEMIT_DSI_LP_MODE,120,1,{0x11}},
	{SPACEMIT_DSI_DCS_SWRITE,SPACEMIT_DSI_LP_MODE,20,1,{0x29}},
};

static struct spacemit_dsi_cmd_desc jd9366tc_sleep_in_cmds[] = {
	{SPACEMIT_DSI_DCS_SWRITE,SPACEMIT_DSI_LP_MODE,20,1,{0x28}},
	{SPACEMIT_DSI_DCS_SWRITE,SPACEMIT_DSI_LP_MODE,120,1,{0x10}},
};


struct lcd_mipi_panel_info lcd_jd9366tc = {
	.lcd_name = "jd9366tc",
	.lcd_id = 0x9366,
	.panel_id0 = 0x66,
	.power_value = 0x9c,
	.panel_type = LCD_MIPI,
	.width_mm = 108,
	.height_mm = 172,
	.dft_pwm_bl = 128,
	.set_id_cmds_num = ARRAY_SIZE(jd9366tc_set_id_cmds),
	.read_id_cmds_num = ARRAY_SIZE(jd9366tc_read_id_cmds),
	.init_cmds_num = ARRAY_SIZE(jd9366tc_init_cmds),
	.set_power_cmds_num = ARRAY_SIZE(jd9366tc_set_power_cmds),
	.read_power_cmds_num = ARRAY_SIZE(jd9366tc_read_power_cmds),
	.sleep_out_cmds_num = ARRAY_SIZE(jd9366tc_sleep_out_cmds),
	.sleep_in_cmds_num = ARRAY_SIZE(jd9366tc_sleep_in_cmds),
	//.drm_modeinfo = jd9366tc_modelist,
	.spacemit_modeinfo = jd9366tc_spacemit_modelist,
	.mipi_info = &jd9366tc_mipi_info,
	.set_id_cmds = jd9366tc_set_id_cmds,
	.read_id_cmds = jd9366tc_read_id_cmds,
	.set_power_cmds = jd9366tc_set_power_cmds,
	.read_power_cmds = jd9366tc_read_power_cmds,
	.init_cmds = jd9366tc_init_cmds,
	.sleep_out_cmds = jd9366tc_sleep_out_cmds,
	.sleep_in_cmds = jd9366tc_sleep_in_cmds,
	.bitclk_sel = 3,
	.bitclk_div = 1,
	.pxclk_sel = 2,
	.pxclk_div = 6,
};

int lcd_jd9366tc_init(void)
{
	int ret;

	ret = lcd_mipi_register_panel(&lcd_jd9366tc);
	return ret;
}
