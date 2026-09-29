/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2026, Spacemit
 */

#ifndef __MP2973_H__
#define __MP2973_H__

enum MP2973_reg {
	MP2973_ID_DCDC1,
};

#define MP2973_BUCK_VSEL_REG		0x21
#define MP2973_BUCK_VSEL_MASK		0x1ff

/* No practical significance */
#define MP2973_BUCK_EN_MASK		0x0
/* No practical significance */
#define MP2973_BUCK_CTRL_REG		0x0

/* VOUT_COMMAND(0x21) encoding comes from MFR_RESO_SET(0xC7), which is paged:
 * page0 [7:6] = rail1, page1 [4:3] = rail2 (DIRECT / LINEAR / 9-bit VID). */
#define MP2973_PAGE_REG			0x00
#define MP2973_PAGE_RAIL1		0
#define MP2973_PAGE_RAIL2		1
#define MP2973_CONFIG_PAGE		2
#define MP2973_MFR_RESO_SET		0xc7
#define MP2973_VOUT_MODE_REG		0x20
#define MP2973_VOUT_FORMAT_DIRECT_R1	(1 << 7)
#define MP2973_VOUT_FORMAT_LINEAR_R1	(1 << 6)
#define MP2973_VOUT_FORMAT_DIRECT_R2	(1 << 4)
#define MP2973_VOUT_FORMAT_LINEAR_R2	(1 << 3)
#define MP2973_MFR_VR_MULTI_CONFIG_R1	0x0d
#define MP2973_MFR_VR_MULTI_CONFIG_R2	0x1d
#define MP2973_IMVP9_EN_R2		(1 << 13)	/* spec: Reserved, fixed 0 */
#define MP2973_VID_STEP_SEL_R1		(1 << 4)
#define MP2973_VID_STEP_SEL_R2		(1 << 3)
#define MP2973_VOUT_CODE_MIN		1
#define MP2973_VOUT_CODE_MAX		0x1ff
#define MP2973_LINEAR16_CODE_MAX	0xfff

/* Rail programmed by default (page0 = rail1); a board with a different wiring
 * overrides this single value. */
#ifndef MP2973_RAIL
#define MP2973_RAIL			MP2973_PAGE_RAIL1
#endif

#define MP2973_BUCK_LINER_RANGE					\
static const struct pm8xx_linear_range mp2973_buck_ranges[] = {	\
	/* sel 1..0x1ff: 500000 = VID min = (vref 49 + code 1) * 10mV */		\
	REGULATOR_LINEAR_RANGE(500000, 1, 511, 10000),		\
};

#define MP2973_REGULATOR_DESC						\
static const struct pm8xx_buck_desc mp2973_buck_desc[] = {		\
	/* BUCK */							\
	PM8XX_DESC_COMMON(MP2973_ID_DCDC1, "EDCDC_REG1",		\
			511, MP2973_BUCK_VSEL_REG, MP2973_BUCK_VSEL_MASK, \
			MP2973_BUCK_CTRL_REG, MP2973_BUCK_EN_MASK,	\
			0, 0,						\
			mp2973_buck_ranges),				\
};

#define MP2973_REGULATOR_MATCH_DATA					\
struct regulator_match_data mp2973_regulator_match_data = {		\
	.nr_buck_desc = ARRAY_SIZE(mp2973_buck_desc),			\
	.buck_desc = mp2973_buck_desc,					\
	.nr_ldo_desc = 0,						\
	.ldo_desc = NULL,						\
	.nr_switch_desc = 0,						\
	.switch_desc = NULL,						\
	.name = "mp2973",						\
	.max_registers = 0xc8,	/* highest accessed reg is 0xC7 (MFR_RESO_SET) */	\
};

#define DECLEAR_MP2973_REGULATOR_MATCH_DATA	extern struct regulator_match_data mp2973_regulator_match_data;

/* shared per-rail VOUT helper (SPL + regulator uclass); ESOS keeps a second copy */
#define MP2973_ENC_DIRECT	0
#define MP2973_ENC_LINEAR	1
#define MP2973_ENC_VID		2

struct mp2973_io {
	int (*read)(void *ctx, u8 reg, u8 *buf, int len);
	int (*write)(void *ctx, u8 reg, u8 *buf, int len);
	void *ctx;
	u8 page;		/* target rail, see MP2973_RAIL */
};

static inline u16 mp2973_code_max(int mode)
{
	if (mode == MP2973_ENC_DIRECT)
		return 0xffff;
	if (mode == MP2973_ENC_LINEAR)
		return MP2973_LINEAR16_CODE_MAX;
	return MP2973_VOUT_CODE_MAX;
}

static inline int mp2973_page(struct mp2973_io *io, u8 page)
{
	u8 pg = page;

	return io->write(io->ctx, MP2973_PAGE_REG, &pg, 1) ? -1 : 0;
}

/* Encoding of io->page's rail (C7h on that page; vref 49, IMVP9 bits Reserved);
 * leaves the page there */
static inline int mp2973_fmt(struct mp2973_io *io, int *mode, int *vref_mv,
			     int *step_mv, int *exp)
{
	u8 val[2] = { 0, 0 };
	u8 vmode;
	u16 reso, cfg;
	int rail1 = (io->page == MP2973_PAGE_RAIL1);
	u8 fmt_direct = rail1 ? MP2973_VOUT_FORMAT_DIRECT_R1 :
				MP2973_VOUT_FORMAT_DIRECT_R2;
	u8 fmt_linear = rail1 ? MP2973_VOUT_FORMAT_LINEAR_R1 :
				MP2973_VOUT_FORMAT_LINEAR_R2;
	u8 step_bit = rail1 ? MP2973_VID_STEP_SEL_R1 : MP2973_VID_STEP_SEL_R2;
	u8 cfg_reg = rail1 ? MP2973_MFR_VR_MULTI_CONFIG_R1 :
			     MP2973_MFR_VR_MULTI_CONFIG_R2;

	*mode = MP2973_ENC_VID;
	*vref_mv = 49;
	*step_mv = 10;
	*exp = -9;

	if (mp2973_page(io, io->page) ||
	    io->read(io->ctx, MP2973_MFR_RESO_SET, val, 2))
		return -1;

	reso = val[0] | ((u16)val[1] << 8);

	if (reso & fmt_direct) {
		*mode = MP2973_ENC_DIRECT;
	} else if (reso & fmt_linear) {
		*mode = MP2973_ENC_LINEAR;
		if (io->read(io->ctx, MP2973_VOUT_MODE_REG, &vmode, 1))
			return -1;
		*exp = vmode & 0x1f;
		if (*exp & 0x10)
			*exp -= 0x20;
	} else {
		if (mp2973_page(io, MP2973_CONFIG_PAGE) ||
		    io->read(io->ctx, cfg_reg, val, 2))
			return -1;
		cfg = val[0] | ((u16)val[1] << 8);
		if (cfg & step_bit)
			*step_mv = 5;
	}

	return mp2973_page(io, io->page);
}

static inline int mp2973_encode(int mode, int vref_mv, int step_mv, int exp,
				u32 uvolt, u16 *code)
{
	int c;

	if (mode == MP2973_ENC_DIRECT) {	/* 1mV/LSB */
		c = (int)(uvolt / 1000);
		if (c <= 0)
			return -1;
	} else if (mode == MP2973_ENC_LINEAR) {	/* LSB = 2^exp V */
		if (exp >= 0 || exp < -19)
			return -1;
		c = (int)(((u64)uvolt * (1u << -exp) + 500000) / 1000000);
	} else {				/* VID */
		c = (int)(uvolt / 1000 + step_mv / 2) / step_mv - vref_mv;
		if (c < MP2973_VOUT_CODE_MIN)
			return -1;
	}

	if (c > (int)mp2973_code_max(mode))
		return -1;

	*code = (u16)c;
	return 0;
}

static inline int mp2973_decode(int mode, int vref_mv, int step_mv, int exp, u16 code)
{
	if (mode == MP2973_ENC_DIRECT)
		return (int)code * 1000;

	if (mode == MP2973_ENC_LINEAR) {
		if (exp >= 0 || exp < -19)
			return -1;
		return (int)(((u64)(code & MP2973_LINEAR16_CODE_MAX) * 1000000) >> (-exp));
	}

	return (vref_mv + (int)(code & MP2973_VOUT_CODE_MAX)) * step_mv * 1000;
}

/* Set io->page's VOUT_COMMAND (encoding aware, range and other rail checked) */
static inline int mp2973_set_vout(struct mp2973_io *io, u32 uvolt, u32 min_uv,
				  u32 max_uv)
{
	int mode, vref_mv, step_mv, exp;
	u8 val[2];
	u16 code, back, other_before, other_after;
	u8 other = (io->page == MP2973_PAGE_RAIL1) ? MP2973_PAGE_RAIL2 :
						      MP2973_PAGE_RAIL1;

	if ((min_uv && uvolt < min_uv) || (max_uv && uvolt > max_uv))
		return -1;

	if (mp2973_fmt(io, &mode, &vref_mv, &step_mv, &exp) ||
	    mp2973_encode(mode, vref_mv, step_mv, exp, uvolt, &code))
		return -1;

	/* encode() rounds to the step grid: re-check the window on the code */
	if (min_uv || max_uv) {
		u32 got = (u32)mp2973_decode(mode, vref_mv, step_mv, exp, code);

		if (min_uv && got < min_uv)
			code++;
		else if (max_uv && got > max_uv)
			code--;
		if (code > mp2973_code_max(mode))
			return -1;
		got = (u32)mp2973_decode(mode, vref_mv, step_mv, exp, code);
		if ((min_uv && got < min_uv) || (max_uv && got > max_uv))
			return -1;
	}

	/* a write on the wrong page would stay silent, so snapshot the other rail */
	if (mp2973_page(io, other) ||
	    io->read(io->ctx, MP2973_BUCK_VSEL_REG, val, 2))
		return -1;
	other_before = val[0] | ((u16)val[1] << 8);

	val[0] = code & 0xff;
	val[1] = code >> 8;
	if (mp2973_page(io, io->page) ||
	    io->write(io->ctx, MP2973_BUCK_VSEL_REG, val, 2))
		return -1;

	val[0] = 0;
	val[1] = 0;
	if (io->read(io->ctx, MP2973_BUCK_VSEL_REG, val, 2))
		return -1;

	back = val[0] | ((u16)val[1] << 8);
	if ((back & mp2973_code_max(mode)) != code)
		return -1;

	if (mp2973_page(io, other) ||
	    io->read(io->ctx, MP2973_BUCK_VSEL_REG, val, 2))
		return -1;
	other_after = val[0] | ((u16)val[1] << 8);
	if (other_after != other_before)
		return -1;

	return mp2973_page(io, io->page);
}

static inline int mp2973_get_vout(struct mp2973_io *io, int *uvolt)
{
	int mode, vref_mv, step_mv, exp;
	u8 val[2] = { 0, 0 };
	u16 code;

	if (mp2973_fmt(io, &mode, &vref_mv, &step_mv, &exp) ||
	    io->read(io->ctx, MP2973_BUCK_VSEL_REG, val, 2))
		return -1;

	code = val[0] | ((u16)val[1] << 8);
	*uvolt = mp2973_decode(mode, vref_mv, step_mv, exp, code);

	return (*uvolt < 0) ? -1 : 0;
}

#endif
