// SPDX-License-Identifier: GPL-2.0+
/*
 * Spacemit k1x DesignWare based PCIe host controller driver
 *
 * Copyright (c) 2023, spacemit Corporation.
 *
 */
#include <asm/io.h>
#include <asm-generic/gpio.h>
#include <clk.h>
#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <generic-phy.h>
#include <linux/bitops.h>
#include <linux/log2.h>
#include <pci.h>
#include <pci_ep.h>
#include <pci_ids.h>
#include <regmap.h>
#include <reset.h>
#include <syscon.h>

#include "pcie_dw_common.h"

/* Doorbell Interface */
#define DBI_OFFSET			0x0
#define DBI_SIZE			0x1000

#define PL_OFFSET			0x700

#define PHY_DEBUG_R0			(PL_OFFSET + 0x28)

#define PHY_DEBUG_R1			(PL_OFFSET + 0x2c)
#define PHY_DEBUG_R1_LINK_UP		(0x1 << 4)
#define PHY_DEBUG_R1_LINK_IN_TRAINING	(0x1 << 29)

#define PCIE_MISC_CONTROL_1		0x8bc
#define DBI_RO_WR_EN			BIT(0)

#define PCIE_CAP_BASE			0x70
#define PCI_CONFIG(r)			(DBI_OFFSET + (r))
#define PCIE_CAPABILITIES(r)		PCI_CONFIG(PCIE_CAP_BASE + (r))

/* Link capability */
#define PF0_PCIE_CAP_LINK_CAP		PCIE_CAPABILITIES(0xc)
#define PCIE_LINK_CAP_MAX_SPEED_MASK	0xf
#define PCIE_LINK_CAP_MAX_SPEED_GEN1	BIT(0)
#define PCIE_LINK_CAP_MAX_SPEED_GEN2	BIT(1)
#define PCIE_LINK_CAP_MAX_SPEED_GEN3	BIT(2)
#define PCIE_LINK_CAP_MAX_SPEED_GEN4	BIT(3)

/* PCIe controller wrapper k1x configuration registers */

#define	K1X_PHY_AHB_IRQ_EN              0x0000
#define	IRQ_EN						BIT(0)

#define	K1X_PHY_AHB_IRQSTATUS_INTX      0x0008
#define	INTA						BIT(6)
#define	INTB						BIT(7)
#define	INTC						BIT(8)
#define	INTD						BIT(9)
#define	LEG_EP_INTERRUPTS (INTA | INTB | INTC | INTD)
#define INTX_MASK				GENMASK(9, 6)
#define INTX_SHIFT					6

#define	K1X_PHY_AHB_IRQENABLE_SET_INTX  0x000c

#define	K1X_PHY_AHB_IRQSTATUS_MSI   0x0010
#define	MSI						BIT(11)
#define	PCIE_REMOTE_INTERRUPT				BIT(31)
/* DMA write channel 0~7 irq*/
#define	EDMA_INT0					BIT(0)
#define	EDMA_INT1					BIT(1)
#define	EDMA_INT2					BIT(2)
#define	EDMA_INT3					BIT(3)
#define	EDMA_INT4					BIT(4)
#define	EDMA_INT5					BIT(5)
#define	EDMA_INT6					BIT(6)
#define	EDMA_INT7					BIT(7)
/* DMA read channel 0~7 irq*/
#define	EDMA_INT8					BIT(8)
#define	EDMA_INT9					BIT(9)
#define	EDMA_INT10					BIT(10)
#define	EDMA_INT11					BIT(11)
#define	EDMA_INT12					BIT(12)
#define	EDMA_INT13					BIT(13)
#define	EDMA_INT14					BIT(14)
#define	EDMA_INT15					BIT(15)
#define	DMA_READ_INT					GENMASK(11, 8)

#define	K1X_PHY_AHB_IRQENABLE_SET_MSI			0x0014

#define	PCIECTRL_K1X_CONF_DEVICE_CMD			0x0000
#define	LTSSM_EN					BIT(6)
/* Perst input value in ep mode */
#define	PCIE_PERST_IN					BIT(7)
/* Perst GPIO en in RC mode 1: perst# low, 0: perst# high */
#define	PCIE_RC_PERST					BIT(12)
/* Wake# GPIO in EP mode 1: Wake# low, 0: Wake# high */
#define	PCIE_EP_WAKE					BIT(13)
#define	APP_HOLD_PHY_RST				BIT(30)
/* BIT31 0: EP, 1: RC*/
#define	DEVICE_TYPE_RC					BIT(31)

#define	PCIE_CTRL_LOGIC					0x0004
#define	PCIE_IGNORE_PERSTN				BIT(2)

#define	K1X_PHY_AHB_LINK_STS				0x0004
#define	SMLH_LINK_UP					BIT(1)
#define	RDLH_LINK_UP					BIT(12)

#define ADDR_INTR_STATUS1       			0x0018
#define ADDR_INTR_ENABLE1 				0x001C
#define MSI_INT						BIT(0)
#define MSIX_INT 					GENMASK(8, 1)

#define ADDR_MSI_RECV_CTRL 				0x0080
#define MSI_MON_EN 					BIT(0)
#define MSIX_MON_EN 					GENMASK(8, 1)
#define MSIX_AFIFO_FULL 				BIT(30)
#define MSIX_AFIFO_EMPTY 				BIT(29)
#define ADDR_MSI_RECV_ADDR0 				0x0084
#define ADDR_MSIX_MON_MASK 				0x0088
#define ADDR_MSIX_MON_BASE0 				0x008c

#define ADDR_MON_FIFO_DATA0 				0x00b0
#define ADDR_MON_FIFO_DATA1 				0x00b4
#define FIFO_EMPTY 					0xFFFFFFFF
#define FIFO_LEN 					32
#define INT_VEC_MASK 					GENMASK(7, 0)

#define EXP_CAP_ID_OFFSET				0x70

#define	PCIECTRL_K1X_CONF_INTX_ASSERT			0x0124
#define	PCIECTRL_K1X_CONF_INTX_DEASSERT			0x0128

/*RC write config  0xD28 offset register which equal with ELBI offset 0x028 addr*/
#define PCIE_ELBI_EP_DMA_IRQ_STATUS	0x028
#define PC_TO_EP_INT			(0x3fffffff)

#define PCIE_ELBI_EP_DMA_IRQ_MASK	0x02c
#define PC_TO_EP_INT_MASK		(0x3fffffff)

#define PCIE_ELBI_EP_MSI_REASON         0x018

struct pcie_k1x {
	/* Must be first member of the struct */
	struct pcie_dw dw;

	/* private control regs */
	void __iomem *priv_base;

    /* DT phy_ahb */
    void __iomem *phy_ahb;

	/* reset, clock resources */
	struct clk clock;
	struct reset_ctl_bulk resets;
};

enum pcie_k1x_devtype {
	K1X_PCIE_ENDPOINT_TYPE = 0,
	K1X_PCIE_HOST_TYPE = 1
};

static enum pcie_k1x_devtype pcie_k1x_get_devtype(struct pcie_k1x *k1x)
{
	u32 val;

	val = readl(k1x->priv_base + PCIECTRL_K1X_CONF_DEVICE_CMD);
    if ((val & DEVICE_TYPE_RC) == DEVICE_TYPE_RC) {
        return K1X_PCIE_HOST_TYPE;
    } else {
        return K1X_PCIE_ENDPOINT_TYPE;
    }
}

static int pcie_k1x_check_link(struct pcie_k1x *k1x)
{
	u32 reg;

	reg = readl(k1x->phy_ahb + K1X_PHY_AHB_LINK_STS);
	return (reg & RDLH_LINK_UP) && (reg & SMLH_LINK_UP);
}

static void pcie_k1x_force_gen1(struct pcie_k1x *k1x)
{
	u32 val, linkcap;

	/*
	 * Force Gen1 operation when starting the link. In case the link is
	 * started in Gen2 mode, there is a possibility the devices on the
	 * bus will not be detected at all. This happens with PCIe switches.
	 */

	/* ctrl_ro_wr_enable */
	val = readl(k1x->dw.dbi_base + PCIE_MISC_CONTROL_1);
	val |= DBI_RO_WR_EN;
	writel(val, k1x->dw.dbi_base + PCIE_MISC_CONTROL_1);

	/* configure link cap */
	linkcap = readl(k1x->dw.dbi_base + PF0_PCIE_CAP_LINK_CAP);
	linkcap |= PCIE_LINK_CAP_MAX_SPEED_MASK;
	writel(linkcap, k1x->dw.dbi_base + PF0_PCIE_CAP_LINK_CAP);

	/* ctrl_ro_wr_disable */
	val &= ~DBI_RO_WR_EN;
	writel(val, k1x->dw.dbi_base + PCIE_MISC_CONTROL_1);
}

static int pcie_k1x_wait_for_link(struct pcie_k1x *k1x)
{
	u32 val;
	int timeout;

	/* Wait for the link to train */
	mdelay(20);
	timeout = 80;

	do {
		mdelay(1);
	} while (--timeout && !pcie_k1x_check_link(k1x));

	val = readl(k1x->phy_ahb + K1X_PHY_AHB_LINK_STS);
	if (!(val & RDLH_LINK_UP) || !(val & SMLH_LINK_UP)) {
		printf("Failed to negotiate PCIe link!\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int pcie_k1x_start_link(struct pcie_k1x *k1x)
{
	u32 val;

	if (pcie_k1x_check_link(k1x))
		return -EALREADY;

	pcie_k1x_force_gen1(k1x);

	val = readl(k1x->priv_base + PCIECTRL_K1X_CONF_DEVICE_CMD);
	val |= LTSSM_EN;
	val &= ~APP_HOLD_PHY_RST;
	writel(val, k1x->priv_base + PCIECTRL_K1X_CONF_DEVICE_CMD);
	return 0;
}

static void pcie_k1x_clk_enable(struct pcie_k1x *k1x, int enable)
{
	u32 mask = 0x13f;
	u32 val = 0, enable_val = 0x3f, disable_val = 0x100;

	val = readl(k1x->priv_base);
	if (enable) {
		val |= (mask & enable_val);
		val &= ~(mask & disable_val);
	} else {
		val |= (mask & disable_val);
		val &= ~(mask & enable_val);
	}

	writel(val, k1x->priv_base);
}

static int pcie_k1x_init_port(struct udevice *dev,
				 enum pcie_k1x_devtype mode)
{
	struct pcie_k1x *k1x = dev_get_priv(dev);
	u32 reg;
    //int ret;

#if 0
	/* enable pcie clk */
	clk_enable(&k1x->clock);
#endif
    pcie_k1x_clk_enable(k1x, 1);

	/* Set desired mode while core is not operational */
	if (mode == K1X_PCIE_HOST_TYPE) {
        reg = readl(k1x->priv_base + PCIECTRL_K1X_CONF_DEVICE_CMD);
        reg |= DEVICE_TYPE_RC;
        writel(reg, k1x->priv_base + PCIECTRL_K1X_CONF_DEVICE_CMD);

        reg = readl(k1x->priv_base + PCIE_CTRL_LOGIC);
        reg |= PCIE_IGNORE_PERSTN;
        writel(reg, k1x->priv_base + PCIE_CTRL_LOGIC);
    } else {
        reg = readl(k1x->priv_base + PCIECTRL_K1X_CONF_DEVICE_CMD);
		reg &= ~DEVICE_TYPE_RC;
		writel(reg, k1x->priv_base + PCIECTRL_K1X_CONF_DEVICE_CMD);
    }

	/* Confirm desired mode from operational core */
	if (pcie_k1x_get_devtype(k1x) != mode)
		return -EINVAL;

    /* warm reset ep */
    if (mode == K1X_PCIE_HOST_TYPE) {
        reg = readl(k1x->priv_base + PCIECTRL_K1X_CONF_DEVICE_CMD);
        reg |= PCIE_RC_PERST;
        writel(reg, k1x->priv_base + PCIECTRL_K1X_CONF_DEVICE_CMD);

        mdelay(1000);

        reg = readl(k1x->priv_base + PCIECTRL_K1X_CONF_DEVICE_CMD);
        reg &= ~PCIE_RC_PERST;
        writel(reg, k1x->priv_base + PCIECTRL_K1X_CONF_DEVICE_CMD);
    }

	pcie_dw_setup_host(&k1x->dw);

	if (pcie_k1x_start_link(k1x) == -EALREADY)
		dev_info(dev, "PCIe link is already up\n");
	else if (pcie_k1x_wait_for_link(k1x) == -ETIMEDOUT)
		return -ETIMEDOUT;

	return 0;
}

static int pcie_k1x_probe(struct udevice *dev)
{
	struct pcie_k1x *k1x = dev_get_priv(dev);
	struct udevice *parent = pci_get_controller(dev);
	struct pci_controller *hose = dev_get_uclass_priv(parent);
	int err;

	k1x->dw.first_busno = dev_seq(dev);
	k1x->dw.dev = dev;

	err = pcie_k1x_init_port(dev, K1X_PCIE_HOST_TYPE);
	if (err) {
		dev_err(dev, "Failed to init port.\n");
		return err;
	}

	printf("PCIE-%d: Link up (Gen%d-x%d, Bus%d)\n",
	       dev_seq(dev), pcie_dw_get_link_speed(&k1x->dw),
	       pcie_dw_get_link_width(&k1x->dw),
	       hose->first_busno);

	return pcie_dw_prog_outbound_atu_unroll(&k1x->dw,
						PCIE_ATU_REGION_INDEX0,
						PCIE_ATU_TYPE_MEM,
						k1x->dw.mem.phys_start,
						k1x->dw.mem.bus_start,
						k1x->dw.mem.size);
}

static void __iomem *get_fdt_addr(struct udevice *dev, const char *name)
{
	fdt_addr_t addr;

	addr = dev_read_addr_name(dev, name);

	return (addr == FDT_ADDR_T_NONE) ? NULL : (void __iomem *)addr;
}

static int pcie_k1x_of_to_plat(struct udevice *dev)
{
	struct pcie_k1x *k1x = dev_get_priv(dev);
	//int err;

	/* get designware DBI base addr */
	k1x->dw.dbi_base = get_fdt_addr(dev, "dbi");
	if (!k1x->dw.dbi_base)
		return -EINVAL;

    /* Get the config space base address and size */
	k1x->dw.cfg_base = (void *)dev_read_addr_size_name(dev, "config",
							 &k1x->dw.cfg_size);
	if ((fdt_addr_t)k1x->dw.cfg_base == FDT_ADDR_T_NONE)
		return -EINVAL;

    /* Get the iATU base address and size */
	k1x->dw.atu_base = get_fdt_addr(dev, "atu");
	if (!k1x->dw.atu_base)
		return -EINVAL;

	/* get private control base addr */
	k1x->priv_base = get_fdt_addr(dev, "k1x_conf");
	if (!k1x->priv_base)
		return -EINVAL;

    k1x->phy_ahb = get_fdt_addr(dev, "phy_ahb");
    if (!k1x->phy_ahb)
		return -EINVAL;

#if 0
	err = clk_get_by_index(dev, 0, &k1x->clock);
	if (err) {
		dev_warn(dev, "It has no clk: %d\n", err);
	}
#endif

	return 0;
}

static const struct dm_pci_ops pcie_k1x_ops = {
	.read_config	= pcie_dw_read_config,
	.write_config	= pcie_dw_write_config,
};

static const struct udevice_id pcie_k1x_ids[] = {
	{ .compatible = "k1x,dwc-pcie" },
	{}
};

U_BOOT_DRIVER(pcie_spacemit) = {
	.name		= "pcie_k1x",
	.id		= UCLASS_PCI,
	.of_match	= pcie_k1x_ids,
	.ops		= &pcie_k1x_ops,
	.of_to_plat	= pcie_k1x_of_to_plat,
	.probe		= pcie_k1x_probe,
	.priv_auto	= sizeof(struct pcie_k1x),
};
