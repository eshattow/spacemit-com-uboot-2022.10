// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Spacemit Co., Ltd.
 *
 */

#include <asm/gpio.h>
#include <asm/io.h>
#include <common.h>
#include <clk.h>
#include <display.h>
#include <dm.h>
#include <edid.h>
#include <regmap.h>
#include <syscon.h>

#include <power-domain-uclass.h>
#include <power-domain.h>
#include <power/regulator.h>
#include <linux/delay.h>
#include <linux/io.h>

#include "spacemit_inno_dp.h"

extern bool is_video_connected;

static int dp_enable(struct udevice *dev, int panel_bpp,
		     const struct display_timing *edid)
{
	struct spacemit_inno_dp_priv *priv = dev_get_priv(dev);
	struct soc_dp_video_mode *mode = &priv->dp_dev.video_mode;
	unsigned long get_rate, set_rate;
	int ret;

	if (soc_dp_hw_read_sink_caps(&priv->dp_dev)) {
		pr_info("Failed to read sink caps\n");
		priv->dp_dev.link.revision = 0x14;
		priv->dp_dev.link.max_rate = SOC_DP_LINK_RATE_5_40;
		priv->dp_dev.link.max_num_lanes = SOC_DP_LANE_4;
		priv->dp_dev.link.enhanced_framing = 1;
	}

	set_rate = clk_round_rate(&priv->pxclk, edid->pixelclock.typ);
	ret = clk_set_rate(&priv->pxclk, set_rate);
	if (ret < 0) {
		pr_err("clk_set_rate pxclk %ld failed: %d\n", set_rate, ret);
		return ret;
	}

	get_rate = clk_get_rate(&priv->pxclk);
	pr_debug("%s pxclk = %ld\n", __func__, get_rate);

	set_rate = clk_round_rate(&priv->dppxclk, edid->pixelclock.typ);
	ret = clk_set_rate(&priv->dppxclk, set_rate);
	if (ret < 0) {
		pr_err("clk_set_rate dppxclk %ld failed: %d\n", set_rate, ret);
		return ret;
	}

	get_rate = clk_get_rate(&priv->dppxclk);
	pr_debug("%s dppxclk rate = %ld\n", __func__, get_rate);

	mode->clock = edid->pixelclock.typ / 1000;

	mode->hdisplay = (uint16_t)edid->hactive.typ;
	mode->hsync_start = (uint16_t)(edid->hactive.typ + edid->hfront_porch.typ);
	mode->hsync_end = (uint16_t)(mode->hsync_start + edid->hsync_len.typ);
	mode->htotal = (uint16_t)(mode->hsync_end + edid->hback_porch.typ);

	mode->vdisplay = (uint16_t)edid->vactive.typ;
	mode->vsync_start = (uint16_t)(edid->vactive.typ + edid->vfront_porch.typ);
	mode->vsync_end = (uint16_t)(mode->vsync_start + edid->vsync_len.typ);
	mode->vtotal = (uint16_t)(mode->vsync_end + edid->vback_porch.typ);

	pr_info("%s() lock %d flag 0x%x\n", __func__, mode->clock, edid->flags);
	pr_info("%s() hdisplay %d hsync_start 0x%d hsync_end %d htotal %d\n", __func__, mode->hdisplay, mode->hsync_start, mode->hsync_end, mode->htotal);
	pr_info("%s() vdisplay %d vsync_start 0x%d vsync_end %d vtotal %d\n", __func__, mode->vdisplay, mode->vsync_start, mode->vsync_end, mode->vtotal);

	mode->flags = 0;
	if (edid->flags & DISPLAY_FLAGS_HSYNC_HIGH)
		mode->flags |= SOC_DP_MODE_FLAG_PHSYNC;
	if (edid->flags & DISPLAY_FLAGS_VSYNC_HIGH)
		mode->flags |= SOC_DP_MODE_FLAG_PVSYNC;

	mode->flags |= SOC_DP_MODE_FLAG_PHSYNC;
	mode->flags |= SOC_DP_MODE_FLAG_PVSYNC;

	soc_dp_phy_power_off(&priv->dp_dev.phy);

	if (soc_dp_mode_set(&priv->dp_dev, mode) == 0) {
		soc_dp_hw_enable(&priv->dp_dev);
		pr_info("%s() successful\n", __func__);
	}

	return 0;
}

static int dp_read_edid(struct udevice *dev, uint8_t *buf, int buf_size)
{
	struct spacemit_inno_dp_priv *priv = dev_get_priv(dev);
	u32 edid_size = EDID_LENGTH;
	int ret;
	int i;

	for (i = 0; i < 3; i++) {
		ret = soc_dp_conn_get_edid_block(&priv->dp_dev, buf, 0, EDID_LENGTH);
		if (ret) {
			pr_info("EDID read failed\n");
			continue;
		}

		/*
		 * check if the EDID has an extension flag, and read additional
		 * EDID data if needed
		 */
		if (buf[EDID_EXTENSION_FLAG]) {
			edid_size += EDID_LENGTH;
			ret = soc_dp_conn_get_edid_block(&priv->dp_dev, buf + EDID_LENGTH, 1, EDID_LENGTH);
			if (ret) {
				pr_info("additional EDID Read failed!\n");
				continue;
			}
		}

		return edid_size;
	}

	return ret;
}

static int spacemit_dp_of_to_plat(struct udevice *dev)
{
	return 0;
}

static int spacemit_dp_probe(struct udevice *dev)
{
	struct spacemit_inno_dp_priv *priv = dev_get_priv(dev);
	struct power_domain pm_domain;
	void __iomem *ciu_addr, *pmu_addr;
	u32 value;
	unsigned long rate;
	unsigned long base;
	u32 id;
	u32 pix_clk;
	int ret;

	memset(priv, 0, sizeof(*priv));
	priv->base = dev_remap_addr_name(dev, "base");
	if (!priv->base)
		return -EINVAL;

	ret = dev_read_u32(dev, "dp-id", &id);
	if (ret) {
		priv->dp_id = -1;
	} else
		priv->dp_id = id;

	ret = dev_read_u32(dev, "edp-id", &id);
	if (ret) {
		priv->edp_id = -1;
	} else
		priv->edp_id = id;

	if (priv->dp_id == -1 && priv->edp_id == -1) {
		pr_err("dp-id and edp-id are not found\n");
		return -EINVAL;
	}

	ret = power_domain_get(dev, &pm_domain);
	if (ret) {
		pr_err("power_domain_get dp failed: %d", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "pxclk", &priv->pxclk);
	if (ret) {
		pr_err("clk_get_by_name pxclk failed: %d", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "mclk", &priv->mclk);
	if (ret) {
		pr_err("clk_get_by_name mclk failed: %d", ret);
		return ret;
	}

	if ((priv->dp_id == 0) || (priv->edp_id == 0)) {
		ret = clk_get_by_name(dev, "hclk", &priv->hclk);
		if (ret) {
			pr_err("clk_get_by_name hclk failed: %d", ret);
			return ret;
		}
	}

	ret = clk_get_by_name(dev, "escclk", &priv->escclk);
	if (ret) {
		pr_err("clk_get_by_name escclk failed: %d", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "dscclk", &priv->dscclk);
	if (ret) {
		pr_err("clk_get_by_name dscclk failed: %d", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "aclk", &priv->aclk);
	if (ret) {
		pr_err("clk_get_by_name aclk failed: %d", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "dppxclk", &priv->dppxclk);
	if (ret) {
		pr_err("clk_get_by_name dppxclk failed: %d", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "aclk_reset", &priv->aclk_reset);
	if (ret) {
		pr_err("reset_get_by_name aclk_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "mclk_reset", &priv->mclk_reset);
	if (ret) {
		pr_err("reset_get_by_name mclk_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "esc_reset", &priv->esc_reset);
	if (ret) {
		pr_err("reset_get_by_name esc_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "dscclk_reset", &priv->dscclk_reset);
	if (ret) {
		pr_err("reset_get_by_name dscclk_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "lcd_reset", &priv->lcd_reset);
	if (ret) {
		pr_err("reset_get_by_name lcd_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "dp_reset", &priv->dp_reset);
	if (ret) {
		pr_err("reset_get_by_name dp_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_deassert(&priv->mclk_reset);
	if (ret) {
		pr_err("reset_assert mclk_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_deassert(&priv->aclk_reset);
	if (ret) {
		pr_err("reset_assert aclk_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_deassert(&priv->esc_reset);
	if (ret) {
		pr_err("reset_assert esc_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_deassert(&priv->dscclk_reset);
	if (ret) {
		pr_err("reset_assert dscclk_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_deassert(&priv->lcd_reset);
	if (ret) {
		pr_err("reset_assert lcd_reset failed: %d\n", ret);
		return ret;
	}

	ret = clk_enable(&priv->mclk);
	if (ret < 0) {
		pr_err("clk_enable mclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_enable(&priv->escclk);
	if (ret < 0) {
		pr_err("clk_enable escclk failed: %d\n", ret);
		return ret;
	}

	if ((priv->dp_id == 0) || (priv->edp_id == 0)) {
		ret = clk_enable(&priv->hclk);
		if (ret < 0) {
			pr_err("clk_enable hclk failed: %d\n", ret);
			return ret;
		}
	}

	ret = clk_enable(&priv->dscclk);
	if (ret < 0) {
		pr_err("clk_enable dscclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_enable(&priv->aclk);
	if (ret < 0) {
		pr_err("clk_enable aclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_enable(&priv->pxclk);
	if (ret < 0) {
		pr_err("clk_enable pxclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(&priv->mclk, 307200000);
	if (ret < 0) {
		pr_err("clk_set_rate mclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(&priv->aclk, 409600000);
	if (ret < 0) {
		pr_err("clk_set_rate aclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(&priv->escclk, 51200000);
	if (ret < 0) {
		pr_err("clk_set_rate escclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(&priv->dscclk, 614400000);
	if (ret < 0) {
		pr_err("clk_set_rate dscclk failed: %d\n", ret);
		return ret;
	}

	pix_clk = dev_read_u32_default(dev, "pix-clk", 150000000);
	pr_debug("%s() set pixel clock %d \n", __func__, pix_clk);

	ret = clk_set_rate(&priv->pxclk, pix_clk);
	if (ret < 0) {
		pr_err("clk_set_rate pxclk failed: %d\n", ret);
		return ret;
	}

	ret = reset_deassert(&priv->dp_reset);
	if (ret) {
		pr_err("reset_assert dp_reset failed: %d\n", ret);
		return ret;
	}

	ret = clk_enable(&priv->dppxclk);
	if (ret < 0) {
		pr_err("clk_enable dppxclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(&priv->dppxclk, pix_clk);
	if (ret < 0) {
		pr_err("clk_set_rate dppxclk failed: %d\n", ret);
		return ret;
	}

	rate = clk_get_rate(&priv->mclk);
	pr_debug("%s clk_get_rate mclk rate = %ld\n", __func__, rate);

	rate = clk_get_rate(&priv->aclk);
	pr_debug("%s clk_get_rate aclk rate = %ld\n", __func__, rate);

	if ((priv->dp_id == 0) || (priv->edp_id == 0)) {
		rate = clk_get_rate(&priv->hclk);
		pr_debug("%s clk_get_rate hclk rate = %ld\n", __func__, rate);
	}

	rate = clk_get_rate(&priv->escclk);
	pr_debug("%s clk_get_rate escclk rate = %ld\n", __func__, rate);

	rate = clk_get_rate(&priv->dscclk);
	pr_debug("%s clk_get_rate dscclk rate = %ld\n", __func__, rate);

	rate = clk_get_rate(&priv->pxclk);
	pr_debug("%s clk_get_rate pxclk rate = %ld\n", __func__, rate);

	rate = clk_get_rate(&priv->dppxclk);
	pr_debug("%s clk_get_rate dppxclk rate = %ld\n", __func__, rate);

	ret = gpio_request_by_name(dev, "power-gpios", 0, &priv->power,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_debug("%s: Warning: cannot get power GPIO: ret=%d\n",
		      __func__, ret);
		priv->power_valid = false;
	} else {
		priv->power_valid = true;
	}

	ret = gpio_request_by_name(dev, "enable-gpios", 0, &priv->enable,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_debug("%s: Warning: cannot get enable GPIO: ret=%d\n",
		      __func__, ret);
		priv->enable_valid = false;
	} else {
		priv->enable_valid = true;
	}

	ret = gpio_request_by_name(dev, "bl-gpios", 0, &priv->bl,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_debug("%s: Warning: cannot get bl GPIO: ret=%d\n", __func__, ret);
		priv->bl_valid = false;
	} else {
		priv->bl_valid = true;
	}

	if (priv->power_valid) {
		dm_gpio_set_value(&priv->power, 1);
		mdelay(2);
	}

	if (priv->enable_valid) {
		dm_gpio_set_value(&priv->enable, 1);
		mdelay(2);
	}

	if (priv->bl_valid) {
		dm_gpio_set_value(&priv->bl, 1);
		mdelay(2);
	}

	if ((priv->edp_id == 0) || (priv->edp_id == 1)) {
		priv->dp_type = INNO_EDP;
		priv->dp_dev.edp_mode = true;
	} else {
		priv->dp_type = INNO_DP;
		priv->dp_dev.edp_mode = false;
	}

	/* mux dp0 */
	ciu_addr = (void __iomem *)0xd4282c00;
	if ((priv->dp_id == 0) || (priv->edp_id == 0)) {
		value = readl(ciu_addr + 0x12c);
		value |= BIT(8);
		writel(value, (ciu_addr + 0x12c));
	}

	/* use DP pixel clock */
	pmu_addr = (void __iomem *)0xd4282800;
	if (priv->dp_id == 0 || priv->edp_id == 0) {
		value = readl(pmu_addr + 0x23c);
		value |= BIT(2);
		writel(value, (pmu_addr + 0x23c));
		base = DP0_REGISTER_BASE_ADDRESS;
	} else if (priv->dp_id == 1 || priv->edp_id == 1) {
		value = readl(pmu_addr + 0x23c);
		value |= BIT(18);
		writel(value, (pmu_addr + 0x23c));
		base = DP1_REGISTER_BASE_ADDRESS;
	}

	soc_dp_init(&priv->dp_dev, base, SOC_DP_REF_CLK_24M, SOC_VIDEO_RGB_8BIT);
	soc_dp_phy_power_on(&priv->dp_dev.phy);
	mdelay(5);

	if (soc_dp_hw_detect_hpd(&priv->dp_dev) != connector_status_connected) {
		is_video_connected = false;
		pr_info("dp cannot get HPD signal\n");
		return -1;
	}

	is_video_connected = true;

	return 0;
}

static const struct dm_display_ops spacemit_dp_ops = {
	.read_edid = dp_read_edid,
	.enable = dp_enable,
};

static const struct udevice_id spacemit_dp_ids[] = {
	{ .compatible = "spacemit,inno-dp" },
	{ .compatible = "spacemit,inno-edp" },
	{ }
};

U_BOOT_DRIVER(spacemit_dp) = {
	.name = "spacemit_dp",
	.id = UCLASS_DISPLAY,
	.of_match = spacemit_dp_ids,
	.ops = &spacemit_dp_ops,
	.of_to_plat = spacemit_dp_of_to_plat,
	.probe = spacemit_dp_probe,
	.priv_auto	= sizeof(struct spacemit_inno_dp_priv),
};
