// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#include <asm/gpio.h>
#include <asm/io.h>
#include <common.h>
#include <clk.h>
#include <display.h>
#include <dm.h>
//#include <dw_hdmi.h>
#include <edid.h>
#include <regmap.h>
#include <syscon.h>

#include <power-domain-uclass.h>
#include <power-domain.h>
#include <power/regulator.h>
#include <linux/delay.h>
#include <linux/io.h>

#include "spacemit_hdmi.h"


static int hdmi_enable(struct udevice *dev, int panel_bpp,
			      const struct display_timing *edid)
{
	void __iomem *hdmi_addr;
	hdmi_addr = ioremap(0xC0400500, 0x200);
	u32 value;

	// hdmi phy param config
	#if 0

	writel(0x4d, hdmi_addr + 0x34);
	writel(0x20200000, hdmi_addr + 0xe8);
	writel(0x509D453E, hdmi_addr + 0xec);
	writel(0x821, hdmi_addr + 0xf0);
	writel(0x3, hdmi_addr + 0xe4);

	udelay(2);
	value = readl(hdmi_addr + 0xe4);
	debug("%s() hdmi 0xe4 0x%x\n", __func__, value);

	writel(0x30184000, hdmi_addr + 0x28);

	#else

	writel(0xEE40410F, hdmi_addr + 0xe0);
	writel(0x0000005d, hdmi_addr + 0x34);
	writel(0x2022C000, hdmi_addr + 0xe8);
	writel(0x508D414D, hdmi_addr + 0xec);

	writel(0x00000901, hdmi_addr + 0xf0);
	writel(0x3, hdmi_addr + 0xe4);

	udelay(2);
	value = readl(hdmi_addr + 0xe4);
	debug("%s() hdmi 0xe4 0x%x\n", __func__, value);

	writel(0x3018C001, hdmi_addr + 0x28);

	#endif

	udelay(1000);

	return 0;
}

int hdmi_read_edid(struct udevice *dev, u8 *buf, int buf_size)
{
	return 0;
}

static int spacemit_hdmi_of_to_plat(struct udevice *dev)
{
	return 0;
}

static int spacemit_hdmi_probe(struct udevice *dev)
{
	struct spacemit_hdmi_priv *priv = dev_get_priv(dev);
	struct power_domain pm_domain;
	unsigned long rate;
	int ret;

	ret = power_domain_get(dev, &pm_domain);
	if (ret) {
		pr_err("power_domain_get hdmi failed: %d", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "hmclk", &priv->hdmi_mclk);
	if (ret) {
		pr_err("clk_get_by_name hdmi mclk failed: %d", ret);
		return ret;
	}

	ret = clk_enable(&priv->hdmi_mclk);
	if (ret < 0) {
		pr_err("clk_enable hdmi mclk failed: %d\n", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "hdmi_reset", &priv->hdmi_reset);
	if (ret) {
		pr_err("reset_get_by_name hdmi reset failed: %d\n", ret);
		return ret;
	}
	ret = reset_deassert(&priv->hdmi_reset);
	if (ret) {
		pr_err("reset_assert hdmi reset failed: %d\n", ret);
		goto free_reset;
	}

	rate = clk_get_rate(&priv->hdmi_mclk);
	debug("%s clk_get_rate hdmi mclk %ld\n", __func__, rate);

	return ret;

free_reset:
	clk_disable(&priv->hdmi_mclk);

	return 0;
}

static const struct dm_display_ops spacemit_hdmi_ops = {
	.read_edid = hdmi_read_edid,
	.enable = hdmi_enable,
};

static const struct udevice_id spacemit_hdmi_ids[] = {
	{ .compatible = "spacemit,hdmi" },
	{ }
};

U_BOOT_DRIVER(spacemit_hdmi) = {
	.name = "spacemit_hdmi",
	.id = UCLASS_DISPLAY,
	.of_match = spacemit_hdmi_ids,
	.ops = &spacemit_hdmi_ops,
	.of_to_plat = spacemit_hdmi_of_to_plat,
	.probe = spacemit_hdmi_probe,
	.priv_auto	= sizeof(struct spacemit_hdmi_priv),
};
