// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for Spacemit K1x Mobile Storage Host Controller
 *
 * Copyright (C) 2023 Spacemit Inc.
 */
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <fdtdec.h>
#include <linux/libfdt.h>
#include <linux/delay.h>
#include <malloc.h>
#include <sdhci.h>
#include <reset-uclass.h>

DECLARE_GLOBAL_DATA_PTR;

/* SDH registers define */
#define SDHC_OP_EXT_REG			0x108
#define OVRRD_CLK_OEN			0x0800
#define FORCE_CLK_ON			0x1000

#define SDHC_LEGACY_CTRL_REG		0x10C
#define GEN_PAD_CLK_ON			0x0040

#define SDHC_MMC_CTRL_REG		0x114
#define MISC_INT_EN			0x0002
#define MISC_INT			0x0004
#define ENHANCE_STROBE_EN		0x0100
#define MMC_HS400			0x0200
#define MMC_HS200			0x0400
#define MMC_CARD_MODE			0x1000

#define SDHC_TX_CFG_REG			0x11C
#define TX_INT_CLK_SEL			0x40000000
#define TX_MUX_SEL			0x80000000

#define SDHC_PHY_CTRL_REG		0x160
#define PHY_FUNC_EN			0x0001
#define PHY_PLL_LOCK			0x0002
#define HOST_LEGACY_MODE		0x80000000

#define SDHC_PHY_FUNC_REG		0x164
#define PHY_TEST_EN			0x0080
#define HS200_USE_RFIFO			0x8000

#define SDHC_PHY_DLLCFG			0x168
#define DLL_PREDLY_NUM			0x04
#define DLL_FULLDLY_RANGE		0x10
#define DLL_VREG_CTRL			0x40
#define DLL_ENABLE			0x80000000
#define DLL_REFRESH_SWEN_SHIFT		0x1C
#define DLL_REFRESH_SW_SHIFT		0x1D

#define SDHC_PHY_DLLCFG1		0x16C
#define DLL_REG2_CTRL			0x0C
#define DLL_REG3_CTRL_MASK		0xFF
#define DLL_REG3_CTRL_SHIFT		0x08

#define SDHC_PHY_DLLSTS			0x170
#define DLL_LOCK_STATE			0x01

#define SDHC_PHY_DLLSTS1		0x174
#define DLL_MASTER_DELAY_MASK		0xFF
#define DLL_MASTER_DELAY_SHIFT		0x10

#define RPM_DELAY			50
#define MAX_74CLK_WAIT_COUNT		74

#define MMC1_IO_V18EN			0x04
#define AKEY_ASFAR			0xBABA
#define AKEY_ASSAR			0xEB10

#define SDHC_RX_CFG_REG			0x118
#define RX_SDCLK_SEL0_MASK		0x03
#define RX_SDCLK_SEL0_SHIFT		0x00
#define RX_SDCLK_SEL0			0x02
#define RX_SDCLK_SEL1_MASK		0x03
#define RX_SDCLK_SEL1_SHIFT		0x02
#define RX_SDCLK_SEL1			0x01

#define SDHC_DLINE_CTRL_REG		0x130
#define DLINE_PU			0x01
#define RX_DLINE_CODE_MASK		0xFF
#define RX_DLINE_CODE_SHIFT		0x10
#define TX_DLINE_CODE_MASK		0xFF
#define TX_DLINE_CODE_SHIFT		0x18

#define SDHC_DLINE_CFG_REG		0x134
#define RX_DLINE_REG_MASK		0xFF
#define RX_DLINE_REG_SHIFT		0x00
#define RX_DLINE_GAIN_MASK		0x1
#define RX_DLINE_GAIN_SHIFT		0x8
#define RX_DLINE_GAIN			0x1
#define TX_DLINE_REG_MASK		0xFF
#define TX_DLINE_REG_SHIFT		0x10

#define SDHC_RX_TUNE_DELAY_MIN		0x0
#define SDHC_RX_TUNE_DELAY_MAX		0xFF
#define SDHC_RX_TUNE_DELAY_STEP		0x1


struct spacemit_sdhci_plat {
	struct mmc_config cfg;
	struct mmc mmc;
	struct reset_ctl_bulk resets;
	struct clk_bulk clks;
};

struct spacemit_sdhci_priv {
	struct sdhci_host host;
	u32 phy_module;
};

/*
 * refer to PMU_SDH0_CLK_RES_CTRL<0x054>, SDH0_CLK_SEL:0x0, SDH0_CLK_DIV:0x1
 * the default clock source is 200MHz [400MHz(pll1_400Mhz)/2]
 *
 * in the start-up phase, use the 200KHz frequency
 */
#define SDHC_DEFAULT_MAX_CLOCK (200*1000*1000)
#define SDHC_MIN_CLOCK (200*1000)

static int is_emulator_platform(void)
{
#ifdef CONFIG_K1_X_BOARD_FPGA
	return 1;
#else
	return 0;
#endif
}

static void set_emmc_phy_bypass(struct sdhci_host *host)
{
	unsigned int value = 0;

	/* set emmc phy bypass if need */
	if (is_emulator_platform()) {
		/* phy bypass */
		value = sdhci_readl (host, SDHC_TX_CFG_REG);
		value |= TX_INT_CLK_SEL;
		sdhci_writel (host, value, SDHC_TX_CFG_REG);

		value = sdhci_readl (host, SDHC_PHY_CTRL_REG);
		value |= HOST_LEGACY_MODE;
		sdhci_writel (host, value, SDHC_PHY_CTRL_REG);

		value = sdhci_readl (host, SDHC_PHY_FUNC_REG);
		value |= PHY_TEST_EN;
		sdhci_writel (host, value, SDHC_PHY_FUNC_REG);

		printf("%s: emmc phy bypass.\n", host->name);
	} else {
		/* phy func mode */
		value = sdhci_readl(host, SDHC_PHY_CTRL_REG);
		value |= (PHY_FUNC_EN | PHY_PLL_LOCK);
		sdhci_writel(host, value, SDHC_PHY_CTRL_REG);
	}
}

#define MAX_WAIT_COUNT 100
int spacemit_set_sdh_74_clk(struct udevice *dev)
{
	struct spacemit_sdhci_priv *priv = dev_get_priv(dev);
	struct sdhci_host *host = &priv->host;
	u32 tmp = 0;
	int count = 0;

	tmp = sdhci_readl(host, SDHC_MMC_CTRL_REG);
	tmp |= MISC_INT | MISC_INT_EN;
	sdhci_writel(host, tmp, SDHC_MMC_CTRL_REG);

	tmp = sdhci_readl(host, SDHC_LEGACY_CTRL_REG);
	tmp |= GEN_PAD_CLK_ON;
	sdhci_writel(host, tmp, SDHC_LEGACY_CTRL_REG);

	while (count < MAX_WAIT_COUNT) {
		if (sdhci_readl(host, SDHC_MMC_CTRL_REG) & MISC_INT)
			break;

		udelay(10);
		count++;
	}
	if (count >= MAX_WAIT_COUNT)
		printf("%s: 74 clk wait timeout(%d)\n", host->name, count);
	return 0;
}

static int spacemit_sdhci_probe(struct udevice *dev)
{
	struct spacemit_sdhci_plat *plat = dev_get_plat(dev);
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct spacemit_sdhci_priv *priv = dev_get_priv(dev);
	struct sdhci_host *host = &priv->host;
	struct dm_mmc_ops *mmc_driver_ops = (struct dm_mmc_ops *)dev->driver->ops;
	u32 value;
	int ret = 0;

	host->mmc = &plat->mmc;
	host->mmc->priv = host;
	host->mmc->dev = dev;
	upriv->mmc = host->mmc;

	ret = reset_get_bulk(dev, &plat->resets);
	if (ret) {
		pr_err("Can't get reset: %d\n", ret);
		return ret;
	}

	ret = reset_deassert_bulk(&plat->resets);
	if (ret) {
		pr_err("Failed to reset: %d\n", ret);
		return ret;
	}

	ret = clk_get_bulk(dev, &plat->clks);
	if (ret) {
		pr_err("Can't get clk: %d\n", ret);
		return ret;
	}

	ret = clk_enable_bulk(&plat->clks);
	if (ret) {
		pr_err("Failed to enable clk: %d\n", ret);
		return ret;
	}

	/* Set quirks */
	host->quirks = SDHCI_QUIRK_WAIT_SEND_CMD | SDHCI_QUIRK_32BIT_DMA_ADDR;
	host->host_caps = MMC_MODE_HS | MMC_MODE_HS_52MHz;
	host->max_clk = SDHC_DEFAULT_MAX_CLOCK;

	plat->cfg.f_max = SDHC_DEFAULT_MAX_CLOCK;
	plat->cfg.f_min = SDHC_MIN_CLOCK;

	mmc_driver_ops->deferred_probe = spacemit_set_sdh_74_clk;

	ret = sdhci_setup_cfg(&plat->cfg, host, SDHC_DEFAULT_MAX_CLOCK, SDHC_MIN_CLOCK);
	if (ret)
		return ret;

	ret = sdhci_probe(dev);
	if (ret)
		return ret;

	/* emmc phy bypass if need */
	if (priv->phy_module) {
		printf("%s: set phy module.\n", host->name);
		set_emmc_phy_bypass(host);
	} else {
		printf("%s: not support phy module.\n", host->name);
		value = sdhci_readl (host, SDHC_TX_CFG_REG);
		value |= TX_INT_CLK_SEL;
		sdhci_writel (host, value, SDHC_TX_CFG_REG);
	}

	printf("%s: probe done.\n", host->name);
	return ret;
}

static int spacemit_sdhci_ofdata_to_platdata(struct udevice *dev)
{
	struct spacemit_sdhci_plat *plat = dev_get_plat(dev);
	struct spacemit_sdhci_priv *priv = dev_get_priv(dev);
	struct sdhci_host *host = &priv->host;
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(dev);
	int ret = 0;

	host->name = dev->name;
	host->ioaddr = (void *)devfdt_get_addr(dev);
	priv->phy_module = fdtdec_get_uint(blob, node, "sdh-phy-module", 0);

	ret = mmc_of_parse(dev, &plat->cfg);
	if (ret)
		return ret;

	return ret;
}

static int spacemit_sdhci_bind(struct udevice *dev)
{
	struct spacemit_sdhci_plat *plat = dev_get_plat(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct udevice_id spacemit_sdhci_ids[] = {
	{ .compatible = "spacemit,k1-x-sdhci" },
	{ }
};

U_BOOT_DRIVER(spacemit_sdhci_drv) = {
	.name		= "spacemit_sdhci",
	.id		= UCLASS_MMC,
	.of_match	= spacemit_sdhci_ids,
	.of_to_plat = spacemit_sdhci_ofdata_to_platdata,
	.ops		= &sdhci_ops,
	.bind		= spacemit_sdhci_bind,
	.probe		= spacemit_sdhci_probe,
	.priv_auto = sizeof(struct spacemit_sdhci_priv),
	.plat_auto = sizeof(struct spacemit_sdhci_plat),
};
