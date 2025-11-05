/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025 Spacemit Co., Ltd.
 *
 */

#ifndef __INNO_EDID_H__
#define __INNO_EDID_H__

#include "inno_utils.h"
#include "inno_modes.h"

#define EDID_FEATURE_DEFAULT_GTF	(1 << 0)
#define EDID_FEATURE_PREFERRED_TIMING	(1 << 1)
#define EDID_FEATURE_STANDARD_COLOR	(1 << 2)

#define EDID_DETAIL_EST_TIMINGS		0xf7
#define EDID_DETAIL_CVT_3BYTE		0xf8
#define EDID_DETAIL_COLOR_MGMT_DATA	0xf9
#define EDID_DETAIL_STD_MODES		0xfa
#define EDID_DETAIL_MONITOR_CPDATA	0xfb
#define EDID_DETAIL_MONITOR_NAME	0xfc
#define EDID_DETAIL_MONITOR_RANGE	0xfd
#define EDID_DETAIL_MONITOR_STRING	0xfe
#define EDID_DETAIL_MONITOR_SERIAL	0xff

#define CEA_EXT				0x02
#define VTB_EXT				0x10
#define DI_EXT				0x40
#define LS_EXT				0x50
#define MI_EXT				0x60
#define DISPLAYID_EXT			0x70

/* 00=16:10, 01=4:3, 10=5:4, 11=16:9 */
#define EDID_TIMING_ASPECT_SHIFT	6
#define EDID_TIMING_ASPECT_MASK		(0x3 << EDID_TIMING_ASPECT_SHIFT)

/* need to add 60 */
#define EDID_TIMING_VFREQ_SHIFT		0
#define EDID_TIMING_VFREQ_MASK		(0x3f << EDID_TIMING_VFREQ_SHIFT)

#define EDID_EST_TIMINGS		16
#define EDID_STD_TIMINGS		8
#define EDID_DETAILED_TIMINGS		4
#define EDID_LENGTH			128
#define BLOCK0_DTD_START		0x36
#define BLOCK0_TOTAL_DTD		4
#define DTD_SIZE			18

struct est_timings {
	uint8_t t1;
	uint8_t t2;
	uint8_t mfg_rsvd;
} __attribute__((packed));

struct std_timing {
	uint8_t hsize; /* need to multiply by 8 then add 248 */
	uint8_t vfreq_aspect;
} __attribute__((packed));

/* If it's not pixel timing, it'll be one of the below */
struct detailed_data_string {
	uint8_t str[13];
} __attribute__((packed));

struct detailed_data_monitor_range {
	uint8_t min_vfreq;
	uint8_t max_vfreq;
	uint8_t min_hfreq_khz;
	uint8_t max_hfreq_khz;
	uint8_t pixel_clock_mhz; /* need to multiply by 10 */
	uint8_t flags;
	union {
		struct {
			uint8_t reserved;
			uint8_t hfreq_start_khz; /* need to multiply by 2 */
			uint8_t c; /* need to divide by 2 */
			uint16_t m;
			uint8_t k;
			uint8_t j; /* need to divide by 2 */
		} __attribute__((packed)) gtf2;
		struct {
			uint8_t version;
			uint8_t data1; /* high 6 bits: extra clock resolution */
			uint8_t data2; /* plus low 2 of above: max hactive */
			uint8_t supported_aspects;
			uint8_t flags; /* preferred aspect and blanking support */
			uint8_t supported_scalings;
			uint8_t preferred_refresh;
		} __attribute__((packed)) cvt;
	} formula;
} __attribute__((packed));

struct detailed_data_wpindex {
	uint8_t white_yx_lo; /* Lower 2 bits each */
	uint8_t white_x_hi;
	uint8_t white_y_hi;
	uint8_t gamma; /* need to divide by 100 then add 1 */
} __attribute__((packed));

struct detailed_data_color_point {
	uint8_t windex1;
	uint8_t wpindex1[3];
	uint8_t windex2;
	uint8_t wpindex2[3];
} __attribute__((packed));

struct cvt_timing {
	uint8_t code[3];
} __attribute__((packed));

struct detailed_non_pixel {
	uint8_t pad1;
	uint8_t type; /* ff=serial, fe=string, fd=monitor range, fc=monitor name
		    fb=color point data, fa=standard timing data,
		    f9=undefined, f8=mfg. reserved */
	uint8_t pad2;
	union {
		struct detailed_data_string str;
		struct detailed_data_monitor_range range;
		struct detailed_data_wpindex color;
		struct std_timing timings[6];
		struct cvt_timing cvt[4];
	} data;
} __attribute__((packed));

#define DRM_EDID_PT_HSYNC_POSITIVE (1 << 1)
#define DRM_EDID_PT_VSYNC_POSITIVE (1 << 2)
#define DRM_EDID_PT_SEPARATE_SYNC  (3 << 3)
#define DRM_EDID_PT_STEREO         (1 << 5)
#define DRM_EDID_PT_INTERLACED     (1 << 7)

struct detailed_pixel_timing {
	uint8_t hactive_lo;
	uint8_t hblank_lo;
	uint8_t hactive_hblank_hi;
	uint8_t vactive_lo;
	uint8_t vblank_lo;
	uint8_t vactive_vblank_hi;
	uint8_t hsync_offset_lo;
	uint8_t hsync_pulse_width_lo;
	uint8_t vsync_offset_pulse_width_lo;
	uint8_t hsync_vsync_offset_pulse_width_hi;
	uint8_t width_mm_lo;
	uint8_t height_mm_lo;
	uint8_t width_height_mm_hi;
	uint8_t hborder;
	uint8_t vborder;
	uint8_t misc;
} __attribute__((packed));

struct detailed_timing {
	uint16_t pixel_clock; /* need to multiply by 10 KHz */
	union {
		struct detailed_pixel_timing pixel_data;
		struct detailed_non_pixel other_data;
	} data;
} __attribute__((packed));

struct edid {
	uint8_t header[8];
	/* Vendor & product info */
	uint8_t mfg_id[2];
	uint8_t prod_code[2];
	uint32_t serial; /* FIXME: byte order */
	uint8_t mfg_week;
	uint8_t mfg_year;
	/* EDID version */
	uint8_t version;
	uint8_t revision;
	/* Display info: */
	uint8_t input;
	uint8_t width_cm;
	uint8_t height_cm;
	uint8_t gamma;
	uint8_t features;
	/* Color characteristics */
	uint8_t red_green_lo;
	uint8_t black_white_lo;
	uint8_t red_x;
	uint8_t red_y;
	uint8_t green_x;
	uint8_t green_y;
	uint8_t blue_x;
	uint8_t blue_y;
	uint8_t white_x;
	uint8_t white_y;
	/* Est. timings and mfg rsvd timings*/
	struct est_timings established_timings;
	/* Standard timings 1-8*/
	struct std_timing standard_timings[8];
	/* Detailing timings 1-4 */
	struct detailed_timing detailed_timings[4];
	/* Number of 128 byte ext. blocks */
	uint8_t extensions;
	/* Checksum */
	uint8_t checksum;
} __attribute__((packed));

bool inno_edid_is_valid(struct edid *edid);
int inno_edid_mode_add_list(const unsigned char *edid, struct list_head *modes);
int inno_edid_mode_free_list(struct list_head *modes);
bool inno_detect_hdmi_monitor(struct edid *edid);

uint8_t inno_mode_match_cea_mode(const struct inno_mode *to_match);
void inno_mode_copy_cea(struct inno_mode *dst, uint8_t vic);

#endif

