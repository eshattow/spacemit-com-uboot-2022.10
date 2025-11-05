/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025 Spacemit Co., Ltd.
 *
 */

#ifndef __INNO_MODE_H__
#define __INNO_MODE_H__

#include "inno_utils.h"

#define INNO_MODE_LEN	32

#define DRM_MODE_FLAG_PHSYNC			(1<<0)
#define DRM_MODE_FLAG_NHSYNC			(1<<1)
#define DRM_MODE_FLAG_PVSYNC			(1<<2)
#define DRM_MODE_FLAG_NVSYNC			(1<<3)
#define DRM_MODE_FLAG_INTERLACE			(1<<4)
#define DRM_MODE_FLAG_DBLSCAN			(1<<5)
#define DRM_MODE_FLAG_CSYNC			(1<<6)
#define DRM_MODE_FLAG_PCSYNC			(1<<7)
#define DRM_MODE_FLAG_NCSYNC			(1<<8)
#define DRM_MODE_FLAG_HSKEW			(1<<9) /* hskew provided */
#define DRM_MODE_FLAG_BCAST			(1<<10) /* deprecated */
#define DRM_MODE_FLAG_PIXMUX			(1<<11) /* deprecated */
#define DRM_MODE_FLAG_DBLCLK			(1<<12)
#define DRM_MODE_FLAG_CLKDIV2			(1<<13)

#define DRM_MODE_FLAG_3D_MASK			(0x1f<<14)
#define DRM_MODE_FLAG_3D_NONE			(0<<14)
#define DRM_MODE_FLAG_3D_FRAME_PACKING		(1<<14)
#define DRM_MODE_FLAG_3D_FIELD_ALTERNATIVE	(2<<14)
#define DRM_MODE_FLAG_3D_LINE_ALTERNATIVE	(3<<14)
#define DRM_MODE_FLAG_3D_SIDE_BY_SIDE_FULL	(4<<14)
#define DRM_MODE_FLAG_3D_L_DEPTH		(5<<14)
#define DRM_MODE_FLAG_3D_L_DEPTH_GFX_GFX_DEPTH	(6<<14)
#define DRM_MODE_FLAG_3D_TOP_AND_BOTTOM	(7<<14)
#define DRM_MODE_FLAG_3D_SIDE_BY_SIDE_HALF	(8<<14)

#define DRM_MODE_TYPE_BUILTIN	(1<<0) /* deprecated */
#define DRM_MODE_TYPE_CLOCK_C	((1<<1) | DRM_MODE_TYPE_BUILTIN) /* deprecated */
#define DRM_MODE_TYPE_CRTC_C	((1<<2) | DRM_MODE_TYPE_BUILTIN) /* deprecated */
#define DRM_MODE_TYPE_PREFERRED	(1<<3)
#define DRM_MODE_TYPE_DEFAULT	(1<<4) /* deprecated */
#define DRM_MODE_TYPE_USERDEF	(1<<5)
#define DRM_MODE_TYPE_DRIVER	(1<<6)
#define DRM_MODE_TYPE_ALL	(DRM_MODE_TYPE_PREFERRED |	\
				 DRM_MODE_TYPE_USERDEF |	\
				 DRM_MODE_TYPE_DRIVER)

#define DRM_MODE_MATCH_TIMINGS (1 << 0)
#define DRM_MODE_MATCH_CLOCK (1 << 1)
#define DRM_MODE_MATCH_FLAGS (1 << 2)
#define DRM_MODE_MATCH_3D_FLAGS (1 << 3)
#define DRM_MODE_MATCH_ASPECT_RATIO (1 << 4)

enum inno_mode_status {
	MODE_OK = 0,
	MODE_HSYNC,
	MODE_VSYNC,
	MODE_H_ILLEGAL,
	MODE_V_ILLEGAL,
	MODE_BAD_WIDTH,
	MODE_NOMODE,
	MODE_NO_INTERLACE,
	MODE_NO_DBLESCAN,
	MODE_NO_VSCAN,
	MODE_MEM,
	MODE_VIRTUAL_X,
	MODE_VIRTUAL_Y,
	MODE_MEM_VIRT,
	MODE_NOCLOCK,
	MODE_CLOCK_HIGH,
	MODE_CLOCK_LOW,
	MODE_CLOCK_RANGE,
	MODE_BAD_HVALUE,
	MODE_BAD_VVALUE,
	MODE_BAD_VSCAN,
	MODE_HSYNC_NARROW,
	MODE_HSYNC_WIDE,
	MODE_HBLANK_NARROW,
	MODE_HBLANK_WIDE,
	MODE_VSYNC_NARROW,
	MODE_VSYNC_WIDE,
	MODE_VBLANK_NARROW,
	MODE_VBLANK_WIDE,
	MODE_PANEL,
	MODE_INTERLACE_WIDTH,
	MODE_ONE_WIDTH,
	MODE_ONE_HEIGHT,
	MODE_ONE_SIZE,
	MODE_NO_REDUCED,
	MODE_NO_STEREO,
	MODE_NO_420,
	MODE_STALE = -3,
	MODE_BAD = -2,
	MODE_ERROR = -1
};

#define DATA_BLOCK_PRODUCT_ID 0x00
#define DATA_BLOCK_DISPLAY_PARAMETERS 0x01
#define DATA_BLOCK_COLOR_CHARACTERISTICS 0x02
#define DATA_BLOCK_TYPE_1_DETAILED_TIMING 0x03
#define DATA_BLOCK_TYPE_2_DETAILED_TIMING 0x04
#define DATA_BLOCK_TYPE_3_SHORT_TIMING 0x05
#define DATA_BLOCK_TYPE_4_DMT_TIMING 0x06
#define DATA_BLOCK_VESA_TIMING 0x07
#define DATA_BLOCK_CEA_TIMING 0x08
#define DATA_BLOCK_VIDEO_TIMING_RANGE 0x09
#define DATA_BLOCK_PRODUCT_SERIAL_NUMBER 0x0a
#define DATA_BLOCK_GP_ASCII_STRING 0x0b
#define DATA_BLOCK_DISPLAY_DEVICE_DATA 0x0c
#define DATA_BLOCK_INTERFACE_POWER_SEQUENCING 0x0d
#define DATA_BLOCK_TRANSFER_CHARACTERISTICS 0x0e
#define DATA_BLOCK_DISPLAY_INTERFACE 0x0f
#define DATA_BLOCK_STEREO_DISPLAY_INTERFACE 0x10
#define DATA_BLOCK_TILED_DISPLAY 0x12
#define DATA_BLOCK_CTA 0x81

#define DRM_MODE_FMT    "\"%s\": %d %d %d %d %d %d %d %d %d %d 0x%x 0x%x"

#define DRM_MODE_ARG(m) \
	(m)->name, (m)->vrefresh, (m)->clock, \
	(m)->hdisplay, (m)->hsync_start, (m)->hsync_end, (m)->htotal, \
	(m)->vdisplay, (m)->vsync_start, (m)->vsync_end, (m)->vtotal, \
	(m)->type, (m)->flags

#define DRM_MODE(nm, t, c, hd, hss, hse, ht, hsk, vd, vss, vse, vt, vs, f) \
	.name = nm, .status = 0, .type = (t), .clock = (c), \
	.hdisplay = (hd), .hsync_start = (hss), .hsync_end = (hse), \
	.htotal = (ht), .hskew = (hsk), .vdisplay = (vd), \
	.vsync_start = (vss), .vsync_end = (vse), .vtotal = (vt), \
	.vscan = (vs), .flags = (f)

#define do_div(n, base) ({                                              \
        unsigned int __base = (base);                                   \
        unsigned int __rem;                                             \
        __rem = ((unsigned long long)(n)) % __base;                     \
        (n) = ((unsigned long long)(n)) / __base;                       \
        __rem;                                                          \
})

#define PICOS2KHZ(a) (1000000000UL/(a))
#define KHZ2PICOS(a) (1000000000UL/(a))

struct displayid_hdr {
	uint8_t rev;
	uint8_t bytes;
	uint8_t prod_id;
	uint8_t ext_count;
};

struct displayid_block {
	uint8_t tag;
	uint8_t rev;
	uint8_t num_bytes;
};

#define for_each_displayid_db(displayid, block, idx, length) \
	for ((block) = (struct displayid_block *)&(displayid)[idx]; \
	     (idx) + sizeof(struct displayid_block) <= (length) && \
	     (idx) + sizeof(struct displayid_block) + (block)->num_bytes <= (length) && \
	     (block)->num_bytes > 0; \
	     (idx) += (block)->num_bytes + sizeof(struct displayid_block), \
	     (block) = (struct displayid_block *)&(displayid)[idx])

enum hdmi_picture_aspect {
	HDMI_PICTURE_ASPECT_NONE,
	HDMI_PICTURE_ASPECT_4_3,
	HDMI_PICTURE_ASPECT_16_9,
	HDMI_PICTURE_ASPECT_64_27,
	HDMI_PICTURE_ASPECT_256_135,
	HDMI_PICTURE_ASPECT_RESERVED,
};

/**
 * struct drm_inno_mode - DRM kernel-internal display mode structure
 * @hdisplay: horizontal display size
 * @hsync_start: horizontal sync start
 * @hsync_end: horizontal sync end
 * @htotal: horizontal total size
 * @hskew: horizontal skew?!
 * @vdisplay: vertical display size
 * @vsync_start: vertical sync start
 * @vsync_end: vertical sync end
 * @vtotal: vertical total size
 * @vscan: vertical scan?!
 * @crtc_hdisplay: hardware mode horizontal display size
 * @crtc_hblank_start: hardware mode horizontal blank start
 * @crtc_hblank_end: hardware mode horizontal blank end
 * @crtc_hsync_start: hardware mode horizontal sync start
 * @crtc_hsync_end: hardware mode horizontal sync end
 * @crtc_htotal: hardware mode horizontal total size
 * @crtc_hskew: hardware mode horizontal skew?!
 * @crtc_vdisplay: hardware mode vertical display size
 * @crtc_vblank_start: hardware mode vertical blank start
 * @crtc_vblank_end: hardware mode vertical blank end
 * @crtc_vsync_start: hardware mode vertical sync start
 * @crtc_vsync_end: hardware mode vertical sync end
 * @crtc_vtotal: hardware mode vertical total size
 *
 * The horizontal and vertical timings are defined per the following diagram.
 *
 * ::
 *
 *
 *               Active                 Front           Sync           Back
 *              Region                 Porch                          Porch
 *     <-----------------------><----------------><-------------><-------------->
 *       //////////////////////|
 *      ////////////////////// |
 *     //////////////////////  |..................               ................
 *                                                _______________
 *     <----- [hv]display ----->
 *     <------------- [hv]sync_start ------------>
 *     <--------------------- [hv]sync_end --------------------->
 *     <-------------------------------- [hv]total ----------------------------->*
 *
 * This structure contains two copies of timings. First are the plain timings,
 * which specify the logical mode, as it would be for a progressive 1:1 scanout
 * at the refresh rate userspace can observe through vblank timestamps. Then
 * there's the hardware timings, which are corrected for interlacing,
 * double-clocking and similar things. They are provided as a convenience, and
 * can be appropriately computed using drm_mode_set_crtcinfo().
 *
 * For printing you can use %DRM_MODE_FMT and DRM_MODE_ARG().
 */
struct inno_mode {
	/**
	 * @head:
	 *
	 * struct list_head for mode lists.
	 */
	struct list_head head;

	/**
	 * @name:
	 *
	 * Human-readable name of the mode, filled out with drm_mode_set_name().
	 */
	char name[INNO_MODE_LEN];

	/**
	 * @status:
	 *
	 * Status of the mode, used to filter out modes not supported by the
	 * hardware. See enum &drm_mode_status.
	 */
	enum inno_mode_status status;

	/**
	 * @type:
	 *
	 * A bitmask of flags, mostly about the source of a mode. Possible flags
	 * are:
	 *
	 *  - DRM_MODE_TYPE_PREFERRED: Preferred mode, usually the native
	 *    resolution of an LCD panel. There should only be one preferred
	 *    mode per connector at any given time.
	 *  - DRM_MODE_TYPE_DRIVER: Mode created by the driver, which is all of
	 *    them really. Drivers must set this bit for all modes they create
	 *    and expose to userspace.
	 *  - DRM_MODE_TYPE_USERDEF: Mode defined via kernel command line
	 *
	 * Plus a big list of flags which shouldn't be used at all, but are
	 * still around since these flags are also used in the userspace ABI.
	 * We no longer accept modes with these types though:
	 *
	 *  - DRM_MODE_TYPE_BUILTIN: Meant for hard-coded modes, unused.
	 *    Use DRM_MODE_TYPE_DRIVER instead.
	 *  - DRM_MODE_TYPE_DEFAULT: Again a leftover, use
	 *    DRM_MODE_TYPE_PREFERRED instead.
	 *  - DRM_MODE_TYPE_CLOCK_C and DRM_MODE_TYPE_CRTC_C: Define leftovers
	 *    which are stuck around for hysterical raisins only. No one has an
	 *    idea what they were meant for. Don't use.
	 */
	unsigned int type;

	/**
	 * @clock:
	 *
	 * Pixel clock in kHz.
	 */
	int clock;		/* in kHz */
	int hdisplay;
	int hsync_start;
	int hsync_end;
	int htotal;
	int hskew;
	int vdisplay;
	int vsync_start;
	int vsync_end;
	int vtotal;
	int vscan;
	/**
	 * @flags:
	 *
	 * Sync and timing flags:
	 *
	 *  - DRM_MODE_FLAG_PHSYNC: horizontal sync is active high.
	 *  - DRM_MODE_FLAG_NHSYNC: horizontal sync is active low.
	 *  - DRM_MODE_FLAG_PVSYNC: vertical sync is active high.
	 *  - DRM_MODE_FLAG_NVSYNC: vertical sync is active low.
	 *  - DRM_MODE_FLAG_INTERLACE: mode is interlaced.
	 *  - DRM_MODE_FLAG_DBLSCAN: mode uses doublescan.
	 *  - DRM_MODE_FLAG_CSYNC: mode uses composite sync.
	 *  - DRM_MODE_FLAG_PCSYNC: composite sync is active high.
	 *  - DRM_MODE_FLAG_NCSYNC: composite sync is active low.
	 *  - DRM_MODE_FLAG_HSKEW: hskew provided (not used?).
	 *  - DRM_MODE_FLAG_BCAST: <deprecated>
	 *  - DRM_MODE_FLAG_PIXMUX: <deprecated>
	 *  - DRM_MODE_FLAG_DBLCLK: double-clocked mode.
	 *  - DRM_MODE_FLAG_CLKDIV2: half-clocked mode.
	 *
	 * Additionally there's flags to specify how 3D modes are packed:
	 *
	 *  - DRM_MODE_FLAG_3D_NONE: normal, non-3D mode.
	 *  - DRM_MODE_FLAG_3D_FRAME_PACKING: 2 full frames for left and right.
	 *  - DRM_MODE_FLAG_3D_FIELD_ALTERNATIVE: interleaved like fields.
	 *  - DRM_MODE_FLAG_3D_LINE_ALTERNATIVE: interleaved lines.
	 *  - DRM_MODE_FLAG_3D_SIDE_BY_SIDE_FULL: side-by-side full frames.
	 *  - DRM_MODE_FLAG_3D_L_DEPTH: ?
	 *  - DRM_MODE_FLAG_3D_L_DEPTH_GFX_GFX_DEPTH: ?
	 *  - DRM_MODE_FLAG_3D_TOP_AND_BOTTOM: frame split into top and bottom
	 *    parts.
	 *  - DRM_MODE_FLAG_3D_SIDE_BY_SIDE_HALF: frame split into left and
	 *    right parts.
	 */
	unsigned int flags;

	/**
	 * @width_mm:
	 *
	 * Addressable size of the output in mm, projectors should set this to
	 * 0.
	 */
	int width_mm;

	/**
	 * @height_mm:
	 *
	 * Addressable size of the output in mm, projectors should set this to
	 * 0.
	 */
	int height_mm;

	/**
	 * @private:
	 *
	 * Pointer for driver private data. This can only be used for mode
	 * objects passed to drivers in modeset operations. It shouldn't be used
	 * by atomic drivers since they can store any additional data by
	 * subclassing state structures.
	 */
	int *private;

	/**
	 * @private_flags:
	 *
	 * Similar to @private, but just an integer.
	 */
	int private_flags;

	/**
	 * @vrefresh:
	 *
	 * Vertical refresh rate, for debug output in human readable form. Not
	 * used in a functional way.
	 *
	 * This value is in Hz.
	 */
	int vrefresh;

	/**
	 * @hsync:
	 *
	 * Horizontal refresh rate, for debug output in human readable form. Not
	 * used in a functional way.
	 *
	 * This value is in kHz.
	 */
	int hsync;

	enum hdmi_picture_aspect picture_aspect_ratio;

	/**
	 * @export_head:
	 *
	 * struct list_head for modes to be exposed to the userspace.
	 * This is to maintain a list of exposed modes while preparing
	 * user-mode's list in drm_mode_getconnector ioctl. The purpose of this
	 * list_head only lies in the ioctl function, and is not expected to be
	 * used outside the function.
	 * Once used, the stale pointers are not reset, but left as it is, to
	 * avoid overhead of protecting it by mode_config.mutex.
	 */
	struct list_head export_head;
};

#define for_each_displaymode(mode, mode_head) \
	list_for_each_entry(mode, mode_head, head)
#define for_each_displaymode_safe(mode, mode2, mode_head) \
	list_for_each_entry_safe(mode, mode2, mode_head, head)

extern bool inno_mode_equal_no_flags(const struct inno_mode *mode1,
		const struct inno_mode *mode2);
extern void inno_mode_set_name(struct inno_mode *mode);
extern int inno_mode_vrefresh(const struct inno_mode *mode);
extern int inno_mode_hsync(const struct inno_mode *mode);
extern struct inno_mode *inno_mode_create(void);
extern void inno_mode_copy(struct inno_mode *dst, const struct inno_mode *src);
extern struct inno_mode *inno_mode_duplicate(
					    const struct inno_mode *mode);
extern struct inno_mode *inno_gtf_mode_complex(int hdisplay, int vdisplay,
		     int vrefresh, bool interlaced, int margins,
		     int GTF_M, int GTF_2C, int GTF_K, int GTF_2J);
extern struct inno_mode *inno_gtf_mode(int hdisplay, int vdisplay, int vrefresh,
	     bool interlaced, int margins);
extern struct inno_mode *inno_cvt_mode(int hdisplay,
				      int vdisplay, int vrefresh,
				      bool reduced, bool interlaced, bool margins);
enum inno_mode_status
inno_mode_validate_basic(const struct inno_mode *mode);

enum inno_mode_status
inno_mode_validate_size(const struct inno_mode *mode,
		       int maxX, int maxY);
enum inno_mode_status
inno_mode_validate_flag(const struct inno_mode *mode,
		       int flags);
void inno_mode_prune_invalid(struct list_head *mode_list);

struct inno_mode *inno_mode_find_out_mode(int perfect_w, int perfect_h, struct list_head *mode_list);
int inno_modes_replace_timing(struct inno_mode *mode);

void inno_mode_sort(struct list_head *mode_list);

#endif /* __INNO_MODE_H__ */
