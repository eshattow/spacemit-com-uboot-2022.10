// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2025 Spacemit
 */

#include <common.h>
#include <clk.h>
#include <cpu_func.h>
#include <dm.h>
#include <errno.h>
#include <eth_phy.h>
#include <log.h>
#include <malloc.h>
#include <memalign.h>
#include <miiphy.h>
#include <net.h>
#include <netdev.h>
#include <phy.h>
#include <reset.h>
#include <wait_bit.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <dm/device-internal.h>

#include "dwc_eth_qos.h"

#define K3_APMU_BASE			0xd4282800

#define CLK_PHASE_CNT			256
#define CLK_PHASE_REVERT		180

#define TXCLK_PHASE_DEFAULT		1
#define RXCLK_PHASE_DEFAULT		1

#define TX_PHASE			1
#define RX_PHASE			0

#define GMAC_AXI_CLK_ENABLE	BIT(0)
#define GMAC_AXI_CLK_RESET	BIT(1)

#define PHY_INTF_RGMII			BIT(3)
#define PHY_INTF_GMII			BIT(4)
/*
 * only valid for rmii mode
 * 0: ref clock from external phy
 * 1: ref clock from soc
 */
#define REF_CLK_SEL			BIT(3)

/*
 * emac function clock select
 * 0: 208M
 * 1: 312M
 */
#define FUNC_CLK_SEL			BIT(4)

/* only valid for rmii, invert tx clk */
#define RMII_TX_CLK_SEL			BIT(6)

/* only valid for rmii, invert rx clk */
#define RMII_RX_CLK_SEL			BIT(7)

/*
 * only valid for rgmiii
 * 0: tx clk from rx clk
 * 1: tx clk from soc
 */
#define RGMII_TX_CLK_SEL		BIT(8)

#define PHY_IRQ_EN			BIT(12)
#define AXI_SINGLE_ID			BIT(13)

#define RMII_TX_PHASE_OFFSET		(16)
#define RMII_TX_PHASE_MASK		GENMASK(18, 16)
#define RMII_RX_PHASE_OFFSET		(20)
#define RMII_RX_PHASE_MASK		GENMASK(22, 20)

#define RGMII_TX_PHASE_OFFSET		(24)
#define RGMII_TX_PHASE_MASK		GENMASK(26, 24)
#define RGMII_RX_PHASE_OFFSET		(20)
#define RGMII_RX_PHASE_MASK		GENMASK(22, 20)

#define EMAC_RX_DLINE_EN		BIT(0)
#define EMAC_RX_DLINE_STEP_OFFSET	(4)
#define EMAC_RX_DLINE_STEP_MASK		GENMASK(5, 4)
#define EMAC_RX_DLINE_CODE_OFFSET	(8)
#define EMAC_RX_DLINE_CODE_MASK		GENMASK(15, 8)

#define EMAC_TX_DLINE_EN		BIT(16)
#define EMAC_TX_DLINE_STEP_OFFSET	(20)
#define EMAC_TX_DLINE_STEP_MASK		GENMASK(21, 20)
#define EMAC_TX_DLINE_CODE_OFFSET	(24)
#define EMAC_TX_DLINE_CODE_MASK		GENMASK(31, 24)

enum clk_tuning_way {
	/* fpga clk tuning register */
	CLK_TUNING_BY_REG,
	/* zebu/evb rgmii delayline register */
	CLK_TUNING_BY_DLINE,
	/* evb rmii only revert tx/rx clock for clk tuning */
	CLK_TUNING_BY_CLK_REVERT,
	CLK_TUNING_MAX,
};

/**
 * priv plat data
 */
struct gmac_plat_data {
	struct udevice *dev;

	void __iomem *ctrl_reg;
	void __iomem *dline_reg;

	phy_interface_t phy_iface;
	u32 phy_reset_gpio;

	u8 tx_clk_phase;
	u8 rx_clk_phase;
	u8 clk_tuning_way;

	bool clk_tuning_enable;
	bool ref_clk_frm_soc;
};

static inline void dev_set_plat_priv(struct udevice *dev, void *priv)
{
	struct eth_pdata *plat = dev_get_plat(dev);

	plat->priv_pdata = priv;
}

static inline void *dev_get_plat_priv(struct udevice *dev)
{
	struct eth_pdata *plat = dev_get_plat(dev);

	return plat->priv_pdata;
}

static bool gmac_iface_is_rmii(struct gmac_plat_data *pdata)
{
	return pdata->phy_iface == PHY_INTERFACE_MODE_RMII;
}

static int clk_phase_rmii_set(struct gmac_plat_data *pdata, bool is_tx)
{
	u32 val;

	switch (pdata->clk_tuning_way) {
	case CLK_TUNING_BY_REG:
		val = readl(pdata->ctrl_reg);
		if (is_tx) {
			val &= ~RMII_TX_PHASE_MASK;
			val |= (pdata->tx_clk_phase & 0x7) << RMII_TX_PHASE_OFFSET;
		} else {
			val &= ~RMII_RX_PHASE_MASK;
			val |= (pdata->rx_clk_phase & 0x7) << RMII_RX_PHASE_OFFSET;
		}
		writel(val, pdata->ctrl_reg);
		break;
	case CLK_TUNING_BY_CLK_REVERT:
		val = readl(pdata->ctrl_reg);
		if (is_tx) {
			if (pdata->tx_clk_phase == CLK_PHASE_REVERT)
				val |= RMII_TX_CLK_SEL;
			else
				val &= ~RMII_TX_CLK_SEL;
		} else {
			if (pdata->rx_clk_phase == CLK_PHASE_REVERT)
				val |= RMII_RX_CLK_SEL;
			else
				val &= ~RMII_RX_CLK_SEL;
		}
		writel(val, pdata->ctrl_reg);
		break;
	default:
		pr_err("wrong clk tuning way:%d !!\n", pdata->clk_tuning_way);
		return -1;
	}
	pr_debug("%s tx phase:%d rx phase:%d\n",
		__func__, pdata->tx_clk_phase, pdata->rx_clk_phase);
	return 0;
}

static int clk_phase_rgmii_set(struct gmac_plat_data *pdata, bool is_tx)
{
	u32 val;

	switch (pdata->clk_tuning_way) {
	case CLK_TUNING_BY_REG:
		val = readl(pdata->ctrl_reg);
		if (is_tx) {
			val &= ~RGMII_TX_PHASE_MASK;
			val |= (pdata->tx_clk_phase & 0x7) << RGMII_TX_PHASE_OFFSET;
		} else {
			val &= ~RGMII_RX_PHASE_MASK;
			val |= (pdata->rx_clk_phase & 0x7) << RGMII_RX_PHASE_OFFSET;
		}
		writel(val, pdata->ctrl_reg);
		break;
	case CLK_TUNING_BY_DLINE:
		val = readl(pdata->dline_reg);
		if (is_tx) {
			val &= ~EMAC_TX_DLINE_CODE_MASK;
			val |= pdata->tx_clk_phase << EMAC_TX_DLINE_CODE_OFFSET;
			val |= EMAC_TX_DLINE_EN;
		} else {
			val &= ~EMAC_RX_DLINE_CODE_MASK;
			val |= pdata->rx_clk_phase << EMAC_RX_DLINE_CODE_OFFSET;
			val |= EMAC_RX_DLINE_EN;
		}
		writel(val, pdata->dline_reg);
		break;
	default:
		pr_err("wrong clk tuning way:%d !!\n", pdata->clk_tuning_way);
		return -1;
	}
	debug("%s tx phase:%d rx phase:%d\n",
		__func__, pdata->tx_clk_phase, pdata->rx_clk_phase);
	return 0;
}

static int clk_phase_set(struct gmac_plat_data *pdata, bool is_tx)
{
	if (pdata->clk_tuning_enable) {
		if (gmac_iface_is_rmii(pdata))
			clk_phase_rmii_set(pdata, is_tx);
		else
			clk_phase_rgmii_set(pdata, is_tx);
	}
	return 0;
}

static void gmac_phy_iface_config(struct gmac_plat_data *pdata)
{
	phy_interface_t phy;
	u32 val;

	phy = pdata->phy_iface;

	val = readl(pdata->ctrl_reg);
	if (phy == PHY_INTERFACE_MODE_RMII) {
		val &= ~(PHY_INTF_RGMII | PHY_INTF_GMII);
		if (pdata->ref_clk_frm_soc)
			val |= REF_CLK_SEL;
		else
			val &= ~REF_CLK_SEL;
	} else if (phy == PHY_INTERFACE_MODE_RGMII) {
		val &= ~PHY_INTF_GMII;
		val |= PHY_INTF_RGMII;
		if (pdata->ref_clk_frm_soc)
			val |= RGMII_TX_CLK_SEL;
	} else {
		val &= ~PHY_INTF_RGMII;
		val |= PHY_INTF_GMII;
	}

	writel(val, pdata->ctrl_reg);

	pr_debug("%s val:0x%x\n", __func__, val);

	clk_phase_set(pdata, TX_PHASE);
	clk_phase_set(pdata, RX_PHASE);
}

#ifdef CONFIG_K3_BOARD_FPGA

#define K3_GPIO0_BASE 0xD4019000
#define K3_GPIO1_BASE 0xD4019040
#define K3_GPIO2_BASE 0xD4019080
#define K3_GPIO3_BASE 0xD4019100

#define K3_GPIO_PDR 0x4	/* GPIO direction register */
#define K3_GPIO_PSR 0x8	/* GPIO set register */
#define K3_GPIO_PCR 0xC	/* GPIO clear register */

static u32 __maybe_unused get_gpio_base(u8 gpio_num)
{
	switch (gpio_num / 32) {
	case 0:
		return K3_GPIO0_BASE;
	case 1:
		return K3_GPIO1_BASE;
	case 2:
		return K3_GPIO2_BASE;
	case 3:
		return K3_GPIO3_BASE;
	default:
		return 0;
	}
}
#endif

static int gmac_phy_reset(struct gmac_plat_data *pdata)
{
#ifdef CONFIG_K3_BOARD_FPGA

	void __iomem *reg;
	u32 reg_gbase = 0, bit_no = 0, val;

	reg_gbase = get_gpio_base(pdata->phy_reset_gpio);
	if (!reg_gbase) {
		pr_err("%s: invalid gpio number %u\n", __func__, pdata->phy_reset_gpio);
		return -EINVAL;
	}

	bit_no = (pdata->phy_reset_gpio) & 0x1f;

	reg =  (void *)(ulong)(reg_gbase + K3_GPIO_PDR);
	val = readl(reg);
	val |= 1 << bit_no;
	writel(val, reg);

	udelay(2);

	reg =  (void *)(ulong)(reg_gbase + K3_GPIO_PSR);
	val = readl(reg);
	val |= 1 << bit_no;
	writel(val, reg);

	reg =  (void *)(ulong)(reg_gbase + K3_GPIO_PCR);
	val = readl(reg);
	val |= 1 << bit_no;
	writel(val, reg);

	mdelay(10);

	reg =  (void *)(ulong)(reg_gbase + K3_GPIO_PSR);
	val = readl(reg);
	val |= 1 << bit_no;
	writel(val, reg);

	mdelay(10);

#else
	int ret;

	ret = gpio_direction_output(pdata->phy_reset_gpio, 1);
	if (ret < 0) {
		pr_err("gpio_direction_output(phy_reset, assert) failed: %d", ret);
		return ret;
	}

	udelay(2);

	ret = gpio_direction_output(pdata->phy_reset_gpio, 0);
	if (ret < 0) {
		pr_err("gpio_direction_output(phy_reset, deassert) failed: %d", ret);
		return ret;
	}

	mdelay(10);

	ret = gpio_direction_output(pdata->phy_reset_gpio, 1);
	if (ret < 0) {
		pr_err("gpio_direction_output(phy_reset, assert) failed: %d", ret);
		return ret;
	}

	mdelay(10);
#endif
	return 0;
}

__weak u32 spacemit_get_eqos_csr_clk(void)
{
	return 100 * 1000000;
}

static ulong eqos_get_tick_clk_rate_spacemit(struct udevice *dev)
{
	return spacemit_get_eqos_csr_clk();
}

static int eqos_probe_resources_spacemit(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	struct gmac_plat_data *pdata;
	u32 ctrl_reg, dline_reg;
	int ret = -EINVAL;

	debug("%s(dev=%p):\n", __func__, dev);

	pdata = malloc(sizeof(*pdata));
	if (!pdata)
		return -ENOMEM;
	memset(pdata, 0, sizeof(*pdata));

	pdata->dev = dev;
	dev_set_plat_priv(dev, pdata);

	/* Get PHY interface */
	pdata->phy_iface = eqos->config->interface(dev);
	if (pdata->phy_iface == PHY_INTERFACE_MODE_NA) {
		pr_err("Invalid PHY interface\n");
		goto err_free_pdata;
	}

	/* Ref clock source select */
	pdata->ref_clk_frm_soc = !dev_read_bool(dev, "ref-clock-from-phy");

	ret = dev_read_u32(dev, "phy-reset-pin", &pdata->phy_reset_gpio);
	if (ret) {
		pr_err("%s: 'phy-reset-pin' not configured in device tree!\n", __func__);
		goto err_free_pdata;
	}

	ret = dev_read_u32(dev, "ctrl-reg", &ctrl_reg);
	if (ret) {
		pr_err("%s: 'ctrl-reg' not configured in device tree!\n", __func__);
		goto err_free_pdata;
	}
	pdata->ctrl_reg = (void *)((ulong)(K3_APMU_BASE + ctrl_reg));

	pdata->clk_tuning_enable = dev_read_bool(dev, "clk_tuning_enable");
	if (pdata->clk_tuning_enable) {
		if (dev_read_bool(dev, "clk-tuning-by-reg"))
			pdata->clk_tuning_way = CLK_TUNING_BY_REG;
		else if (dev_read_bool(dev, "clk-tuning-by-clk-revert"))
			pdata->clk_tuning_way = CLK_TUNING_BY_CLK_REVERT;
		else if (dev_read_bool(dev, "clk-tuning-by-delayline")) {
			pdata->clk_tuning_way = CLK_TUNING_BY_DLINE;
			ret = dev_read_u32(dev, "dline-reg", &dline_reg);
			if (ret) {
				pr_err("%s: 'dline-reg' not configured in device tree!\n", __func__);
				goto err_free_pdata;
			}
			pdata->dline_reg = (void *)((ulong)(K3_APMU_BASE + dline_reg));
		} else
			pdata->clk_tuning_way = CLK_TUNING_BY_REG;

		pdata->tx_clk_phase = dev_read_u32_default(dev, "tx-phase", TXCLK_PHASE_DEFAULT);
		pdata->rx_clk_phase = dev_read_u32_default(dev, "rx-phase", RXCLK_PHASE_DEFAULT);

		debug("tx_phase:%d  rx_phase:%d clk_tuning:%d\n",
			pdata->tx_clk_phase, pdata->rx_clk_phase, pdata->clk_tuning_enable);
	}

#ifndef CONFIG_K3_BOARD_FPGA
	ret = clk_get_by_name(dev, "stmmaceth", &eqos->clk_master_bus);
	if (ret) {
		pr_err("clk_get_by_name(master_bus) failed: %d", ret);
		goto err_free_pdata;
	}

	ret = clk_get_by_name(dev, "ptp-clk", &eqos->clk_ptp_ref);
	if (ret) {
		pr_err("clk_get_by_name(ptp_ref) failed: %d", ret);
		goto err_free_clk_master_bus;
	}

	ret = reset_get_by_name(dev, "stmmaceth", &eqos->reset_ctl);
	if (ret) {
		pr_err("reset_get_by_name(rst) failed: %d", ret);
		goto err_free_clk_ptp_ref;
	}
#endif

	ret = gmac_phy_reset(pdata);
	if (ret) {
		pr_err("gmac_phy_reset() failed: %d\n", ret);
		goto err_free_reset;
	}

	gmac_phy_iface_config(pdata);

	debug("%s: OK\n", __func__);

	return 0;

err_free_reset:
#ifndef CONFIG_K3_BOARD_FPGA
	reset_free(&eqos->reset_ctl);
err_free_clk_ptp_ref:
	clk_free(&eqos->clk_ptp_ref);
err_free_clk_master_bus:
	clk_free(&eqos->clk_master_bus);
#endif
err_free_pdata:
	free(pdata);
	dev_set_plat_priv(dev, NULL);
	return ret;
}

static int eqos_remove_resources_spacemit(struct udevice *dev)
{
	struct gmac_plat_data *pdata = dev_get_plat_priv(dev);
#ifndef CONFIG_K3_BOARD_FPGA
	struct eqos_priv *eqos = dev_get_priv(dev);

	reset_free(&eqos->reset_ctl);
	clk_free(&eqos->clk_ptp_ref);
	clk_free(&eqos->clk_master_bus);
#endif
	if (pdata) {
		free(pdata);
		dev_set_plat_priv(dev, NULL);
	}

	return 0;
}

#ifdef CONFIG_K3_BOARD_FPGA

static int eqos_start_resets_spacemit(struct udevice *dev)
{
	struct gmac_plat_data *pdata = dev_get_plat_priv(dev);
	u32 val;

	val = readl(pdata->ctrl_reg);
	val |= GMAC_AXI_CLK_RESET;
	writel(val, pdata->ctrl_reg);

	return 0;
}

static int eqos_stop_resets_spacemit(struct udevice *dev)
{
	struct gmac_plat_data *pdata = dev_get_plat_priv(dev);
	u32 val;

	val = readl(pdata->ctrl_reg);
	val &= ~GMAC_AXI_CLK_RESET;
	writel(val, pdata->ctrl_reg);

	return 0;
}

static int eqos_start_clks_spacemit(struct udevice *dev)
{
	struct gmac_plat_data *pdata = dev_get_plat_priv(dev);
	u32 val;

	val = readl(pdata->ctrl_reg);
	val |= GMAC_AXI_CLK_ENABLE;
	writel(val, pdata->ctrl_reg);

	return 0;
}

static int eqos_stop_clks_spacemit(struct udevice *dev)
{
	struct gmac_plat_data *pdata = dev_get_plat_priv(dev);
	u32 val;

	val = readl(pdata->ctrl_reg);
	val &= ~GMAC_AXI_CLK_ENABLE;
	writel(val, pdata->ctrl_reg);

	return 0;
}

__weak int spacemit_eqos_txclk_set_rate(unsigned long rate)
{
	return 0;
}
#else

static int eqos_stop_resets_spacemit(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	int ret;

	debug("%s(dev=%p):\n", __func__, dev);

	ret = reset_assert(&eqos->reset_ctl);
	if (ret < 0) {
		pr_err("%s: reset_assert() failed: %d\n", __func__, ret);
		return ret;
	}

	debug("%s: OK\n", __func__);
	return 0;
}

static int eqos_start_resets_spacemit(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	int ret;

	debug("%s(dev=%p):\n", __func__, dev);

	ret = reset_deassert(&eqos->reset_ctl);
	if (ret < 0) {
		pr_err("reset_deassert() failed: %d", ret);
		return ret;
	}

	debug("%s: OK\n", __func__);
	return 0;
}

static int eqos_stop_clks_spacemit(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	debug("%s(dev=%p):\n", __func__, dev);

	clk_disable(&eqos->clk_ptp_ref);
	clk_disable(&eqos->clk_master_bus);

	debug("%s: OK\n", __func__);
	return 0;
}

static int eqos_start_clks_spacemit(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	int ret;

	debug("%s(dev=%p):\n", __func__, dev);

	ret = clk_enable(&eqos->clk_master_bus);
	if (ret) {
		pr_err("clk_enable(clk_master_bus) failed: %d", ret);
		return ret;
	}

	ret = clk_enable(&eqos->clk_ptp_ref);
	if (ret) {
		pr_err("clk_enable(clk_ptp_ref) failed: %d", ret);
		goto err_disable_clk_master_bus;
	}

	debug("%s: OK\n", __func__);
	return 0;

err_disable_clk_master_bus:
	clk_disable(&eqos->clk_master_bus);
	return ret;
}

/**
 * Placeholder implementation.
 */
__weak int spacemit_eqos_txclk_set_rate(unsigned long rate)
{
	return 0;
}
#endif

static int eqos_set_tx_clk_speed_spacemit(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	ulong rate;
	int ret;

	debug("%s(dev=%p):\n", __func__, dev);

	switch (eqos->phy->speed) {
	case SPEED_1000:
		rate = 125 * 1000 * 1000;
		break;
	case SPEED_100:
		rate = 25 * 1000 * 1000;
		break;
	case SPEED_10:
		rate = 2.5 * 1000 * 1000;
		break;
	default:
		pr_err("invalid speed %d", eqos->phy->speed);
		return -EINVAL;
	}

	ret = spacemit_eqos_txclk_set_rate(rate);
	if (ret < 0) {
		pr_err("spacemit (tx_clk, %lu) failed: %d", rate, ret);
		return ret;
	}

	return 0;
}

static struct eqos_ops eqos_spacemit_ops = {
	.eqos_inval_desc = eqos_inval_desc_generic,
	.eqos_flush_desc = eqos_flush_desc_generic,
	.eqos_inval_buffer = eqos_inval_buffer_generic,
	.eqos_flush_buffer = eqos_flush_buffer_generic,
	.eqos_probe_resources = eqos_probe_resources_spacemit,
	.eqos_remove_resources = eqos_remove_resources_spacemit,
	.eqos_stop_resets = eqos_stop_resets_spacemit,
	.eqos_start_resets = eqos_start_resets_spacemit,
	.eqos_stop_clks = eqos_stop_clks_spacemit,
	.eqos_start_clks = eqos_start_clks_spacemit,
	.eqos_calibrate_pads = eqos_null_ops,
	.eqos_disable_calibration = eqos_null_ops,
	.eqos_set_tx_clk_speed = eqos_set_tx_clk_speed_spacemit,
	.eqos_get_enetaddr = eqos_null_ops,
	.eqos_get_tick_clk_rate = eqos_get_tick_clk_rate_spacemit,
};

struct eqos_config __maybe_unused eqos_spacemit_k3_config = {
	.reg_access_always_ok = false,
	.mdio_wait = 10,
	.swr_wait = 50,
	.config_mac = EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB,
	.config_mac_mdio = EQOS_MAC_MDIO_ADDRESS_CR_250_300,
	.axi_bus_width = EQOS_AXI_WIDTH_64,
	.interface = dev_read_phy_mode,
	.ops = &eqos_spacemit_ops
};
