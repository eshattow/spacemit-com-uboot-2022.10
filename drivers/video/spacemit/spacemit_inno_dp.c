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

static int dp_phy_wait_for_hpd(struct spacemit_inno_dp_priv *priv)
{
	ulong start;

	pr_debug("%s() \n", __func__);

	start = get_timer(0);
	do {
		if (inno_conn_detect(priv->dp_conn)) {
			pr_info("%s() dp get hpd signal \n", __func__);
			return 0;
		}
		udelay(100);
	} while (get_timer(start) < 100);

	return -1;
}

static int dp_enable(struct udevice *dev, int panel_bpp,
		     const struct display_timing *edid)
{
	struct spacemit_inno_dp_priv *priv = dev_get_priv(dev);

	inno_conn_enable(priv->dp_conn);

	return 0;
}

static int dp_read_timing(struct udevice *dev,
			  struct display_timing *timing)
{
	struct spacemit_inno_dp_priv *priv = dev_get_priv(dev);
	struct inno_mode *mode = &priv->dp_conn->out_mode;

	inno_conn_prepare(priv->dp_conn);

	timing->pixelclock.typ = mode->clock;
	timing->hback_porch.typ = mode->htotal - mode->hsync_end;
	timing->hfront_porch.typ = mode->hsync_start - mode->hdisplay;
	timing->hsync_len.typ = mode->hsync_end - mode->hsync_start;
	timing->vback_porch.typ = mode->vtotal - mode->vsync_end;
	timing->vfront_porch.typ = mode->vsync_start - mode->vdisplay;
	timing->vsync_len.typ = mode->vsync_end - mode->vsync_start;

	timing->hactive.typ = mode->hdisplay;
	timing->vactive.typ = mode->vdisplay;

	return 0;
}

static int spacemit_dp_of_to_plat(struct udevice *dev)
{
	return 0;
}

static int spacemit_dp_probe(struct udevice *dev)
{
	struct spacemit_inno_dp_priv *priv = dev_get_priv(dev);
	struct power_domain pm_domain;
	unsigned long rate;
	int ret;

	pr_debug("%s() \n", __func__);

	priv->base = dev_remap_addr_name(dev, "dp");
	if (!priv->base)
		return -EINVAL;

	priv->dp_id = dev_read_u32_default(dev, "dpu-id", 0);

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

	if (priv->dp_id == 0) {
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

	if (priv->dp_id == 0) {
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

	ret = clk_set_rate(&priv->pxclk, 148500000);
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

	ret = clk_set_rate(&priv->dppxclk, 148500000);
	if (ret < 0) {
		pr_err("clk_set_rate dppxclk failed: %d\n", ret);
		return ret;
	}

	rate = clk_get_rate(&priv->mclk);
	pr_info("%s clk_get_rate mclk rate = %ld\n", __func__, rate);

	rate = clk_get_rate(&priv->aclk);
	pr_info("%s clk_get_rate aclk rate = %ld\n", __func__, rate);

	if (priv->dp_id == 0) {
		rate = clk_get_rate(&priv->hclk);
		pr_info("%s clk_get_rate hclk rate = %ld\n", __func__, rate);
        }

	rate = clk_get_rate(&priv->escclk);
	pr_info("%s clk_get_rate escclk rate = %ld\n", __func__, rate);

	rate = clk_get_rate(&priv->dscclk);
	pr_info("%s clk_get_rate dscclk rate = %ld\n", __func__, rate);

	rate = clk_get_rate(&priv->pxclk);
	pr_info("%s clk_get_rate pxclk rate = %ld\n", __func__, rate);

	rate = clk_get_rate(&priv->dppxclk);
	pr_info("%s clk_get_rate dppxclk rate = %ld\n", __func__, rate);

	priv->dp_conn = inno_get_conn_module(INNO_CONN_DP);
	priv->dp_type = INNO_DP;

	inno_conn_init(priv->dp_conn);

	if (priv->dp_type == INNO_DP) {
		ret = dp_phy_wait_for_hpd(priv);
		is_video_connected = (ret >= 0);
	} else
		is_video_connected = true;

	if (!is_video_connected) {
		pr_info("dp cannot get HPD signal\n");
		return ret;
	}

	return 0;
}

static const struct dm_display_ops spacemit_dp_ops = {
	.read_timing = dp_read_timing,
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
