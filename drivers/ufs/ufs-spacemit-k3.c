// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
 * spacemit_k3 UFS host controller driver
 *
 * Copyright (C) 2025 Spacemit Technology Co., Ltd.
 */

#include <clk.h>
#include <dm.h>
#include <ufs.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/bug.h>
#include <linux/iopoll.h>
#include <asm/unaligned.h>
#include "ufs.h"

struct spacemit_k3_ufs_priv {
	struct clk aclk;
	u32 phy_mng_base;
	u32 atop_base;
};

/*UFS PMUAP_REG*/
#define PMU_MAIN_BASE 0xD4050000
#define APMU_BASE 0xD4282800
#define APB_SPARE_BASE 0xD4090000

#define ACGR_REG 0x1024
#define APB_SPARE8_REG 0x11c
#define APB_SPARE2_REG 0x104
#define PMU_UFS_CLK_RES_CTRL_REG 0x268

#define PLAT_EMU_UFS_ACLK_RST_SHIFT 9
#define PLAT_EMU_UFS_ACLK_EN_SHIFT 10
#define PLAT_EMU_UFS_ACLK_SEL_SHIFT 11
#define PLAT_EMU_UFS_ACLK_DIV_SHIFT 14
#define PLAT_EMU_UFS_ACLK_FC_SHIFT 17

#define PLAT_UFS_ACLK_RST_SHIFT 0
#define PLAT_UFS_ACLK_EN_SHIFT 1
#define PLAT_UFS_ACLK_SEL_SHIFT 2
#define PLAT_UFS_ACLK_DIV_SHIFT 5
#define PLAT_UFS_ACLK_FC_SHIFT 8

#define UFS_CLK_SEL 0
#define UFS_CLK_DIV 1

#define UFS_CLK_SEL_SHIFT 2
#define UFS_CLK_DIV_SHIFT 5
/*UFS HOST PHY REGISTER*/
#define UFS_ARASAN_TOP_BASE 0x1C00
#define UFS_ARASAN_PHY_MNG_BASE 0x1B00

#define UFS_MPHY_RST_CTRL 0x0
#define UFS_MPHY_PU_CTRL 0x4
#define UFS_MPHY_BKDR_CTRL 0x8
#define UFS_DEVICE_IO_CTRL 0xc
/*UFS HOST LOGIC REGISTER*/

/*on arasan*/
#define UFS_SYS1CLK_1US_REG 0xC0
#define UFS_TX_SYMBOL_CLK_NS_US_REG 0xC4
#define UFS_LOCAL_PORT_ID_REG 0xC8
#define UFS_PA_ERR_CODE_REG 0xCC
#define UFS_RETRY_TIMER_REG 0xD0
#define UFS_PA_LINK_STARTUP_TIMER_REG 0xD8
#define UFS_CFG1_REG 0xDC

#define UFS_BOOT_LU_SIZE 32

#define UFSHCD_HCE_UIC_PWR_MASK                                                                    \
	(UIC_HIBERNATE_ENTER | UIC_HIBERNATE_EXIT | UIC_POWER_MODE | UIC_COMMAND_COMPL)

#define UFSHCD_LINK_ALL_MASK                                                                       \
	(UFSHCD_HCE_UIC_PWR_MASK | UTP_TRANSFER_REQ_COMPL | UIC_ERROR | DEVICE_FATAL_ERROR |       \
	 CONTROLLER_FATAL_ERROR | SYSTEM_BUS_FATAL_ERROR)

/* PA Layer Gettable and settable M-PHY Specific Attributes */
#define PA_TXHSG1SYNCLENGTH 0x1552
#define PA_TXHSG1PREPARELENGTH 0x1553
#define PA_TXHSG2SYNCLENGTH 0x1554
#define PA_TXHSG2PREPARELENGTH 0x1555
#define PA_TXHSG3SYNCLENGTH 0x1556
#define PA_TXHSG3PREPARELENGTH 0x1557
#define PA_TXMK2EXTENSION 0x155A
#define PA_PEERSCRAMBLING 0x155B
#define PA_TXSKIP 0x155C
#define PA_TXSKIPPERIOD 0x155D
#define PA_PEER_TX_LCC_ENABLE 0x155F

#define PA_SCRAMBLING 0x1585
#define PA_MK2EXTENSIONGUARDBAND 0x15AB

/*special TX/RX Configuration Attributes*/
#define RX_LS_PRE_LEN_CAP 0x008D
#define RX_LANE_HB8_BKDOOR_ATTR 0x00F4
#define RX_PWRM_CLOSURE_LEN_CAP 0x008E
#define RX_MIN_STALL_CAP 0x0088
#define RX_LANE_SOF_BKDOOR_ATT 0x00F2
#define RX_LS_PREPARELEN_TIME 0x008D
#define RX_GARBAGE_COUNT_OFFSET 0x00F2
#define VS_TX_BURST_CLOSURE_DELAY 0xD084

#define UFS_LOGICAL_BLOCK_SIZE 512

/*special analog reg*/
#define ANA_EQ_CTRL_REG_ATTR 0x00CD
#define ANA_HSGEAR_CTRL_ATTR 0x00C1

/* UFS_MPHY_PU_CTRL bit definitions */
#define UFS_MPHY_PU_PLL_LOCK BIT(31)

/* UFS controller timing constants for 409MHz SYS1CLK */
#define UFS_SYS1CLK_1US_409MHZ		409		/* 1us worth of SYS1CLK cycles at ~409MHz */
#define UFS_TX_SYMBOL_CLK_NS_US_409MHZ	0x800	/* TX symbol clock ns/us ratio for 409MHz */
#define UFS_PA_LINK_STARTUP_TIMER_MAX	0xffffffff	/* Max PA link startup timer */

extern int ufshcd_query_descriptor_retry(struct ufs_hba *hba, enum query_opcode opcode,
					 enum desc_idn idn, u8 index, u8 selector, u8 *desc_buf,
					 int *buf_len);

static void spacemit_k3_ufs_clk_enable(struct spacemit_k3_ufs_priv *priv)
{
	int ret;

	ret = clk_enable(&priv->aclk);
	if (ret) {
		pr_err("ufs: fail to enable ufs aclk, ret=%d\n", ret);
		return;
	}

	/* HYNIX1 phone need delay*/
	mdelay(5);
}

static void spacemit_k3_ufs_clk_disable(struct spacemit_k3_ufs_priv *priv)
{
	/*disable ufs aclk*/
	clk_disable(&priv->aclk);
	pr_info("ufs: clk disable ufs aclk\n");
}

static int __maybe_unused debug_print_desc(struct udevice *dev, enum desc_idn idn)
{
	u8 *desc_buf;

	int ret = 0;
	struct ufs_hba *hba = dev_get_uclass_priv(dev);
	int desc_size;

	switch (idn) {
	case QUERY_DESC_IDN_CONFIGURATION:
		desc_size = hba->desc_size.conf_desc;
		break;
	case QUERY_DESC_IDN_DEVICE:
		desc_size = hba->desc_size.dev_desc;
		break;
	case QUERY_DESC_IDN_UNIT:
		desc_size = hba->desc_size.unit_desc;
		break;
	case QUERY_DESC_IDN_GEOMETRY:
		desc_size = hba->desc_size.geom_desc;
		break;
	default:
		dev_err(hba->dev, "Invalid descriptor ID\n");
		return -EINVAL;
	}

	desc_buf = kmalloc(desc_size, GFP_KERNEL);
	if (!desc_buf) {
		ret = -ENOMEM;
		goto out;
	}

	if (idn != QUERY_DESC_IDN_UNIT) {
		ret = ufshcd_query_descriptor_retry(hba, UPIU_QUERY_OPCODE_READ_DESC, idn, 0, 0,
						    desc_buf, &desc_size);
		if (ret) {
			dev_err(hba->dev, "%s:FAILed read descriptor%d\n", __func__, ret);
			return ret;
		}

		pr_info("ufs: debug print descriptor for idn %d\n", idn);

		for (int i = 0; i < hba->desc_size.conf_desc; i++) {
			pr_info("[%x]:%x  ", i, desc_buf[i]);
			if ((i + 1) % 8 == 0) {
				pr_info("\n");
			}
		}
	} else {
		printf("ufs: debug print descriptor for idn %d\n", idn);
		for (int i = 0; i < 8; i++) {
			ret = ufshcd_query_descriptor_retry(hba, UPIU_QUERY_OPCODE_READ_DESC, idn,
							    i, 0, desc_buf, &desc_size);
			if (ret) {
				dev_err(hba->dev, "%s:FAILed read descriptor%d\n", __func__, ret);

				return ret;
			}

			pr_info("ufs: unit %d descriptor\n", i);
			for (int i = 0; i < hba->desc_size.conf_desc; i++) {
				pr_info("[%x]:%x  ", i, desc_buf[i]);
				if ((i + 1) % 8 == 0) {
					pr_info("\n");
				}
			}
		}
	}
out:
	kfree(desc_buf);
	return ret;
}

static int __maybe_unused spacemit_k3_ufs_config_lun(struct udevice *dev)
{
	u8 *desc_buf;
	uint64_t qTotalRawDeviceCapacity;
	uint32_t dSegmentSize, boot_lun_size, user_lun_size;
	uint8_t bAllocationUnitSize, bMaxNumberLU;
	int ret;
	uint32_t alloc_unit_bytes;
	struct ufs_hba *hba = dev_get_uclass_priv(dev);

	desc_buf = kmalloc(hba->desc_size.geom_desc, GFP_KERNEL);
	if (!desc_buf) {
		ret = -ENOMEM;
		goto out;
	}

	/*read ufs size info from greometry*/
	ret = ufshcd_query_descriptor_retry(hba, UPIU_QUERY_OPCODE_READ_DESC,
					    QUERY_DESC_IDN_GEOMETRY, 0, 0, desc_buf,
					    &hba->desc_size.geom_desc);
	qTotalRawDeviceCapacity = get_unaligned_be64(&desc_buf[GEO_DESC_PARAM_TOTAL_RAW_DEV_CAP]);
	dSegmentSize = get_unaligned_be32(&desc_buf[GEO_DESC_PARAM_SEG_SIZE]);
	bAllocationUnitSize = desc_buf[GEO_DESC_PARAM_ALLOC_UNIT_SIZE];
	bMaxNumberLU = desc_buf[GEO_DESC_PARAM_MAX_NUM_LUN];
	pr_info("ufs: qTotalRawDeviceCapacity: %llu \ndSegmentSize:%u\n bAllocationUnitSize:%d\n "
		"bMaxNumberLU%x\n",
		qTotalRawDeviceCapacity, dSegmentSize, bAllocationUnitSize, bMaxNumberLU);
	kfree(desc_buf);

	/*
	 * 1.set lu0, lu1, lu2 enable
	 * 2.set lu0-lu7 logicblocksize = 4k, bdataliability = 0x1, bprovisiontype = 0x3
	 */
	desc_buf = kmalloc(hba->desc_size.conf_desc, GFP_KERNEL);
	if (!desc_buf) {
		ret = -ENOMEM;
		goto out;
	}

	ret = ufshcd_query_descriptor_retry(hba, UPIU_QUERY_OPCODE_READ_DESC,
					    QUERY_DESC_IDN_CONFIGURATION, 0, 0, desc_buf,
					    &hba->desc_size.conf_desc);
	if (ret) {
		dev_err(hba->dev, "%s:FAILed read descriptor%d\n", __func__, ret);

		return ret;
	}

	desc_buf[CONFIG_DESC_HEADER_PARAM_BOOT_EN] = 0x0;
	for (int i = 0; i < 8; i++) {
		if (i < 3)
			desc_buf[hba->desc_size.conf_head_desc + hba->desc_size.conf_unit_desc * i +
				 CONFIG_DESC_UNIT_PARAM_LU_EN] = 0x1;
		else
			desc_buf[hba->desc_size.conf_head_desc + hba->desc_size.conf_unit_desc * i +
				 CONFIG_DESC_UNIT_PARAM_LU_EN] = 0x0;
		desc_buf[hba->desc_size.conf_head_desc + hba->desc_size.conf_unit_desc * i +
			 CONFIG_DESC_UNIT_PARAM_BOOT_LU_ID] = 0x0;
		desc_buf[hba->desc_size.conf_head_desc + hba->desc_size.conf_unit_desc * i +
			 CONFIG_DESC_UNIT_PARAM_LU_WRI_PRO] = 0x0;
		desc_buf[hba->desc_size.conf_head_desc + hba->desc_size.conf_unit_desc * i +
			 CONFIG_DESC_UNIT_PARAM_MEM_TYPE] = 0x0;
		memset(&desc_buf[hba->desc_size.conf_head_desc + hba->desc_size.conf_unit_desc * i +
				 CONFIG_DESC_UNIT_PARAM_NUM_ALLOC_UNIT],
		       0x0, 4);
		desc_buf[hba->desc_size.conf_head_desc + hba->desc_size.conf_unit_desc * i +
			 CONFIG_DESC_UNIT_PARAM_DATA_RELY] = 0x1;
		desc_buf[hba->desc_size.conf_head_desc + hba->desc_size.conf_unit_desc * i +
			 CONFIG_DESC_UNIT_PARAM_LOGIC_BLK_SIZE] = 0xC;
		desc_buf[hba->desc_size.conf_head_desc + hba->desc_size.conf_unit_desc * i +
			 CONFIG_DESC_UNIT_PARAM_PROVIS_TYPE] = 0x3;
	}

	/*
	 * 3.set lun0:32M, lu1:32M, lu2:total_cap-64M
	 */
	alloc_unit_bytes = dSegmentSize * bAllocationUnitSize * UFS_LOGICAL_BLOCK_SIZE;
	boot_lun_size =
		((UFS_BOOT_LU_SIZE * 1024 * 1024) / alloc_unit_bytes);
	user_lun_size = (((qTotalRawDeviceCapacity * UFS_LOGICAL_BLOCK_SIZE) -
			 (UFS_BOOT_LU_SIZE * 2 * 1024 * 1024)) /
			alloc_unit_bytes);

	put_unaligned_be32(
		boot_lun_size,
		&desc_buf[hba->desc_size.conf_head_desc + hba->desc_size.conf_unit_desc * 0 +
			  CONFIG_DESC_UNIT_PARAM_NUM_ALLOC_UNIT]);
	put_unaligned_be32(
		boot_lun_size,
		&desc_buf[hba->desc_size.conf_head_desc + hba->desc_size.conf_unit_desc * 1 +
			  CONFIG_DESC_UNIT_PARAM_NUM_ALLOC_UNIT]);
	put_unaligned_be32(
		user_lun_size,
		&desc_buf[hba->desc_size.conf_head_desc + hba->desc_size.conf_unit_desc * 2 +
			  CONFIG_DESC_UNIT_PARAM_NUM_ALLOC_UNIT]);

	pr_info("ufs: boot_lun_size:0x%x, user_lun_size:0x%x\n", boot_lun_size, user_lun_size);

	if (ret) {
		dev_err(hba->dev, "%s:!!!FAILed write descriptor%d\n", __func__, ret);

		return ret;
	}
	pr_info("ufs: ufs_config_lun done\n");

out:
	kfree(desc_buf);
	return ret;
}

static int spacemit_k3_ufs_mphy_init(struct ufs_hba *hba)
{
	struct udevice *dev = hba->dev;
	struct spacemit_k3_ufs_priv *priv = dev_get_priv(dev);
	u32 reg_val;

	/* reset all mphy logical */
	ufshcd_writel(hba, 0x003, priv->phy_mng_base + UFS_MPHY_RST_CTRL);
	mdelay(1);

	/* power up all */
	ufshcd_writel(hba, 0x07f, priv->phy_mng_base + UFS_MPHY_PU_CTRL);
	mdelay(1);

	/* asserted ana_rx_hb8_reset */
	ufshcd_writel(hba, 0x37f, priv->phy_mng_base + UFS_MPHY_PU_CTRL);
	mdelay(1);

	/* deasserted ana_rx_hb8_reset */
	ufshcd_writel(hba, 0x07f, priv->phy_mng_base + UFS_MPHY_PU_CTRL);
	mdelay(1);

	/* deasserted ufs device reset & refer clk output enable */
	ufshcd_writel(hba, 0x101, priv->phy_mng_base + UFS_DEVICE_IO_CTRL);
	mdelay(1);

	/* note: refer clk 26MHz */

	/* wait PLL_lock here, bit31 at 0x0104 */
	{
		u32 timeout = 100000;
		do {
			reg_val = ufshcd_readl(hba, priv->phy_mng_base + UFS_MPHY_PU_CTRL);
			pr_debug("ufs: UFS_MPHY_PU_CTRL:0x%x\n", reg_val);
			if (reg_val & UFS_MPHY_PU_PLL_LOCK)
				break;
			udelay(10);
		} while (--timeout);

		if (!(reg_val & UFS_MPHY_PU_PLL_LOCK))
			pr_err("ufs: MPHY PLL lock timeout in mphy_init, UFS_MPHY_PU_CTRL=0x%x\n",
			       reg_val);
		else
			pr_debug("ufs: MPHY Pll was locked\n");
	}

	/* force cdr_pi_on, always enable rx_pck20 */
	ufshcd_writel(hba, 0x1, priv->phy_mng_base + 0x08);
	udelay(20);

	ufshcd_writel(hba, 0x40, priv->atop_base + (0xC2 << 2));
	udelay(20);

	ufshcd_writel(hba, 0x0, priv->phy_mng_base + 0x08);
	udelay(20);

	/* HYNIX1 phone: extra settle time after MPHY tuning */
	mdelay(5);

	pr_debug("ufs: ufs_spacemit_k3_mphy_init done\n");

	return 0;
}

static void spacemit_k3_ufs_phy_shutdown(struct ufs_hba *hba, struct spacemit_k3_ufs_priv *priv)
{
	ufshcd_writel(hba, 0x000, priv->phy_mng_base + UFS_DEVICE_IO_CTRL);
	udelay(20);

	ufshcd_writel(hba, 0x000, priv->phy_mng_base + UFS_MPHY_RST_CTRL);
	udelay(20);

	ufshcd_writel(hba, 0x000, priv->phy_mng_base + UFS_MPHY_PU_CTRL);
	udelay(20);
}

static int spacemit_k3_ufs_unipro_init(struct ufs_hba *hba)
{
	int err;
	u32 real_sysclk, reg_val;

	/* PA_TXHSG1SYNCLENGTH */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TXHSG1SYNCLENGTH), 0x4f);
	if (err) {
		pr_err("Writing PA_TXHSG1SYNCLENGTH error \n");
	}
	/* PA_TXHSG1PREPARELENGTH */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TXHSG1PREPARELENGTH), 0xf);
	if (err) {
		pr_err("Writing PA_TXHSG1PREPARELENGTH error \n");
	}

	/* PA_TXHSG2SYNCLENGTH */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TXHSG2SYNCLENGTH), 0x4f);
	if (err) {
		pr_err("Writing PA_TXHSG2SYNCLENGTH error \n");
	}
	/* PA_TXHSG2PREPARELENGTH */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TXHSG2PREPARELENGTH), 0xf);
	if (err) {
		pr_err("Writing PA_TXHSG2PREPARELENGTH error \n");
	}

	/* PA_TXHSG3SYNCLENGTH */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TXHSG3SYNCLENGTH), 0x4f);
	if (err) {
		pr_err("Writing PA_TXHSG3SYNCLENGTH error \n");
	}
	/* PA_TXHSG3PREPARELENGTH */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TXHSG3PREPARELENGTH), 0xf);
	if (err) {
		pr_err("Writing PA_TXHSG3PREPARELENGTH error \n");
	}

	/* PA_TXMK2EXTENSION */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TXMK2EXTENSION), 0x0);
	if (err) {
		pr_err("Writing PA_TXMK2EXTENSION error \n");
	}

	/* PA_PEERSCRAMBLING */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_PEERSCRAMBLING), 0x1);
	if (err) {
		pr_err("Writing PA_PEERSCRAMBLING error \n");
	}

	/* PA_TXSKIP */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TXSKIP), 0x1);
	if (err) {
		pr_err("Writing PA_TXSKIP error \n");
	}
	/* PA_TXSKIPPERIOD */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TXSKIPPERIOD), 250);
	if (err) {
		pr_err("Writing PA_TXSKIPPERIOD error \n");
	}

	/* PA_LOCAL_TX_LCC_ENABLE */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_LOCAL_TX_LCC_ENABLE), 0x0);
	if (err) {
		pr_err("Writing PA_LOCAL_TX_LCC_ENABLE error \n");
	}
	/* PA_PEER_TX_LCC_ENABLE */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_PEER_TX_LCC_ENABLE), 0x0);
	if (err) {
		pr_err("Writing PA_PEER_TX_LCC_ENABLE error \n");
	}

	/* PA_SCRAMBLING */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_SCRAMBLING), 0x1);
	if (err) {
		pr_err("Writing PA_SCRAMBLING error \n");
	}

	/* PA_GRANULARITY */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_GRANULARITY), 0x1);
	if (err) {
		pr_err("Writing PA_GRANULARITY error \n");
	}

	/* PA_MK2EXTENSIONGUARDBAND */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_MK2EXTENSIONGUARDBAND), 0x0);
	if (err) {
		pr_err("Writing PA_MK2EXTENSIONGUARDBAND error \n");
	}

	/* PA_TACTIVATE */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TACTIVATE), 0x64);
	if (err) {
		pr_err("Writing PA_TACTIVATE error \n");
	}
	/* PA_TXTRAILINGCLOCKS */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TXTRAILINGCLOCKS), 0x64);
	if (err) {
		pr_err("Writing PA_TXTRAILINGCLOCKS error \n");
	}

	{
		/* PA_STALLNOCONFIGTIME */
		err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_STALLNOCONFIGTIME), 15);
		if (err) {
			pr_err("Writing PA_STALLNOCONFIGTIME error \n");
		}

		/* RX_LS_PREPARELEN_TIME RX0 */
		err = ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(RX_LS_PRE_LEN_CAP, 4), 0x0B);
		if (err) {
			pr_err("Writing RX_LS_PREPARELEN_TIME RX0 error \n");
		}

		/* RX_LS_PREPARELEN_TIME RX1 */
		err = ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(RX_LS_PRE_LEN_CAP, 5), 0X0B);
		if (err) {
			pr_err("Writing RX_LS_PREPARELEN_TIME RX1 error \n");
		}

		/* RX_HIBERNATE_BKEN RX0 */
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_LANE_HB8_BKDOOR_ATTR, UIC_ARG_MPHY_RX_GEN_SEL_INDEX(0)),
			0x9F);
		if (err) {
			pr_err("Writing RX_HIBERNATE_BKEN RX0 error \n");
		}

		/* RX_HIBERNATE_BKEN RX1 */
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_LANE_HB8_BKDOOR_ATTR, UIC_ARG_MPHY_RX_GEN_SEL_INDEX(1)),
			0x9F);
		if (err) {
			pr_err("Writing RX_HIBERNATE_BKEN RX1 error \n");
		}

		/* PWM_BURST_closure_length */
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_PWRM_CLOSURE_LEN_CAP, UIC_ARG_MPHY_RX_GEN_SEL_INDEX(0)),
			15);
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_PWRM_CLOSURE_LEN_CAP, UIC_ARG_MPHY_RX_GEN_SEL_INDEX(1)),
			15);

		/* min_stall_not_config_time*/
		err = ufshcd_dme_set(
			hba, UIC_ARG_MIB_SEL(RX_MIN_STALL_CAP, UIC_ARG_MPHY_RX_GEN_SEL_INDEX(0)),
			0xFF);
		err = ufshcd_dme_set(
			hba, UIC_ARG_MIB_SEL(RX_MIN_STALL_CAP, UIC_ARG_MPHY_RX_GEN_SEL_INDEX(1)),
			0xFF);

		/* TX HB8_TIME CAP */
		err = ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(TX_HIBERN8TIME_CAPABILITY,
								UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)),
						0x64);
		err = ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(TX_HIBERN8TIME_CAPABILITY,
								UIC_ARG_MPHY_TX_GEN_SEL_INDEX(1)),
						0x64);

		/*RX HB8_TIME CAP*/
		err = ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(RX_HIBERN8TIME_CAPABILITY,
								UIC_ARG_MPHY_RX_GEN_SEL_INDEX(0)),
						0x64);
		err = ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(RX_HIBERN8TIME_CAPABILITY,
								UIC_ARG_MPHY_RX_GEN_SEL_INDEX(1)),
						0x64);

		/*TX EQ 3DB*/
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(ANA_EQ_CTRL_REG_ATTR, UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)),
			0x5);

		/*RX garbage cnt = 32 SI*/
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_LANE_SOF_BKDOOR_ATT, UIC_ARG_MPHY_RX_GEN_SEL_INDEX(0)),
			0x9F);
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_LANE_SOF_BKDOOR_ATT, UIC_ARG_MPHY_RX_GEN_SEL_INDEX(1)),
			0x9F);
	}

	pr_debug("ufs: ufs_spacemit_k3_uniprov1p6_init done.\n");

	/* program controller timing registers for 409MHz SYS1CLK */
	real_sysclk = UFS_SYS1CLK_1US_409MHZ;
	ufshcd_writel(hba, real_sysclk, UFS_SYS1CLK_1US_REG);

	reg_val = UFS_TX_SYMBOL_CLK_NS_US_409MHZ;
	ufshcd_writel(hba, reg_val, UFS_TX_SYMBOL_CLK_NS_US_REG);

	reg_val = UFS_PA_LINK_STARTUP_TIMER_MAX;
	ufshcd_writel(hba, reg_val, UFS_PA_LINK_STARTUP_TIMER_REG);

	/* HYNIX1 phone need delay*/
	mdelay(5);

	pr_debug("ufs: UFS_PA_LINK_STARTUP_TIMER_REG(0xd8) val: 0x%x\n",
		 ufshcd_readl(hba, UFS_PA_LINK_STARTUP_TIMER_REG));
	pr_debug("ufs: REG_UFS_SYS1CLK_1US(0xc0) val: 0x%x\n",
		 ufshcd_readl(hba, UFS_SYS1CLK_1US_REG));
	pr_debug("ufs: REG_UFS_TX_SYMBOL_CLK_NS_US(0xc4) val: 0x%x\n",
		 ufshcd_readl(hba, UFS_TX_SYMBOL_CLK_NS_US_REG));

	return 0;
}

static int spacemit_k3_ufs_silent_reset(struct ufs_hba *hba)
{
	struct udevice *dev = hba->dev;
	struct spacemit_k3_ufs_priv *priv = dev_get_priv(dev);

	/* stop device ref_clk & asserted ufs device reset */
	spacemit_k3_ufs_phy_shutdown(hba, priv);
	return 0;
}

static int spacemit_k3_ufs_link_startup_notify(struct ufs_hba *hba,
					       enum ufs_notify_change_status status)
{
	struct udevice *dev = hba->dev;
	struct spacemit_k3_ufs_priv *priv = dev_get_priv(dev);

	uint32_t reg_val;

	pr_debug("ufs: spacemit_k3_ufs_link_startup_notify, status:%d\n", status);
	if (status == PRE_CHANGE) {
		/*do nothing*/
		return 0;
	}

	if (status == POST_CHANGE) {
		/*check status after send dme link_up*/
		reg_val = ufshcd_readl(hba, REG_CONTROLLER_STATUS);
		if (!(reg_val & DEVICE_PRESENT)) {
			pr_err("ufs: DEVICE_PRESENT !!!FAIL! read REG_CONTROLLER_STATUS:0x%x\n",
			       reg_val);
			return -ENODEV;
		}

		reg_val = ufshcd_readl(hba, REG_INTERRUPT_STATUS);
		pr_debug("ufs: REG_INTERRUPT_STATUS before clear (0x%x):0x%x\n",
			 REG_INTERRUPT_STATUS, reg_val);

		if (reg_val & UIC_LINK_STARTUP)
			ufshcd_writel(hba, UIC_LINK_STARTUP, REG_INTERRUPT_STATUS);
		if (reg_val & UIC_ERROR)
			ufshcd_writel(hba, UIC_ERROR, REG_INTERRUPT_STATUS);

		reg_val = ufshcd_readl(hba, REG_INTERRUPT_STATUS);
		pr_debug("ufs: REG_INTERRUPT_STATUS after clear (0x%x):0x%x\n",
			 REG_INTERRUPT_STATUS, reg_val);

		reg_val = ufshcd_readl(hba, REG_UIC_ERROR_CODE_PHY_ADAPTER_LAYER);
		pr_debug("ufs: REG_UIC_ERROR_CODE_PHY_ADAPTER_LAYER(0x%x):0x%x\n",
			 REG_UIC_ERROR_CODE_PHY_ADAPTER_LAYER, reg_val);

		/* add 0xe8 make UFS2.1 run GEAR3+2Lane@409M*/
		mdelay(5);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xe8, UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)), 0x97);
		mdelay(1);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xe8, UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)), 0xd7);
		mdelay(1);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xe8, UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)), 0x17);

		/*LCC_DISABLE*/
		mdelay(5);
		ufshcd_dme_set(hba,
			       UIC_ARG_MIB_SEL(TX_LCC_ENABLE, UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)), 0);
		mdelay(1);
		ufshcd_dme_set(hba,
			       UIC_ARG_MIB_SEL(TX_LCC_ENABLE, UIC_ARG_MPHY_TX_GEN_SEL_INDEX(1)), 0);

		/*TX_Min_ActivateTime*/
		mdelay(1);
		ufshcd_dme_set(
			hba, UIC_ARG_MIB_SEL(TX_MIN_ACTIVATETIME, UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)),
			0x0);
		mdelay(1);
		ufshcd_dme_set(
			hba, UIC_ARG_MIB_SEL(TX_MIN_ACTIVATETIME, UIC_ARG_MPHY_TX_GEN_SEL_INDEX(1)),
			0x0);
		mdelay(10);

		/* use backdoor reg to pre-set TX RATE/GEAR to let PLL lock before set_power_mode
		 * switch */
		ufshcd_dme_set(hba, UIC_ARG_MIB(ANA_HSGEAR_CTRL_ATTR), 0x25);
		mdelay(10);

		/* wait PLL_lock here, bit31 at UFS_MPHY_PU_CTRL */
		{
			u32 timeout = 100000;
			do {
				reg_val = ufshcd_readl(hba, priv->phy_mng_base + UFS_MPHY_PU_CTRL);
				if (reg_val & UFS_MPHY_PU_PLL_LOCK)
					break;
				udelay(10);
			} while (--timeout);

			if (!(reg_val & UFS_MPHY_PU_PLL_LOCK))
				pr_err("ufs: MPHY PLL lock timeout, UFS_MPHY_PU_CTRL=0x%x\n",
				       reg_val);
		}
	}

	return 0;
}

static int spacemit_k3_ufs_hce_enable_notify(struct ufs_hba *hba,
					     enum ufs_notify_change_status status)
{
	pr_debug("ufs: spacemit_k3_ufs_hce_enable_notify, status:%d\n", status);

	if (status == PRE_CHANGE) {
		/*do nothing*/
	}

	if (status == POST_CHANGE) {
		spacemit_k3_ufs_mphy_init(hba);
		spacemit_k3_ufs_unipro_init(hba);
	}

	return 0;
}

static int spacemit_k3_ufs_init(struct ufs_hba *hba)
{
	struct udevice *dev = hba->dev;
	struct spacemit_k3_ufs_priv *priv;
	priv = dev_get_priv(dev);

	spacemit_k3_ufs_clk_enable(priv);

	return 0;
}

static const struct ufs_hba_ops spacemit_k3_ufs_vops = {
	.init = spacemit_k3_ufs_init,
	.hce_enable_notify = spacemit_k3_ufs_hce_enable_notify,
	.link_startup_notify = spacemit_k3_ufs_link_startup_notify,
	.device_reset = spacemit_k3_ufs_silent_reset,
};

static int spacemit_k3_ufs_pltfm_bind(struct udevice *dev)
{
	struct udevice *scsi_dev;
	pr_info("ufs: call spacemit_k3_ufs_pltfm_bind\n");
	return ufs_scsi_bind(dev, &scsi_dev);
}

static int spacemit_k3_ufs_pltfm_probe(struct udevice *dev)
{
	struct spacemit_k3_ufs_priv *priv = dev_get_priv(dev);
	struct ufs_hba *hba = dev_get_uclass_priv(dev);
	struct ufs_hba_ops *hba_ops = (struct ufs_hba_ops *) dev->driver_data;
	int ret;
	int retries;

	for (retries = 3; retries > 0; retries--) {
		ret = ufshcd_probe(dev, hba_ops);
		if (!ret) {
			break;
		}
		hba->ops->device_reset(hba);
	}

	if (ret) {
		spacemit_k3_ufs_phy_shutdown(hba, priv);
		spacemit_k3_ufs_clk_disable(priv);
		pr_err("ufs host probe failed:%d\n", ret);
	} else {
		pr_info("ufs: ufs host probed.\n");
	}

	return ret;
}

static int spacemit_k3_ufs_of_to_plat(struct udevice *dev)
{
	const char *compat;
	int compat_length;
	int ret;

	struct spacemit_k3_ufs_priv *priv = dev_get_priv(dev);

	compat = ofnode_get_property(dev->node_, "compatible", &compat_length);
	if (!compat) {
		return -1;
	}

	if (!strcmp(compat, "spacemit,k3-ufshci")) {
		priv->phy_mng_base = UFS_ARASAN_PHY_MNG_BASE;
		priv->atop_base = UFS_ARASAN_TOP_BASE;
	}

	ret = clk_get_by_index(dev, 0, &priv->aclk);
	if (ret) {
		dev_err(dev, "ufs: failed to get aclk, ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static int spacemit_k3_ufs_pltfm_remove(struct udevice *dev)
{
	struct spacemit_k3_ufs_priv *priv = dev_get_priv(dev);

	pr_info("ufs: spacemit_k3_ufs_pltfm_remove\n");
	spacemit_k3_ufs_clk_disable(priv);

	return 0;
}

static const struct udevice_id spacemit_k3_ufs_pltfm_ids[] = {
	{
		.compatible = "spacemit,k3-ufshci",
		.data = (ulong) &spacemit_k3_ufs_vops,
	},
	{ /* sentinel */ }
};

U_BOOT_DRIVER(spacemit_k3_ufs) = {
	.name = "ufs-spacemit_k3",
	.id = UCLASS_UFS,
	.of_match = spacemit_k3_ufs_pltfm_ids,
	.of_to_plat = spacemit_k3_ufs_of_to_plat,
	.bind = spacemit_k3_ufs_pltfm_bind,
	.probe = spacemit_k3_ufs_pltfm_probe,
	.remove = spacemit_k3_ufs_pltfm_remove,
	.priv_auto = sizeof(struct spacemit_k3_ufs_priv),
};