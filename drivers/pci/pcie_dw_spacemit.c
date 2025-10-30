// SPDX-License-Identifier: GPL-2.0+
/*
 * Spacemit DesignWare based PCIe host controller driver
 *
 * Copyright (c) 2025, spacemit Corporation.
 *
 */
#include <common.h>
#include <dm.h>
#include <log.h>
#include <pci.h>
#include <generic-phy.h>
#include <power-domain.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm-generic/gpio.h>
#include <dm/device_compat.h>
#include <linux/bitops.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <clk.h>
#include <reset.h>

#include "pcie_dw_common.h"
#include "pcie_dw_spacemit.h"

/**
 *
 * @dw: The common PCIe DW structure
 * @app_base: The base address of application register space
 */
struct spacemit_pcie {
	/* Must be first member of the struct */
	struct pcie_dw dw;

	void __iomem *app_base; /* DT app */
	void __iomem *phy_ahb; /* DT phy_ahb */

	/* reset, clock resources */
	struct clk clock;
	struct reset_ctl reset;
};

static inline u32 spacemit_pcie_readl(struct spacemit_pcie *pcie, u32 offset)
{
	return readl(pcie->app_base + offset);
}

static inline void spacemit_pcie_writel(struct spacemit_pcie *pcie, u32 offset, u32 value)
{
	writel(value, pcie->app_base + offset);
}

static inline u32 spacemit_pcie_phy_ahb_readl(struct spacemit_pcie *pcie, u32 offset)
{
	return readl(pcie->phy_ahb + offset);
}

static inline void spacemit_pcie_phy_ahb_writel(struct spacemit_pcie *pcie, u32 offset, u32 value)
{
	writel(value, pcie->phy_ahb + offset);
}

/*
 * These interfaces resemble the pci_find_*capability() interfaces, but these
 * are for configuring host controllers, which are bridges *to* PCI devices but
 * are not PCI devices themselves.
 */
static u8 __dw_pcie_find_next_cap(struct pcie_dw *pci, u8 cap_ptr, u8 cap)
{
	u8 cap_id, next_cap_ptr;
	u16 reg;

	if (!cap_ptr)
		return 0;

	reg = readw(pci->dbi_base + cap_ptr);
	cap_id = (reg & 0x00ff);

	if (cap_id > PCI_CAP_ID_MAX)
		return 0;

	if (cap_id == cap)
		return cap_ptr;

	next_cap_ptr = (reg & 0xff00) >> 8;
	return __dw_pcie_find_next_cap(pci, next_cap_ptr, cap);
}

u8 dw_pcie_find_capability(struct pcie_dw *pci, u8 cap)
{
	u8 next_cap_ptr;
	u16 reg;

	reg = readw(pci->dbi_base + PCI_CAPABILITY_LIST);
	next_cap_ptr = (reg & 0x00ff);

	return __dw_pcie_find_next_cap(pci, next_cap_ptr, cap);
}

static void spacemit_set_max_link_width(struct spacemit_pcie *pcie, int num_lanes)
{
	u32 lnkcap, lwsc, plc;
	u8 cap;

	if (!num_lanes)
		return;

	/* Set the number of lanes */
	plc = readl(pcie->dw.dbi_base + PCIE_PORT_LINK_CONTROL);
	plc &= ~PORT_LINK_FAST_LINK_MODE;
	plc |= PORT_LINK_DLL_LINK_EN;
	plc &= ~PORT_LINK_MODE_MASK;

	/* Set link width speed control register */
	lwsc = readl(pcie->dw.dbi_base + PCIE_LINK_WIDTH_SPEED_CONTROL);
	lwsc &= ~PORT_LOGIC_LINK_WIDTH_MASK;
	switch (num_lanes) {
	case 1:
		plc |= PORT_LINK_MODE_1_LANES;
		lwsc |= PORT_LOGIC_LINK_WIDTH_1_LANES;
		break;
	case 2:
		plc |= PORT_LINK_MODE_2_LANES;
		lwsc |= PORT_LOGIC_LINK_WIDTH_2_LANES;
		break;
	case 4:
		plc |= PORT_LINK_MODE_4_LANES;
		lwsc |= PORT_LOGIC_LINK_WIDTH_4_LANES;
		break;
	case 8:
		plc |= PORT_LINK_MODE_8_LANES;
		lwsc |= PORT_LOGIC_LINK_WIDTH_8_LANES;
		break;
	default:
		dev_err(pcie->dw.dev, "num-lanes %u: invalid value\n", num_lanes);
		return;
	}
	writel(plc, pcie->dw.dbi_base + PCIE_PORT_LINK_CONTROL);
	writel(lwsc, pcie->dw.dbi_base + PCIE_LINK_WIDTH_SPEED_CONTROL);

	cap = dw_pcie_find_capability(&pcie->dw, PCI_CAP_ID_EXP);
	lnkcap = readl(pcie->dw.dbi_base + cap + PCI_EXP_LNKCAP);
	lnkcap &= ~PCI_EXP_LNKCAP_MLW;
	lnkcap |= FIELD_PREP(PCI_EXP_LNKCAP_MLW, num_lanes);
	writel(lnkcap, pcie->dw.dbi_base + cap + PCI_EXP_LNKCAP);
}

/**
 * pcie_dw_configure() - Configure link capabilities and speed
 *
 * @regs_base: A pointer to the PCIe controller registers
 * @cap_speed: The capabilities and speed to configure
 *
 * Configure the link capabilities and speed in the PCIe root complex.
 */
static void pcie_dw_configure(struct spacemit_pcie *pcie, u32 cap_speed)
{
	u32 val;

	dw_pcie_dbi_write_enable(&pcie->dw, true);

	val = readl(pcie->dw.dbi_base + PCIE_LINK_CAPABILITY);
	val &= ~TARGET_LINK_SPEED_MASK;
	val |= cap_speed;
	writel(val, pcie->dw.dbi_base + PCIE_LINK_CAPABILITY);

	val = readl(pcie->dw.dbi_base + PCIE_LINK_CTL_2);
	val &= ~TARGET_LINK_SPEED_MASK;
	val |= cap_speed;
	writel(val, pcie->dw.dbi_base + PCIE_LINK_CTL_2);

	spacemit_set_max_link_width(pcie, 1);

	dw_pcie_dbi_write_enable(&pcie->dw, false);
}

/**
 * is_link_up() - Return the link state
 *
 * @regs_base: A pointer to the PCIe DBICS registers
 *
 * Return: 1 (true) for active line and 0 (false) for no link
 */
static int is_link_up(struct spacemit_pcie *pcie)
{
	u32 reg = spacemit_pcie_phy_ahb_readl(pcie, SPACEMIT_PHY_AHB_LINK_STS);

	return (reg & RDLH_LINK_UP) && (reg & SMLH_LINK_UP);
}

/**
 * wait_link_up() - Wait for the link to come up
 *
 * @regs_base: A pointer to the PCIe controller registers
 *
 * Return: 1 (true) for active line and 0 (false) for no link (timeout)
 */
static int wait_link_up(struct spacemit_pcie *pcie)
{
	unsigned long timeout;

	timeout = get_timer(0) + PCIE_LINK_UP_TIMEOUT_MS;
	while (!is_link_up(pcie)) {
		if (get_timer(0) > timeout)
			return 0;
	};

	return 1;
}

static int spacemit_pcie_link_up(struct spacemit_pcie *pcie, u32 cap_speed)
{
	u32 reg;

	if (is_link_up(pcie)) {
		printf("PCI Link already up before configuration!\n");
		return 1;
	}

	/* DW pre link configurations */
	pcie_dw_configure(pcie, cap_speed);

	/* Initiate link training */
	reg = spacemit_pcie_readl(pcie, PCIECTRL_SPACEMIT_CONF_DEVICE_CMD);
	reg &= ~APP_HOLD_PHY_RST;
	reg |= LTSSM_EN;
	spacemit_pcie_writel(pcie, PCIECTRL_SPACEMIT_CONF_DEVICE_CMD, reg);

	/* Check that link was established */
	if (!wait_link_up(pcie))
		return 0;

	/*
	 * Link can be established in Gen 1. still need to wait
	 * till MAC nagaotiation is completed
	 */
	udelay(100);

	return 1;
}

static int pcie_set_mode(struct spacemit_pcie *pcie, enum dw_pcie_device_mode mode)
{
	u32 reg;

	switch (mode) {
	case DW_PCIE_RC_TYPE:
		reg = spacemit_pcie_readl(pcie, PCIECTRL_SPACEMIT_CONF_DEVICE_CMD);
		reg |= PCIE_AUX_PWR_DET;
		spacemit_pcie_writel(pcie, PCIECTRL_SPACEMIT_CONF_DEVICE_CMD, reg);
		dev_info(pcie->dw.dev, "pcie config device cmd in rc mode: 0x%x\n", reg);

		reg = spacemit_pcie_readl(pcie, PCIE_CTRL_LOGIC);
		reg |= PCIE_IGNORE_PERSTN;
		spacemit_pcie_writel(pcie, PCIE_CTRL_LOGIC, reg);
		break;

	case DW_PCIE_EP_TYPE:
		reg = spacemit_pcie_readl(pcie, PCIECTRL_SPACEMIT_CONF_DEVICE_CMD);
		reg |= DEVICE_TYPE_RC;
		spacemit_pcie_writel(pcie, PCIECTRL_SPACEMIT_CONF_DEVICE_CMD, reg);
		break;

	default:
		dev_err(pcie->dw.dev, "INVALID device type %d\n", mode);
		return -EINVAL;
	}

	return 0;
}

static int pcie_dw_init_id(struct spacemit_pcie *pcie)
{
	dw_pcie_dbi_write_enable(&pcie->dw, true);
	writew(SPACEMIT_PCIE_VENDOR_ID, pcie->dw.dbi_base + PCI_VENDOR_ID);
	writew(SPACEMIT_PCIE_DEVICE_ID, pcie->dw.dbi_base + PCI_DEVICE_ID);
	dw_pcie_dbi_write_enable(&pcie->dw, false);

	return 0;
}

static int spacemit_pcie_host_init(struct spacemit_pcie *pcie)
{
	u32 reg;

	mdelay(100);
	/* set Perst# gpio high state*/
	reg = spacemit_pcie_readl(pcie, PCIE_CTRL_LOGIC);
	reg &= ~PCIE_PERSTN_OE;
	spacemit_pcie_writel(pcie, PCIE_CTRL_LOGIC, reg);

	return 0;
}

/**
 * spacemit_pcie_probe() - Probe the PCIe bus for active link
 *
 * @dev: A pointer to the device being operated on
 *
 * Probe for an active link on the PCIe bus and configure the controller
 * to enable this port.
 *
 * Return: 0 on success, else -ENODEV
 */
static int spacemit_pcie_probe(struct udevice *dev)
{
	struct spacemit_pcie *pcie = dev_get_priv(dev);
	struct udevice *ctlr = pci_get_controller(dev);
	struct pci_controller *hose = dev_get_uclass_priv(ctlr);
	struct phy phy0, phy1;
	int ret;
	u32 reg;

	/* enable pcie clk */
	clk_enable(&pcie->clock);
	reset_deassert(&pcie->reset);

	reg = spacemit_pcie_readl(pcie, PCIECTRL_SPACEMIT_CONF_DEVICE_CMD);
	reg &= ~LTSSM_EN;
	spacemit_pcie_writel(pcie, PCIECTRL_SPACEMIT_CONF_DEVICE_CMD, reg);

	/* set Perst# (fundamental reset) gpio low state*/
	reg = spacemit_pcie_readl(pcie, PCIE_CTRL_LOGIC);
	reg |= PCIE_PERSTN_OE;
	spacemit_pcie_writel(pcie, PCIE_CTRL_LOGIC, reg);

	ret = generic_phy_get_by_name(dev, "pcie-phy0", &phy0);
	if (ret) {
		dev_err(dev, "Unable to get phy0\n");
		return ret;
	}
	generic_phy_reset(&phy0);
	generic_phy_init(&phy0);
	generic_phy_power_on(&phy0);

	ret = generic_phy_get_by_name(dev, "pcie-phy1", &phy1);
	if (ret) {
		dev_err(dev, "Unable to get phy1\n");
		return ret;
	}
	generic_phy_reset(&phy1);
	generic_phy_init(&phy1);
	generic_phy_power_on(&phy1);

	pcie->dw.first_busno = dev_seq(dev);
	pcie->dw.dev = dev;

	pcie_set_mode(pcie, DW_PCIE_RC_TYPE);

	spacemit_pcie_host_init(pcie);
	pcie_dw_setup_host(&pcie->dw);
	pcie_dw_init_id(pcie);

	if (!spacemit_pcie_link_up(pcie, LINK_SPEED_GEN_1)) {
		printf("PCIE-%d: Link down\n", dev_seq(dev));
		return -ENODEV;
	}

	printf("PCIE-%d: Link up (Gen%d-x%d, Bus%d)\n", dev_seq(dev),
	       pcie_dw_get_link_speed(&pcie->dw),
	       pcie_dw_get_link_width(&pcie->dw),
	       hose->first_busno);

	ret = pcie_dw_prog_outbound_atu_unroll(&pcie->dw, PCIE_ATU_REGION_INDEX0, PCIE_ATU_TYPE_MEM,
					       pcie->dw.mem.phys_start, pcie->dw.mem.bus_start,
					       pcie->dw.mem.size);
	return 0;
}

/**
 * spacemit_pcie_of_to_plat() - Translate from DT to device state
 *
 * @dev: A pointer to the device being operated on
 *
 * Translate relevant data from the device tree pertaining to device @dev into
 * state that the driver will later make use of. This state is stored in the
 * device's private data structure.
 *
 * Return: 0 on success, else -EINVAL
 */
static int spacemit_pcie_of_to_plat(struct udevice *dev)
{
	int ret = 0;
	struct spacemit_pcie *pcie = dev_get_priv(dev);

	/* Get the controller base address */
	pcie->dw.dbi_base = (void *)dev_read_addr_name(dev, "dbi");
	if ((fdt_addr_t)pcie->dw.dbi_base == FDT_ADDR_T_NONE)
		return -EINVAL;

	/* Get the config space base address and size */
	pcie->dw.cfg_base = (void *)dev_read_addr_size_name(dev, "config", &pcie->dw.cfg_size);
	if ((fdt_addr_t)pcie->dw.cfg_base == FDT_ADDR_T_NONE)
		return -EINVAL;

	/* Get the iATU base address and size */
	pcie->dw.atu_base = (void *)dev_read_addr_name(dev, "atu");
	if ((fdt_addr_t)pcie->dw.atu_base == FDT_ADDR_T_NONE)
		return -EINVAL;

	/* Get the app base address */
	pcie->phy_ahb = (void *)dev_read_addr_name(dev, "phy_ahb");
	if ((fdt_addr_t)pcie->phy_ahb == FDT_ADDR_T_NONE)
		return -EINVAL;

	/* Get the PMU base address */
	pcie->app_base = (void *)dev_read_addr_name(dev, "app");
	if ((fdt_addr_t)pcie->app_base == FDT_ADDR_T_NONE)
		return -EINVAL;

	ret = clk_get_by_index(dev, 0, &pcie->clock);
	if (ret) {
		dev_err(dev, "failed to get clk: %d\n", ret);
		return ret;
	}

	ret = reset_get_by_index(dev, 0, &pcie->reset);
	if (ret) {
		dev_warn(dev, "It has no reset: %d\n", ret);
		return -EINVAL;
	}

	return 0;
}

static const struct dm_pci_ops spacemit_pcie_ops = {
	.read_config = pcie_dw_read_config,
	.write_config = pcie_dw_write_config,
};

static const struct udevice_id spacemit_pcie_ids[] = { { .compatible = "spacemit,k3-pcie" }, {} };

U_BOOT_DRIVER(spacemit_pcie_dw) = {
	.name		= "pcie_dw_spacemit",
	.id		= UCLASS_PCI,
	.of_match	= spacemit_pcie_ids,
	.ops		= &spacemit_pcie_ops,
	.of_to_plat	= spacemit_pcie_of_to_plat,
	.probe		= spacemit_pcie_probe,
	.priv_auto	= sizeof(struct spacemit_pcie),
};
