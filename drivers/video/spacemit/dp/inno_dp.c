// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Spacemit Co., Ltd.
 *
 */

#include "inno_modes.h"
#include "inno_edid.h"
#include "inno_conn.h"
#include "inno_dp_common.h"
#include "inno_dp.h"
#include "inno_utils.h"
#include "inno_parse_edid.h"
#include <asm/io.h>
#include <linux/io.h>

static uint32_t inno_dp_aux_write(uint32_t cmd, uint32_t addr, uint32_t * wr_buf,
				  uint32_t length, struct inno_conn_t *conn)
{
	//Read Request Transaction
	if (cmd & 0x1) {
	osal_write32(0x404,//(0x400 INNODP_PHY_AUX_ENC_CMD)
					(length << 24) | (addr << 4) | cmd, conn);
	//Write Request Transaction
	} else {
	osal_write32(0x404,//(0x400 INNODP_PHY_AUX_ENC_CMD)
					(length << 24) | (addr << 4) | cmd, conn);
		osal_write32(0x418, wr_buf[0], conn); //(0x40c INNODP_AUX_DATA1)
		osal_write32(0x414, wr_buf[1], conn); //(0x410 INNODP_AUX_DATA2)
		osal_write32(0x410, wr_buf[2], conn); //(0x414 INNODP_AUX_DATA3)
		osal_write32(0x40c, wr_buf[3], conn); //(0x408 INNODP_AUX_DATA0)
	}

	osal_write32(0x408, BIT(31) | osal_read32(0x408, conn), conn);//(0x408 INNODP_PHY_AUX_START)

	return 0;
}

static uint32_t inno_dp_aux_read(uint32_t read, uint32_t addr, uint32_t *rd_buf,
				 struct inno_conn_t *conn)
{
	uint32_t ret = 0;
	uint32_t retry = 0;

	//wait aux reply  event
	while (retry++ <= INNODP_WAIT_AUX_REPLY_CNT) {
		if (((osal_read32(0x400, conn) >> 22) & 0x3) == 0) {
			break;
		}
		osal_msleep(1);
	}

	if (read == 1) {
		osal_msleep(2);
		rd_buf[0] = osal_read32(0x418, conn);
		rd_buf[1] = osal_read32(0x414, conn);
		rd_buf[2] = osal_read32(0x410, conn);
		rd_buf[3] = osal_read32(0x40c, conn);
	}

	ret = (osal_read32(0x400, conn) >> 28) & 0xf;

	return ret;

}

static uint32_t inno_dp_aux_channel_run(struct aux_cfg *aux_get_rx, struct dp_chip_t *inno)
{
	uint32_t ret = 0;
	struct inno_conn_t *conn = (struct inno_conn_t *)inno->priv;

	ret = inno_dp_aux_write(aux_get_rx->aux_cmd,
			aux_get_rx->dpcd_addr, aux_get_rx->wr_buff,
			aux_get_rx->length, conn);
	if (ret == 1)
		return 1;

	ret = inno_dp_aux_read(aux_get_rx->read,
			aux_get_rx->dpcd_addr, aux_get_rx->rd_buff, conn);

	return ret;
}
static uint32_t inno_dp_get_transmit_enable(uint32_t lane_count)
{
	if (lane_count == 1)
		return 0x1;
	else if (lane_count == 2)
		return 0x3;
	else if (lane_count == 3)
		return 0x7;
	else if (lane_count == 4)
		return 0xf;
	else
		return 0x1;
}

static void inno_dp_train_pattern_set(struct dp_chip_t *inno, uint32_t pattern)
{
	uint32_t temp = 0;
	struct inno_conn_t *conn = (struct inno_conn_t *)inno->priv;

	/* (0x100 INNODP_LANE_CONFIG) */
	temp = osal_read32(0x100, conn);
	temp &= ~(0xf << 25);
	temp |= pattern << 25; //pattern mode
	temp |= inno_dp_get_transmit_enable(inno->lane_count) << 17;  //transmit lanes
	temp |= BIT(8);
	osal_write32(0x100, temp, conn);
}

static void inno_dp_set_enhance(struct dp_chip_t *inno)
{
	struct inno_conn_t *conn = (struct inno_conn_t *)inno->priv;

	if (inno->enhance_mode) {
		osal_write32(0x18, BIT(30) | osal_read32(0x18, conn), conn);
	} else {
		osal_write32(0x18, ~(BIT(30)) & osal_read32(0x18, conn), conn);
	}
}


static void inno_dp_phy_cfg(struct dp_chip_t *inno)
{
	struct inno_conn_t *conn = (struct inno_conn_t *)inno->priv;

	osal_write32(0x100, (inno->phy_lanes << 5) |
		     (inno->phy_rate) | osal_read32(0x100, conn), conn);
	//phy power on
	osal_write32(0x100, ~(0xf << 13) & osal_read32(0x100, conn), conn);
}

static void inno_dp_core_pll_cfg(struct dp_chip_t *inno)
{
#define pll_prediv     (0)
#define pll_fbdiv      (1)
#define pll_postdiv    (2)
#define pll_clkdiv_16m (3)
#define pll_postdiv_en (4)
#define pll_vcoclk_div8_en (5)

	//for reference clk 50mhz config
	uint32_t pll_table[][6] = {
		{2, 64, 0, 12, 1, 1},
		{1, 54, 0, 21, 1, 1},
		{1, 54, 0, 42, 0, 0},
	};
	struct inno_conn_t *conn = (struct inno_conn_t *)inno->priv;

	if (inno->phy_rate >= ARRAY_SIZE(pll_table) || inno->phy_rate < 0)
		inno->phy_rate = 1;

	//power down core pll
	osal_write32(0x180, BIT(0) | osal_read32(0x180, conn), conn);

	osal_write32(0x188,//(0x188), INNODP_SPREAD_CFG
				(osal_read32(0x188, conn) & ~(0x3 << 8)) |
				(pll_table[inno->phy_rate][pll_postdiv] << 8) |
				(pll_table[inno->phy_rate][pll_postdiv_en] << 11) |
				(pll_table[inno->phy_rate][pll_vcoclk_div8_en] << 16), conn);

	osal_write32(0x1a0, (osal_read32(0x1a0, conn) & ~(0x3f << 8)) |
		     (pll_table[inno->phy_rate][pll_clkdiv_16m] << 8), conn);//(0x1a0), INNODP_CLKDIV_16M

	//(0x180)
	osal_write32(0x180, (osal_read32(0x180, conn) & ~(0x3f << 8) & ~(0xf << 16) & ~(0xff << 24)) |
		     ((pll_table[inno->phy_rate][pll_fbdiv] >> 8) << 16) |
		     ((pll_table[inno->phy_rate][pll_fbdiv] & 0xff) << 24) |
		     (pll_table[inno->phy_rate][pll_prediv] << 8), conn);
	//turn off frac ctr
	osal_write32(0x180, (0x3 << 4) & osal_read32(0x180, conn), conn);

	//power up core pll
	osal_write32(0x180, ~(BIT(0)) & osal_read32(0x180, conn), conn);

	osal_msleep(1);

#undef pll_fbdiv
#undef pll_prediv
#undef pll_postdiv
#undef pll_clkdiv_16m
}

static void inno_dp_phy_reset(struct inno_conn_t *conn)
{
	/* reset phy and controller, video, audio */
	osal_write32(0x1c,  BIT(31) | BIT(30) | BIT(28) | BIT(0) | osal_read32(0x1c, conn), conn);
	osal_msleep(5);

	osal_write32(0x1c, (~(BIT(31) | BIT(30) | BIT(28) | BIT(0))) & osal_read32(0x1c, conn), conn);
	osal_msleep(5);
}

static void inno_dp_phy_init(struct inno_conn_t *conn)
{
	/* core pll cfg */
	osal_write32(0x180, 0xe1300231, conn);
	osal_write32(0x184, 0x22000000, conn);
	osal_write32(0x188, 0x1, conn);
	osal_write32(0x1a0, 0x2a00, conn);
	osal_write32(0x198, 0x2012a, conn);
	osal_write32(0x180, 0xe1300230, conn);
	osal_msleep(10);

	/* check core pll lock */
	osal_read32(0x180, conn);

	/* enable hpd plug */
	osal_write32(0x8c, 0x20000000, conn);
}

static void inno_dp_tu_init(struct dp_chip_t *inno, struct inno_mode *mode)
{
	struct inno_conn_t *conn = (struct inno_conn_t *)inno->priv;
	uint16_t htotal = mode->htotal;
	uint16_t hactive = mode->hdisplay;
	uint32_t clock = mode->clock / 1000;
	uint16_t hblank = htotal - hactive;
	uint32_t hb_num = 0;
	uint32_t tu;
	uint32_t tu_frac;
	uint32_t tu_int;
	uint32_t rd_thres;
	uint32_t link_rate = 0;
	uint32_t lane = inno->lane_count;

	if (!htotal)
		return;

	if (inno->phy_rate == 0x00) {
		hb_num = hblank * (162 / 4) / clock;
		link_rate = 162;
	} else if (inno->phy_rate == 0x1) {
		hb_num = hblank * (270 / 4) / clock;
		link_rate = 270;
	} else if (inno->phy_rate == 0x2) {
		hb_num = hblank * (540 / 4) / clock;
		link_rate = 540;
	}

	uint32_t bpp = 24; //for rgb888
	tu = clock * bpp * 640 / (8 * lane * link_rate);
	tu_frac = tu % 10;
	tu_int = tu / 10;

	if (tu_int < 6) {
		rd_thres = 32;
	} else if (hblank < 80) {
		rd_thres = 12;
	} else {
		rd_thres = 16;
	}

	osal_write32(0x230,  (osal_read32(0x230, conn) & 0xffff) | (hb_num << 16), conn);
	osal_write32(0x218, (tu_frac << 21) | (rd_thres << 10) | (tu_int << 25), conn);
}

static void inno_dp_video_cfg(struct inno_conn_t *conn, struct inno_mode *mode)
{
	uint32_t value = 0;
	uint16_t htotal = mode->htotal;
	uint16_t hactive = mode->hdisplay;
	uint16_t hsync_end = mode->hsync_end;
	uint16_t hsync_start = mode->hsync_start;
	uint16_t vactive = mode->vdisplay;
	uint16_t vsync_end = mode->vsync_end;
	uint16_t vsync_start = mode->vsync_start;
	uint16_t vtotal = mode->vtotal;
	uint32_t flags = mode->flags;
	uint8_t msa_misc0 = 0x20;
	uint8_t polarity = 0x0;

	uint16_t hblank = htotal - hactive;
	uint16_t hsync = hsync_end - hsync_start;
	uint16_t hoffset = htotal - hsync_start;
	uint16_t vblank = vtotal - vactive;
	uint16_t vsync = vsync_end - vsync_start;
	uint16_t voffset = vtotal - vsync_start;
	uint16_t video_map = 0;

	value |= flags & DRM_MODE_FLAG_PHSYNC ? BIT(1) : 0;
	value |= flags & DRM_MODE_FLAG_PVSYNC ? BIT(0) : 0;
	polarity = value;

	// The parsing result of edid does not match
	if (hactive == 1920 && vactive == 1200) {
		polarity = 0x3;
	}

	msa_misc0 = 0x20; //rgb888
	video_map = 0x1; //rgb888

	osal_write32(0x200, (~(0x1f << 22) & osal_read32(0x200, conn)) |
		     (video_map << 22), conn);//(0x200 INNODP_VDEIO_MAP_WIDTH)
	osal_write32(0x224, (hoffset << 16) | voffset, conn);//(0x224 INNODP_VH_START)
	osal_write32(0x20c, polarity << 28, conn); //(0x20c INNODP_TX_POLARITY)
	osal_write32(0x228, msa_misc0 | osal_read32(0x228, conn), conn);//(0x228 INNODP_MAS_MISC0)
	osal_write32(0x214, (hblank << 16) |
		     (hactive), conn);	//(0x210 INNODP_TX_HACTIVE_HBALNK)
	osal_write32(0x22c, 0, conn);//(0x22c INNODP_MAS_MISC1)
	osal_write32(0x210, (vactive << 16) | (vblank), conn);	//(0x214 INNODP_TX_VACTIVE_VBALNK)
	osal_write32(0x21c, (hsync) |
		     ((htotal - hactive - hoffset) << 16), conn);//(0x218 INNODP_TX_HS_HFP)
	osal_write32(0x220, (vsync) |
		     ((vtotal - vactive - voffset) << 16), conn);//(0x21c INNODP_TX_VS_VFP)

	osal_write32(0x220, (vsync) |
		     ((vtotal - vactive - voffset) << 16), conn);//(0x21c INNODP_TX_VS_VFP)

	//osal_write32(0x23c, ~(BIT(1)) & osal_read32(0x23c, conn), conn);
	osal_write32(0x23c, (BIT(1)) | osal_read32(0x23c, conn), conn);

	inno_dp_tu_init(conn->priv, mode);
}

static void inno_dp_link_config(struct dp_chip_t *inno)
{
	inno_dp_core_pll_cfg(inno);
	inno_dp_phy_cfg(inno);
	inno_dp_set_enhance(inno);
}

static void inno_dp_swing_adjust(struct dp_chip_t *inno, int param)
{
	struct inno_conn_t *conn = (struct inno_conn_t *)inno->priv;
#define inno_DP_MAX_SW_COMBI (0x9)
	const uint8_t rates[] = {0x06, 0x0a, 0x14};
	uint8_t combi_idx = 8;
	uint32_t temp1 = 0;
	uint8_t i = 0;
	struct swing_adjust_item {
		uint8_t vol_swing;
		uint8_t pre_emphasis;
	} swing[][inno_DP_MAX_SW_COMBI] = {
		{
			/* 1.62 Gbps */
			{0x0a, 0x00}, /* swing:0, emphasis: 0 */
			{0x0e, 0x02}, /* swing:0, emphasis: 1 */
			{0x11, 0x04}, /* swing:0, emphasis: 2 */
			{0x0c, 0x00}, /* swing:1, emphasis: 0 */
			{0x0f, 0x02}, /* swing:1, emphasis: 1 */
			{0x13, 0x05}, /* swing:1, emphasis: 2 */
			{0x0f, 0x00}, /* swing:2, emphasis: 0 */
			{0x12, 0x03}, /* swing:2, emphasis: 1 */
			{0x18, 0x06}, /* swing:2, emphasis: 2 */
		},
		{
			/* 2.7 Gbps */
			{0x07, 0x00}, /* swing:0, emphasis: 0 */
			{0x0a, 0x02}, /* swing:0, emphasis: 1 */
			{0x0f, 0x05}, /* swing:0, emphasis: 2 */
			{0x0d, 0x00}, /* swing:1, emphasis: 0 */
			{0x10, 0x02}, /* swing:1, emphasis: 1 */
			{0x14, 0x05}, /* swing:1, emphasis: 2 */
			{0x12, 0x00}, /* swing:2, emphasis: 0 */
			{0x16, 0x03}, /* swing:2, emphasis: 1 */
			{0x19, 0x06}, /* swing:2, emphasis: 2 */
		},
		{
			/* 5.4 Gbps */
			{0x06, 0x00}, /* swing:0, emphasis: 0 */
			{0x09, 0x01}, /* swing:0, emphasis: 1 */
			{0x0d, 0x03}, /* swing:0, emphasis: 2 */
			{0x0b, 0x00}, /* swing:1, emphasis: 0 */
			{0x12, 0x03}, /* swing:1, emphasis: 1 */
			{0x17, 0x05}, /* swing:1, emphasis: 2 */
			{0x10, 0x00}, /* swing:2, emphasis: 0 */
			{0x1d, 0x04}, /* swing:2, emphasis: 1 */
			{0x1a, 0x06}, /* swing:2, emphasis: 2 */
		},
	};

	combi_idx = inno->lane_swing[0] * 3 + (inno->lane_emphasis[0] >> DP_TRAIN_PRE_EMPHASIS_SHIFT);

	for (i = 0; i < ARRAY_SIZE(rates); i++)
		if (inno->lane_rate == rates[i])
			break;

	if (i >= ARRAY_SIZE(rates) || combi_idx >= inno_DP_MAX_SW_COMBI) {
		osal_printf_func( "invalid rate index:%d %d\n", i, combi_idx);
		return;
	}

	osal_printf_func("configuration voltage swing:%d pre-emphasis:%d rate:%u KHZ\n",
		swing[i][combi_idx].vol_swing,
		swing[i][combi_idx].pre_emphasis,
		inno->lane_rate * 27000);

	temp1 = osal_read32(0x1a8, conn);/*(0x1a8)*/
	temp1 &= ~((0x1f << 16) | (0x1f << 24));
	temp1 |= ((swing[i][combi_idx].vol_swing << 16) | (swing[i][combi_idx].vol_swing << 24));
	osal_write32(0x1a8, temp1, conn);

	temp1 = osal_read32(0x1ac, conn);/*(0x1ac)*/
	temp1 &= ~((0x1f) | (0x1f << 8) | (0xffff << 16));
	temp1 |= (swing[i][combi_idx].vol_swing | (swing[i][combi_idx].vol_swing << 8) |
		 (swing[i][combi_idx].pre_emphasis << 16) | (swing[i][combi_idx].pre_emphasis << 20) |
		 (swing[i][combi_idx].pre_emphasis << 24) | (swing[i][combi_idx].pre_emphasis << 28));
	osal_write32(0x1ac, temp1, conn);
}

static void inno_dp_phy_pattern_update(struct dp_chip_t *inno, uint8_t *pattern)
{
	struct inno_conn_t *conn = (struct inno_conn_t *)inno->priv;
	const uint32_t pattern_item[][4] = {
		/* pattern convert */
		[0] = {0, 0, 0, 0},
		[1] = {1, 0, 0, 0},
		[2] = {5, 0, 0, 0},
		[3] = {6, 0, 0, 0},
		[4] = {7, 0x3e0f83e0, 0x3e0f83e0, 0x000f83e0},
		[5] = {8, 0, 0, 0},
		[6] = {0, 0, 0, 0},
		[7] = {4, 0, 0, 0},
	};

	if (*pattern >= ARRAY_SIZE(pattern_item)) {
		osal_printf_func("unsupport test pattern:%d\n", *pattern);
		*pattern = 1;
	}

	osal_printf_func( "phy pattern test:%d convert:%d\n", *pattern, pattern_item[*pattern][0]);

	osal_write32(0x108, pattern_item[*pattern][1], conn);
	osal_write32(0x10c, pattern_item[*pattern][2], conn);
	osal_write32(0x110, pattern_item[*pattern][3], conn);

	*pattern = pattern_item[*pattern][0];
}

static void inno_dp_irq_enable(struct dp_chip_t *inno)
{
	uint32_t tmp = 0;
	struct inno_conn_t *conn = (struct inno_conn_t *)inno->priv;

	tmp = osal_read32(0x84, conn);
	//tmp |= BIT(16); /* enable aux reply event, if interrupts are used to handle aux events, instead of polling the */
	tmp |= BIT(17); /* enable hpd plug evnet */
	osal_write32(0x84, tmp, conn);

	tmp = osal_read32(0x8c, conn);
	/* enable hpd event; hpd in irq; hpd out irq */
	tmp = BIT(31) | BIT(29) | BIT(28);
	osal_write32(0x8c, tmp, conn);

	tmp = osal_read32(0x84, conn);
	tmp = osal_read32(0x8c, conn);
}

static int inno_dp_irq_handle(struct dp_chip_t *inno)
{
	uint32_t value = 0;
	uint32_t hpd_plug = 0;
	int irq_status = 0;
	struct inno_conn_t *conn = (struct inno_conn_t *)inno->priv;

	value = osal_read32(0x80, conn);
	hpd_plug = osal_read32(0x88, conn);

#define CHIP_DP_HPD_EVENT       BIT(17)
#define CHIP_DP_HPD_IRQ         BIT(31)
#define CHIP_DP_HPD_PLUG_IN     BIT(29)
#define CHIP_DP_HPD_PLUG_OUT    BIT(28)
#define CHIP_DP_AUX_REPLY_EVENT BIT(16)

	if (value & CHIP_DP_HPD_EVENT) {

		osal_printf_func( "dp irq status %#.8x\n", value);
		osal_printf_func( "dp hpd status %#.8x\n", hpd_plug);

		if (hpd_plug & CHIP_DP_HPD_IRQ) {
			osal_printf_func( "hpd irq event\n");
			irq_status |= INNODP_HPD_IRQ;
		}

		if (hpd_plug & CHIP_DP_HPD_PLUG_IN) {
			osal_printf_func("hpd in irq\n");
			irq_status |= INNODP_HPD_IN;
		}

		if (hpd_plug & CHIP_DP_HPD_PLUG_OUT) {
			osal_printf_func("hpd out irq\n");
			irq_status |= INNODP_HPD_OUT;
		}

		osal_write32(0x88, irq_status, conn);
	}

	if (value & CHIP_DP_AUX_REPLY_EVENT) {
		osal_write32(0x80, BIT(16) | //clear aux reply event
			     osal_read32(0x80, conn), conn); //(0x80 INNODP_STA_AUX_HPD)
	}

	return irq_status;
}

static bool inno_dp_detect(struct inno_conn_t *conn)
{
	uint32_t reg_value = 0, reg_value1;

	//need 500ms for long
	osal_msleep(500);

	reg_value = osal_read32(0x80, conn);
	reg_value1 = osal_read32(0x88, conn);
	if ((reg_value & BIT(17)) && (reg_value1 & BIT(29))) {
		//clear interrupt
		inno_dp_irq_handle(conn->priv);
	}

	if (reg_value1 & BIT(26)) {
		osal_printf("%s() true\n", __func__);
		return true;
	} else {
		osal_printf("%s() false\n", __func__);
		return false;
	}
}

static int inno_dp_init(struct inno_conn_t *conn)
{
	struct dp_chip_t *inno = NULL;
	void __iomem *base;

	inno = osal_malloc(sizeof(struct dp_chip_t));
	osal_memset(inno, 0, sizeof(struct dp_chip_t));

	conn->priv = inno;
	inno->priv = conn;

	base = (void __iomem *)ioremap(conn->regbase, conn->regsize);
	conn->reg_mmap_addr = base;

	inno->dp_training_pattern_set = inno_dp_train_pattern_set;
	inno->dp_aux_channel_run = inno_dp_aux_channel_run;
	inno->dp_link_config = inno_dp_link_config;

	inno->dp_phy_pattern_update = inno_dp_phy_pattern_update;
	inno->dp_source_swing_adjust = inno_dp_swing_adjust;

	inno->irq_handle = inno_dp_irq_handle;

	inno_dp_phy_reset(conn);
	inno_dp_phy_init(conn);
	inno_dp_irq_enable(inno);

	return 0;
}

static void inno_dp_exit(struct inno_conn_t *conn)
{
	//power down phy
	osal_write32(0x100, (0xc << 13) | osal_read32(0x100, conn), conn);
	//power down pll
	osal_write32(0x180, BIT(0) | osal_read32(0x180, conn), conn);
	osal_write32(0x190, BIT(0) | osal_read32(0x190, conn), conn);
	osal_write32(0x1a4, ~(0xf) & osal_read32(0x1a4, conn), conn);
	osal_write32(0x1a4, ~(0xf << 8) & osal_read32(0x1a4, conn), conn);
	osal_write32(0x1b0, ~(0xf << 28) & osal_read32(0x1b0, conn), conn);
	osal_write32(0x1c0, ~(BIT(27)) & osal_read32(0x1c0, conn), conn);

	iounmap(conn->reg_mmap_addr);

	if (conn->priv) {
		osal_free(conn->priv);
		conn->priv = NULL;
	}
}

static int inno_dp_get_edid(struct inno_conn_t *conn, uint8_t *buff)
{
	int ret = -1;
	struct dp_chip_t *inno = (struct dp_chip_t *)conn->priv;

	if (inno_dp_detect(conn)) {
		ret = inno_dp_read_edid(inno, buff);
	}

	return ret;
}

static int inno_dp_show_edid(struct inno_conn_t *conn, uint8_t *buff)
{
	struct edid *edid = (struct edid *)buff;

	if (!inno_edid_is_valid(edid)) {
		osal_printf_func("[%d]Edid Invalid\n", conn->conn_id);
		return -1;
	}

	ParseEdidBlock0(buff);
	if (edid->extensions)
		ParseEdidBlock1(buff + EDID_LENGTH);

	return 0;
}

static int inno_dp_modeset(struct inno_conn_t *conn, struct inno_mode *mode)
{
	struct dp_chip_t *inno = (struct dp_chip_t *)conn->priv;

	osal_printf("%s() hdisplay %d vdisplay %d\n", __func__, mode->hdisplay, mode->vdisplay);

	osal_write32(0x18, osal_read32(0x18, conn) & ~BIT(29), conn);
	osal_write32(0x18, osal_read32(0x18, conn) | BIT(30), conn);
	osal_write32(0x100, osal_read32(0x100, conn) & ~0x1E0000, conn);

	/* core pll config */
	osal_printf("%s() core pll config\n", __func__);

	osal_write32(0x180, 0xe13002b1, conn);
	osal_write32(0x188, 0x10801, conn);
	osal_write32(0x184, osal_read32(0x184, conn) & 0xFF000000, conn);
	osal_write32(0x198, osal_read32(0x198, conn) | BIT(17), conn);
	osal_write32(0x198, osal_read32(0x198, conn) | BIT(8), conn);
	osal_write32(0x1a0, 0x1500, conn);
	osal_write32(0x180, 0xe1300230, conn);
	osal_msleep(100);

	osal_read32(0x180, conn);

	/* compliance config */
	osal_printf("%s() compliance config\n", __func__);

	inno_dp_compliance_config(inno);

	/* pixel pll config*/
	osal_printf("%s() pixel pll config\n", __func__);

	osal_write32(0x190, 0x63300181, conn);
	osal_write32(0x194, 0x01000801, conn);
	osal_write32(0x198, 0x0102012a, conn);
	osal_write32(0x190, 0x63300180, conn);
	osal_msleep(2);

	osal_write32(0x190, 0x63300182, conn);
	osal_msleep(100);

	osal_read32(0x190, conn);
	osal_read32(0x180, conn);

	// hpd config
	osal_printf("%s() hpd config\n", __func__);

	osal_write32(0x18, osal_read32(0x18, conn) | BIT(28), conn);
	// osal_write32(0x84, osal_read32(0x84, conn) | BIT(16), conn);
	osal_write32(0x84, osal_read32(0x84, conn) | BIT(17), conn);
	osal_write32(0x8c, osal_read32(0x8c, conn) | BIT(28), conn);
	osal_write32(0x8c, osal_read32(0x8c, conn) | BIT(29), conn);
	osal_write32(0x8c, osal_read32(0x8c, conn) | BIT(31), conn);

	osal_read32(0x80, conn);
	osal_read32(0x88, conn);

	if (osal_read32(0x88, conn) & BIT(29) )
		osal_write32(0x88, osal_read32(0x88, conn) | BIT(29), conn);

	osal_read32(0x88, conn);

	// analog config
	osal_printf("%s() analog config\n", __func__);
	// output mode control of 4 data lanes
	osal_write32(0x1B0, osal_read32(0x1B0, conn) & ~0xF000000, conn);
	osal_write32(0x1c0, 0xf08000, conn);
	osal_write32(0x1c4, 0x00, conn);
	osal_write32(0x1c0, 0xf00000, conn);
	osal_msleep(100);

	osal_read32(0x1c0, conn);
	osal_write32(0x1DC, (osal_read32(0x1DC, conn) & ~0xFF) | 0xFF, conn);
	osal_write32(0x1DC, osal_read32(0x1DC, conn) & 0xFF0000FF, conn);

	// output mode lane2 lane3 level value 0xf
	osal_write32(0x1A4, (osal_read32(0x1A4, conn) & 0x00FFFFFF) | (0xff << 24), conn);
	// output mode lane0 lane1 level value 0xf
	osal_write32(0x1A8, (osal_read32(0x1A8, conn) & 0xFFFFFF00) | 0xff, conn);

	osal_write32(0x1A8, (osal_read32(0x1A8, conn) & 0x0000FFFF) | (0x0B0B << 16), conn);
	osal_write32(0x1AC, (osal_read32(0x1AC, conn) & 0xFFFF0000) | 0x0B0B, conn);
	osal_write32(0x1AC, (osal_read32(0x1AC, conn) & 0x0000FFFF) | (0x2222 << 16), conn);
	osal_write32(0x1B0, osal_read32(0x1B0, conn) & 0xFFFF0000, conn);
	osal_write32(0x1B0, (osal_read32(0x1B0, conn) & ~(0xF << 24)) | (0xF << 24), conn);
	osal_write32(0x1D0, (osal_read32(0x1D0, conn) & ~(0x3 << 12)) | (0x1 << 12), conn);
	osal_write32(0x100, (osal_read32(0x100, conn) & ~(0x3 << 5)) | (0x2 << 5), conn);
	osal_write32(0x100, (osal_read32(0x100, conn) & ~0x3) | 0x1, conn);

	/* video stream config*/
	osal_printf("%s()video stream config\n", __func__);

	/* video stream disable */
	osal_write32(0x200, 0x400000, conn);

	osal_write32(0x224, 0xc00029, conn);
	osal_write32(0x20c, 0x30000000, conn);
	osal_write32(0x228, 0x20, conn);
	osal_write32(0x22c, 0x00, conn);
	osal_write32(0x214, 0x1180780, conn);
	osal_write32(0x210, 0x438002d, conn);
	osal_write32(0x21c, 0x58002c, conn);
	osal_write32(0x220, 0x40005, conn);
	osal_read32(0x220, conn);

	//colorbar 148.5MZ
	// osal_write32(0x230, 0x7f0000, conn);

	// DPU 150MZ
	osal_write32(0x230, 0x7e0000, conn);
	osal_read32(0x230, conn);

	// colorbar 148.5MZ
	// osal_write32(0x218, 0x34804001, conn);

	// DPU 150MZ
	osal_write32(0x218, 0x34c04001, conn);
	osal_read32(0x218, conn);

	/* video stream enable */
	osal_write32(0x200, 0x10400000, conn);

	/* training config */
	osal_printf("%s() training config\n", __func__);

	// inno_dp_compliance_config(inno);
	inno_dp_sink_power_ctrl(conn->priv, true);
	inno_dp_link_train(conn->priv);

	// PHY SSC disable
	osal_write32(0x100, osal_read32(0x100, conn) | BIT(8), conn);

	osal_write32(0x100, osal_read32(0x100, conn) & ~(0xF << 25), conn);
	// osal_write32(0x28, osal_read32(0x28, conn) | BIT(0), conn);

	// 1080P colorbar
	// osal_printf("%s() colorbar mode\n", __func__);
	// osal_write32(0x238, 0x1021, conn);

	// DPU mode
	osal_write32(0x238, 0x1020, conn);

	return 0;
}

static void inno_dp_video_enable(struct inno_conn_t *conn)
{
	osal_write32(0x200, osal_read32(0x200, conn) | BIT(28), conn);
}

static void inno_dp_video_disable(struct inno_conn_t *conn)
{
	osal_write32(0x200, ~(BIT(28)) & osal_read32(0x200, conn), conn);
}

static void inno_dp_tx_bist(uint32_t vic, uint8_t bist_mode, struct inno_conn_t *conn)
{
#define BIST_MODE_CHECKERBOARD  0x10
#define BIST_MODE_COLORBAR  0x8

	uint32_t vid_bist_mode = BIST_MODE_CHECKERBOARD;

	if (bist_mode) {
		osal_write32(0x238,//dpureg(0x238), INNODP_BIST
			     (osal_read32(0x238, conn) & 0x7fff00c0) |
			     BIT(0) | (vic << 8) | (vid_bist_mode << 1), conn);
		osal_write32(0x28,//dpureg(0x28), INNODP_PIXEL_CLK_SEL
			     (osal_read32(0x28, conn) | BIT(0)), conn);
	} else {
		osal_write32(0x238,//dpureg(0x238), INNODP_BIST
		osal_read32(0x238, conn) & ~(BIT(0)), conn);
		osal_write32(0x28,//dpureg(0x28), INNODP_PIXEL_CLK_SEL
		osal_read32(0x28, conn) & ~(BIT(0)), conn);
	}
}


static int inno_dp_enable(struct inno_conn_t *conn)
{
	inno_dp_sink_power_ctrl(conn->priv, true);

	inno_dp_link_train(conn->priv);

	inno_dp_video_cfg(conn, &conn->out_mode);

	inno_dp_video_enable(conn);

	//inno_dp_link_start(conn->priv);

	inno_dp_tx_bist(conn->vic, 1, conn);

	conn->is_enable = true;

	return 0;
}

static int inno_dp_disable(struct inno_conn_t *conn)
{
	inno_dp_video_disable(conn);
	inno_dp_sink_power_ctrl(conn->priv, false);

	conn->is_enable = false;

	return 0;
}

struct inno_conn_func_t g_inno_dp_func = {
	.init = inno_dp_init,
	.exit = inno_dp_exit,
	.fini = NULL,
	.get_edid = inno_dp_get_edid,
	.show_edid = inno_dp_show_edid,
	.modeset = inno_dp_modeset,
	.detect = inno_dp_detect,
	.enable  = inno_dp_enable,
	.disable = inno_dp_disable,
};
