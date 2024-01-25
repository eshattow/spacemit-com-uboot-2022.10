// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#ifndef _SPACEMIT_DPU_H_
#define _SPACEMIT_DPU_H_
#include <clk.h>
#include <reset.h>

#define DPU_INT_REG_24	0x960
#define DPU_INT_REG_14	0x938

#define OUTFMT_RGB121212	0
#define OUTFMT_RGB101010	1
#define OUTFMT_RGB888		2
#define OUTFMT_RGB666		12
#define OUTFMT_RGB565		13

enum dpu_modes {
	DPU_MODE_EDP = 0,
	DPU_MODE_MIPI,
	DPU_MODE_HDMI,
	DPU_MODE_LVDS,
	DPU_MODE_DP,
};

enum dpu_features {
	DPU_FEATURE_OUTPUT_10BIT = (1 << 0),
};

struct spacemit_dpu_priv {
	void __iomem * regs_dsi;
	void __iomem * regs_hdmi;
	struct udevice *conn_dev;
	struct display_timing timing;
};

struct spacemit_dpu_driverdata {
	/* configuration */
	u32 features;
	/* block-specific setters/getters */
	void (*set_pin_polarity)(struct udevice *, enum dpu_modes, u32);
};

struct spacemit_mode_modeinfo {
	const char *name;
	unsigned int refresh;
	unsigned int xres;
	unsigned int yres;
	unsigned int real_xres;
	unsigned int real_yres;
	unsigned int left_margin;
	unsigned int right_margin;
	unsigned int upper_margin;
	unsigned int lower_margin;
	unsigned int hsync_len;
	unsigned int vsync_len;
	unsigned int hsync_invert;
	unsigned int vsync_invert;
	unsigned int invert_pixclock;
	unsigned int pixclock_freq;
	int pix_fmt_out;
	uint32_t height; /* screen height in mm */
	uint32_t width; /* screen width in mm */
};

#endif
