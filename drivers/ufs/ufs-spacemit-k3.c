// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
 * spacemit_k3 UFS host controller driver
 *
 * Copyright (C) 2025 Spacemit Technology Co., Ltd.
 */

#include <clk.h>
#include <dm.h>
#include <reset.h>
#include <scsi.h>
#include <ufs.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <dm/device-internal.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/bug.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <asm/unaligned.h>
#include <configs/k3.h>
#include "ufs.h"

struct spacemit_k3_ufs_priv {
	struct clk aclk;
	struct reset_ctl reset;
	u32 phy_mng_base;
	u32 atop_base;
	u32 ref_clk_freq;
	u32 clock_freq_hz;
	ulong aclk_rate_hz; 
};
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

#define UFSHCD_HCE_UIC_PWR_MASK                                      \
	(UIC_HIBERNATE_ENTER | UIC_HIBERNATE_EXIT | UIC_POWER_MODE | \
	 UIC_COMMAND_COMPL)

#define UFSHCD_LINK_ALL_MASK                                            \
	(UFSHCD_HCE_UIC_PWR_MASK | UTP_TRANSFER_REQ_COMPL | UIC_ERROR | \
	 DEVICE_FATAL_ERROR | CONTROLLER_FATAL_ERROR | SYSTEM_BUS_FATAL_ERROR)

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
#define UFS_DL_AFC0REQTIMEOUTVAL_MAX 0xFFFF

extern int ufshcd_query_descriptor_retry(struct ufs_hba *hba,
					 enum query_opcode opcode,
					 enum desc_idn idn, u8 index,
					 u8 selector, u8 *desc_buf,
					 int *buf_len);

/**
 * spacemit_k3_ufs_set_ref_clk - Set UFS device reference clock frequency
 * @hba: UFS host controller handle
 *
 * Read the expected reference clock frequency from DTS and configure
 * the UFS device's bRefClkFreq attribute if it differs from current value.
 *
 * Returns: 0 on success, -EAGAIN if reinit required, negative error code on failure
 */
static int spacemit_k3_ufs_set_ref_clk(struct ufs_hba *hba)
{
	struct udevice *dev = hba->dev;
	struct spacemit_k3_ufs_priv *priv = dev_get_priv(dev);
	u32 ref_clk = priv->ref_clk_freq;
	u32 cur_clk = 0;
	int err;
	bool updated = false;

	err = ufshcd_query_attr_retry(hba, UPIU_QUERY_OPCODE_READ_ATTR,
				      QUERY_ATTR_IDN_REF_CLK_FREQ, 0, 0,
				      &cur_clk);
	if (err) {
		dev_warn(hba->dev, "Failed to read bRefClkFreq, err = %d\n",
			 err);
		return err;
	}

	pr_info("ufs: bRefClkFreq current=%u expected=%u\n", cur_clk, ref_clk);

	if (cur_clk != ref_clk) {
		err = ufshcd_query_attr_retry(hba, UPIU_QUERY_OPCODE_WRITE_ATTR,
					      QUERY_ATTR_IDN_REF_CLK_FREQ, 0, 0,
					      &ref_clk);
		if (err) {
			dev_warn(hba->dev,
				 "Failed to set bRefClkFreq to %u, err = %d\n",
				 ref_clk, err);
			return err;
		}
		updated = true;
	}

	if (updated) {
		pr_info("ufs: bRefClkFreq updated, reinit required\n");
		return -EAGAIN;
	}

	return 0;
}

static int spacemit_k3_ufs_parse_ref_clk_freq(u32 raw, u32 *ref_clk_freq)
{
	/* DTS must provide one of the UFS-spec reference clock frequencies in Hz. */
	switch (raw) {
	case 19200000:
		*ref_clk_freq = UFS_REF_CLK_FREQ_19_2_MHZ;
		return 0;
	case 26000000:
		*ref_clk_freq = UFS_REF_CLK_FREQ_26_MHZ;
		return 0;
	case 38400000:
		*ref_clk_freq = UFS_REF_CLK_FREQ_38_4_MHZ;
		return 0;
	case 52000000:
		*ref_clk_freq = UFS_REF_CLK_FREQ_52_MHZ;
		return 0;
	default:
		return -EINVAL;
	}
}

/**
 * spacemit_k3_ufs_set_power_mode - Configure UFS power mode
 * @hba: UFS host controller handle
 *
 * Configure the UFS link to the maximum supported power mode.
 * Uses ufshcd_get_max_pwr_mode() to get the negotiated parameters,
 * then switches to Rate B for better compatibility.
 *
 * Returns: 0 on success, negative error code on failure
 */
static int spacemit_k3_ufs_set_power_mode(struct ufs_hba *hba)
{
	int ret;

	ret = ufshcd_get_max_pwr_mode(hba);
	if (ret) {
		dev_err(hba->dev,
			"%s: Failed getting max supported power mode\n",
			__func__);
		return ret;
	}

	/*
	 * Switch to Rate B for better compatibility with K3 platform.
	 * ufshcd_get_max_pwr_mode() sets Rate A by default.
	 */
	hba->max_pwr_info.info.hs_rate = PA_HS_MODE_B;

	ret = ufshcd_change_power_mode(hba, &hba->max_pwr_info.info);
	if (ret) {
		dev_err(hba->dev, "%s: Failed setting power mode, err = %d\n",
			__func__, ret);
		return ret;
	}

	ufshcd_print_pwr_info(hba);
	return 0;
}

static int spacemit_k3_ufs_init_aclk(struct udevice *dev)
{
	struct spacemit_k3_ufs_priv *priv = dev_get_priv(dev);
	ulong rate_hz = 0;

	if (priv->clock_freq_hz) {
		rate_hz = clk_set_rate(&priv->aclk, priv->clock_freq_hz);
		if (IS_ERR_VALUE(rate_hz)) {
			dev_warn(dev,
				 "ufs: failed to set aclk to %uHz: %ld, using current rate\n",
				 priv->clock_freq_hz, (long)rate_hz);
			rate_hz = 0;
		}
	}

	if (!rate_hz)
		rate_hz = clk_get_rate(&priv->aclk);

	if (IS_ERR_VALUE(rate_hz) || !rate_hz) {
		long err = IS_ERR_VALUE(rate_hz) ? (long)rate_hz : -EINVAL;

		dev_err(dev, "ufs: failed to get aclk rate: %ld\n", err);
		return err;
	}

	priv->aclk_rate_hz = rate_hz;
	return 0;
}

static int spacemit_k3_ufs_get_aclk_rate(struct ufs_hba *hba, ulong *rate_hz)
{
	struct spacemit_k3_ufs_priv *priv = dev_get_priv(hba->dev);
	ulong rate;

	rate = clk_get_rate(&priv->aclk);
	if (IS_ERR_VALUE(rate) || !rate)
		rate = priv->aclk_rate_hz;

	if (!rate) {
		dev_err(hba->dev, "ufs: invalid aclk rate\n");
		return -EINVAL;
	}

	priv->aclk_rate_hz = rate;
	*rate_hz = rate;
	return 0;
}

static int spacemit_k3_ufs_get_sys1clk_1us(struct ufs_hba *hba,
					   u32 *sys1clk_1us)
{
	ulong rate_hz;
	u32 cycles_per_us;
	int ret;

	ret = spacemit_k3_ufs_get_aclk_rate(hba, &rate_hz);
	if (ret)
		return ret;

	/*
	 * Bias upward so controller timing derived from SYS1CLK is never
	 * programmed shorter than the real ACLK period.
	 */
	cycles_per_us = DIV_ROUND_UP(rate_hz, 1000000UL);
	if (!cycles_per_us)
		return -ERANGE;

	*sys1clk_1us = cycles_per_us;
	return 0;
}

static int spacemit_k3_ufs_clk_enable(struct udevice *dev)
{
	struct spacemit_k3_ufs_priv *priv = dev_get_priv(dev);
	int ret;

	/* First deassert reset */
	ret = reset_deassert(&priv->reset);
	if (ret) {
		dev_err(dev, "ufs: fail to deassert reset, ret=%d\n", ret);
		return ret;
	}

	ret = spacemit_k3_ufs_init_aclk(dev);
	if (ret)
		goto err_assert_reset;

	/* Then enable clock */
	ret = clk_enable(&priv->aclk);
	if (ret) {
		dev_err(dev, "ufs: fail to enable ufs aclk, ret=%d\n", ret);
		goto err_assert_reset;
	}

	/* HYNIX1 phone need delay */
	mdelay(5);
	return 0;

err_assert_reset:
	reset_assert(&priv->reset);
	return ret;
}

static void spacemit_k3_ufs_clk_disable(struct spacemit_k3_ufs_priv *priv)
{
	/* Disable clock first */
	clk_disable(&priv->aclk);

	/* Then assert reset */
	reset_assert(&priv->reset);
}

static int __maybe_unused debug_print_desc(struct udevice *dev,
					   enum desc_idn idn)
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
		ret = ufshcd_query_descriptor_retry(hba,
						    UPIU_QUERY_OPCODE_READ_DESC,
						    idn, 0, 0, desc_buf,
						    &desc_size);
		if (ret) {
			dev_err(hba->dev, "%s:FAILed read descriptor%d\n",
				__func__, ret);
			return ret;
		}

		debug("ufs: debug print descriptor for idn %d\n", idn);

		for (int i = 0; i < hba->desc_size.conf_desc; i++) {
			debug("[%x]:%x  ", i, desc_buf[i]);
			if ((i + 1) % 8 == 0) {
				debug("\n");
			}
		}
	} else {
		debug("ufs: debug print descriptor for idn %d\n", idn);
		for (int i = 0; i < 8; i++) {
			ret = ufshcd_query_descriptor_retry(
				hba, UPIU_QUERY_OPCODE_READ_DESC, idn, i, 0,
				desc_buf, &desc_size);
			if (ret) {
				dev_err(hba->dev,
					"%s:FAILed read descriptor%d\n",
					__func__, ret);

				return ret;
			}

			debug("ufs: unit %d descriptor\n", i);
			for (int i = 0; i < hba->desc_size.conf_desc; i++) {
				debug("[%x]:%x  ", i, desc_buf[i]);
				if ((i + 1) % 8 == 0) {
					debug("\n");
				}
			}
		}
	}
out:
	kfree(desc_buf);
	return ret;
}

#define SPACEMIT_UFS_CONFIG_LUN_SLOTS 8
#define SPACEMIT_UFS_UNIT_DESC_PARAM_LU_ENABLE 0x03

static __maybe_unused int
spacemit_k3_ufs_get_conf_desc_layout(struct ufs_hba *hba, int *head_desc_size,
				     int *unit_desc_size)
{
	if (hba->desc_size.conf_head_desc > 0 &&
	    hba->desc_size.conf_unit_desc > 0 &&
	    hba->desc_size.conf_head_desc +
			    hba->desc_size.conf_unit_desc *
				    SPACEMIT_UFS_CONFIG_LUN_SLOTS <=
		    hba->desc_size.conf_desc) {
		*head_desc_size = hba->desc_size.conf_head_desc;
		*unit_desc_size = hba->desc_size.conf_unit_desc;
		return 0;
	}

	/* Try known descriptor layouts first. */
	if (QUERY_DESC_CONFIGURATION_DEF_SIZE_NEW_HEAD +
		    QUERY_DESC_CONFIGURATION_DEF_SIZE_NEW_UNIT *
			    SPACEMIT_UFS_CONFIG_LUN_SLOTS <=
	    hba->desc_size.conf_desc) {
		*head_desc_size = QUERY_DESC_CONFIGURATION_DEF_SIZE_NEW_HEAD;
		*unit_desc_size = QUERY_DESC_CONFIGURATION_DEF_SIZE_NEW_UNIT;
		return 0;
	}

	if (QUERY_DESC_CONFIGURATION_DEF_SIZE_HEAD +
		    QUERY_DESC_CONFIGURATION_DEF_SIZE_UNIT *
			    SPACEMIT_UFS_CONFIG_LUN_SLOTS <=
	    hba->desc_size.conf_desc) {
		*head_desc_size = QUERY_DESC_CONFIGURATION_DEF_SIZE_HEAD;
		*unit_desc_size = QUERY_DESC_CONFIGURATION_DEF_SIZE_UNIT;
		return 0;
	}

	return -EINVAL;
}

static bool spacemit_k3_ufs_is_conf_desc_layout_valid(struct ufs_hba *hba,
						      int head_desc_size,
						      int unit_desc_size)
{
	if (head_desc_size <= 0 || unit_desc_size <= 0)
		return false;

	return head_desc_size +
		       unit_desc_size * SPACEMIT_UFS_CONFIG_LUN_SLOTS <=
	       hba->desc_size.conf_desc;
}

static int spacemit_k3_ufs_read_unit_lu_state(struct ufs_hba *hba,
					      u8 *unit_lu_enabled,
					      int *enabled_lun_count)
{
	u8 desc_buf[QUERY_DESC_MAX_SIZE];
	int desc_size;
	bool any_read_ok = false;
	int i, ret;

	*enabled_lun_count = 0;
	memset(unit_lu_enabled, 0, SPACEMIT_UFS_CONFIG_LUN_SLOTS);

	for (i = 0; i < SPACEMIT_UFS_CONFIG_LUN_SLOTS; i++) {
		desc_size = hba->desc_size.unit_desc;
		if (desc_size <= SPACEMIT_UFS_UNIT_DESC_PARAM_LU_ENABLE ||
		    desc_size > QUERY_DESC_MAX_SIZE)
			desc_size = QUERY_DESC_UNIT_DEF_SIZE;

		ret = ufshcd_query_descriptor_retry(hba,
						    UPIU_QUERY_OPCODE_READ_DESC,
						    QUERY_DESC_IDN_UNIT, i, 0,
						    desc_buf, &desc_size);
		if (ret) {
			dev_dbg(hba->dev,
				"%s: read unit descriptor[%d] failed: %d\n",
				__func__, i, ret);
			continue;
		}

		any_read_ok = true;
		if (desc_size <= SPACEMIT_UFS_UNIT_DESC_PARAM_LU_ENABLE)
			continue;

		if (desc_buf[SPACEMIT_UFS_UNIT_DESC_PARAM_LU_ENABLE] == 0x1) {
			unit_lu_enabled[i] = 1;
			(*enabled_lun_count)++;
		}
	}

	return any_read_ok ? 0 : -EIO;
}

static int spacemit_k3_ufs_score_conf_layout(const u8 *desc_buf,
					     int head_desc_size,
					     int unit_desc_size,
					     const u8 *unit_lu_enabled)
{
	int i;
	int score = 0;

	for (i = 0; i < SPACEMIT_UFS_CONFIG_LUN_SLOTS; i++) {
		int offset = head_desc_size + unit_desc_size * i;
		bool conf_lu_enabled =
			desc_buf[offset + CONFIG_DESC_UNIT_PARAM_LU_EN] == 0x1;

		if (conf_lu_enabled == !!unit_lu_enabled[i])
			score++;
	}

	return score;
}

static int spacemit_k3_ufs_select_conf_desc_layout(struct ufs_hba *hba,
						   const u8 *desc_buf,
						   const u8 *unit_lu_enabled,
						   int *head_desc_size,
						   int *unit_desc_size)
{
	int candidate_head[3];
	int candidate_unit[3];
	int candidate_count = 0;
	int i;
	int best = -1;
	int best_score = -1;

	candidate_head[candidate_count] = hba->desc_size.conf_head_desc;
	candidate_unit[candidate_count++] = hba->desc_size.conf_unit_desc;

	if (QUERY_DESC_CONFIGURATION_DEF_SIZE_NEW_HEAD !=
		    hba->desc_size.conf_head_desc ||
	    QUERY_DESC_CONFIGURATION_DEF_SIZE_NEW_UNIT !=
		    hba->desc_size.conf_unit_desc) {
		candidate_head[candidate_count] =
			QUERY_DESC_CONFIGURATION_DEF_SIZE_NEW_HEAD;
		candidate_unit[candidate_count++] =
			QUERY_DESC_CONFIGURATION_DEF_SIZE_NEW_UNIT;
	}

	if (QUERY_DESC_CONFIGURATION_DEF_SIZE_HEAD !=
		    hba->desc_size.conf_head_desc ||
	    QUERY_DESC_CONFIGURATION_DEF_SIZE_UNIT !=
		    hba->desc_size.conf_unit_desc) {
		candidate_head[candidate_count] =
			QUERY_DESC_CONFIGURATION_DEF_SIZE_HEAD;
		candidate_unit[candidate_count++] =
			QUERY_DESC_CONFIGURATION_DEF_SIZE_UNIT;
	}

	for (i = 0; i < candidate_count; i++) {
		int score;

		if (!spacemit_k3_ufs_is_conf_desc_layout_valid(
			    hba, candidate_head[i], candidate_unit[i]))
			continue;

		score = spacemit_k3_ufs_score_conf_layout(desc_buf,
							  candidate_head[i],
							  candidate_unit[i],
							  unit_lu_enabled);
		if (score > best_score) {
			best = i;
			best_score = score;
		}
	}

	if (best < 0)
		return -EINVAL;

	*head_desc_size = candidate_head[best];
	*unit_desc_size = candidate_unit[best];

	debug("ufs: selected config layout head=0x%x unit=0x%x score=%d\n",
	      *head_desc_size, *unit_desc_size, best_score);

	return 0;
}

static int
spacemit_k3_ufs_get_total_alloc_units_from_geometry(struct ufs_hba *hba,
						    u64 *total_alloc_units)
{
	u8 *desc_buf;
	u64 qTotalRawDeviceCapacity;
	u32 dSegmentSize;
	u8 bAllocationUnitSize;
	u64 alloc_unit_bytes;
	int ret;

	desc_buf = kmalloc(hba->desc_size.geom_desc, GFP_KERNEL);
	if (!desc_buf)
		return -ENOMEM;

	ret = ufshcd_query_descriptor_retry(hba, UPIU_QUERY_OPCODE_READ_DESC,
					    QUERY_DESC_IDN_GEOMETRY, 0, 0,
					    desc_buf,
					    &hba->desc_size.geom_desc);
	if (ret)
		goto out;

	qTotalRawDeviceCapacity =
		get_unaligned_be64(&desc_buf[GEO_DESC_PARAM_TOTAL_RAW_DEV_CAP]);
	dSegmentSize = get_unaligned_be32(&desc_buf[GEO_DESC_PARAM_SEG_SIZE]);
	bAllocationUnitSize = desc_buf[GEO_DESC_PARAM_ALLOC_UNIT_SIZE];
	if (!dSegmentSize || !bAllocationUnitSize) {
		ret = -EINVAL;
		goto out;
	}

	alloc_unit_bytes = (u64)dSegmentSize * bAllocationUnitSize *
			   UFS_LOGICAL_BLOCK_SIZE;
	if (!alloc_unit_bytes) {
		ret = -EINVAL;
		goto out;
	}

	*total_alloc_units =
		(qTotalRawDeviceCapacity * UFS_LOGICAL_BLOCK_SIZE) /
		alloc_unit_bytes;
	if (!*total_alloc_units || *total_alloc_units > 0xFFFFFFFFULL)
		ret = -ERANGE;

out:
	kfree(desc_buf);
	return ret;
}

/*
 * Return values:
 *   0: already single-LUN and active
 *   1: descriptor changed or re-init still needed
 *  <0: error
 */
static __maybe_unused int
spacemit_k3_ufs_check_and_config_single_lun(struct udevice *dev)
{
	u8 *desc_buf;
	u8 unit_lu_enabled[SPACEMIT_UFS_CONFIG_LUN_SLOTS];
	u32 conf_desc_lock = 0;
	u64 total_alloc_units = 0;
	int ret;
	int unit_state_ret;
	int unit_enabled_lun_count = 0;
	int conf_enabled_lun_count = 0;
	int conf_head_desc;
	int conf_unit_desc;
	int i;
	bool need_reconfigure;
	struct ufs_hba *hba = dev_get_uclass_priv(dev);

	desc_buf = kmalloc(hba->desc_size.conf_desc, GFP_KERNEL);
	if (!desc_buf)
		return -ENOMEM;

	ret = ufshcd_query_descriptor_retry(hba, UPIU_QUERY_OPCODE_READ_DESC,
					    QUERY_DESC_IDN_CONFIGURATION, 0, 0,
					    desc_buf,
					    &hba->desc_size.conf_desc);
	if (ret) {
		dev_err(hba->dev, "%s: failed to read config descriptor: %d\n",
			__func__, ret);
		goto out;
	}

	unit_state_ret = spacemit_k3_ufs_read_unit_lu_state(
		hba, unit_lu_enabled, &unit_enabled_lun_count);
	if (unit_state_ret) {
		dev_dbg(hba->dev, "%s: unit descriptor state unavailable: %d\n",
			__func__, unit_state_ret);
		ret = spacemit_k3_ufs_get_conf_desc_layout(hba, &conf_head_desc,
							   &conf_unit_desc);
	} else {
		ret = spacemit_k3_ufs_select_conf_desc_layout(hba, desc_buf,
							      unit_lu_enabled,
							      &conf_head_desc,
							      &conf_unit_desc);
	}

	if (ret) {
		dev_err(hba->dev,
			"%s: unsupported config descriptor layout (len=0x%x)\n",
			__func__, hba->desc_size.conf_desc);
		goto out;
	}

	for (i = 0; i < SPACEMIT_UFS_CONFIG_LUN_SLOTS; i++) {
		int offset = conf_head_desc + conf_unit_desc * i;

		if (desc_buf[offset + CONFIG_DESC_UNIT_PARAM_LU_EN] == 0x1)
			conf_enabled_lun_count++;

		total_alloc_units += get_unaligned_be32(
			&desc_buf[offset +
				  CONFIG_DESC_UNIT_PARAM_NUM_ALLOC_UNIT]);
	}

	if (!unit_state_ret)
		need_reconfigure =
			(unit_enabled_lun_count != 1 || !unit_lu_enabled[0]);
	else
		need_reconfigure = (conf_enabled_lun_count != 1);

	debug("ufs: conf_lun_count=%d unit_lun_count=%d total_alloc_units=%llu need_recfg=%d\n",
	      conf_enabled_lun_count, unit_enabled_lun_count, total_alloc_units,
	      need_reconfigure);

	if (!need_reconfigure) {
		ret = 0;
		goto out;
	}

	if (!total_alloc_units || total_alloc_units > 0xFFFFFFFFULL) {
		ret = spacemit_k3_ufs_get_total_alloc_units_from_geometry(
			hba, &total_alloc_units);
		if (ret) {
			dev_err(hba->dev,
				"%s: failed to get total alloc units from geometry: %d\n",
				__func__, ret);
			goto out;
		}
	}

	ret = ufshcd_query_attr_retry(hba, UPIU_QUERY_OPCODE_READ_ATTR,
				      QUERY_ATTR_IDN_CONF_DESC_LOCK, 0, 0,
				      &conf_desc_lock);
	if (ret) {
		dev_warn(hba->dev,
			 "%s: failed to read bConfigDescrLock (%d), continue\n",
			 __func__, ret);
	} else if (conf_desc_lock) {
		dev_err(hba->dev,
			"%s: bConfigDescrLock is set, cannot reconfigure LUNs\n",
			__func__);
		ret = -EPERM;
		goto out;
	}

	/* Keep only LU0 enabled and move all allocated units into LU0. */
	desc_buf[CONFIG_DESC_HEADER_PARAM_BOOT_EN] = 0x0;
	for (i = 0; i < SPACEMIT_UFS_CONFIG_LUN_SLOTS; i++) {
		int offset = conf_head_desc + conf_unit_desc * i;

		if (i == 0) {
			desc_buf[offset + CONFIG_DESC_UNIT_PARAM_LU_EN] = 0x1;
			put_unaligned_be32(
				(u32)total_alloc_units,
				&desc_buf[offset +
					  CONFIG_DESC_UNIT_PARAM_NUM_ALLOC_UNIT]);
		} else {
			desc_buf[offset + CONFIG_DESC_UNIT_PARAM_LU_EN] = 0x0;
			put_unaligned_be32(
				0,
				&desc_buf[offset +
					  CONFIG_DESC_UNIT_PARAM_NUM_ALLOC_UNIT]);
		}

		desc_buf[offset + CONFIG_DESC_UNIT_PARAM_BOOT_LU_ID] = 0x0;
	}

	ret = ufshcd_query_descriptor_retry(hba, UPIU_QUERY_OPCODE_WRITE_DESC,
					    QUERY_DESC_IDN_CONFIGURATION, 0, 0,
					    desc_buf,
					    &hba->desc_size.conf_desc);
	if (ret) {
		dev_err(hba->dev, "%s: failed to write config descriptor: %d\n",
			__func__, ret);
		goto out;
	}

	dev_info(
		hba->dev,
		"single-LUN config descriptor written, UFS re-init is required\n");
	ret = 1;

out:
	kfree(desc_buf);
	return ret;
}

#if !defined(CONFIG_SPL_BUILD)
extern enum board_boot_mode get_boot_mode(void);

static bool spacemit_k3_ufs_should_enforce_single_lun(void)
{
	enum board_boot_mode boot_mode = get_boot_mode();

	/*
	 * Normal UFS boots are expected to run on already provisioned media.
	 * Skip the expensive descriptor walk in that path and keep the
	 * single-LUN enforcement for recovery / flashing flows.
	 */
	return boot_mode != BOOT_MODE_UFS;
}
#endif

static int spacemit_k3_ufs_wait_mphy_pll_lock(struct ufs_hba *hba,
					      const char *where)
{
	struct spacemit_k3_ufs_priv *priv = dev_get_priv(hba->dev);
	u32 reg_val = 0;
	int timeout = 10000;

	while (timeout-- > 0) {
		reg_val = ufshcd_readl(hba, priv->phy_mng_base + UFS_MPHY_PU_CTRL);
		if (reg_val & UFS_MPHY_PU_PLL_LOCK)
			return 0;
		udelay(1);
	}

	dev_err(hba->dev,
		"%s: M-PHY PLL lock timeout, UFS_MPHY_PU_CTRL=0x%08x\n",
		where, reg_val);
	return -ETIMEDOUT;
}

static int spacemit_k3_ufs_mphy_init(struct ufs_hba *hba)
{
	struct udevice *dev = hba->dev;
	struct spacemit_k3_ufs_priv *priv = dev_get_priv(dev);
	int ret;

	/* reset all mphy logical */
	ufshcd_writel(hba, 0x003, priv->phy_mng_base + UFS_MPHY_RST_CTRL);
	mdelay(1);

	/* power up all */
	ufshcd_writel(hba, 0x87f, priv->phy_mng_base + UFS_MPHY_PU_CTRL);
	mdelay(1);

	/* asserted ana_rx_hb8_reset */
	ufshcd_writel(hba, 0xb7f,
		      priv->phy_mng_base + UFS_MPHY_PU_CTRL);
	mdelay(1);

	/* deasserted ana_rx_hb8_reset */
	ufshcd_writel(hba, 0x87f, priv->phy_mng_base + UFS_MPHY_PU_CTRL);
	mdelay(1);

	/* deasserted ufs device reset & refer clk output enable */
	ufshcd_writel(hba, 0x101,
		      priv->phy_mng_base + UFS_DEVICE_IO_CTRL);
	mdelay(1);

	ret = spacemit_k3_ufs_wait_mphy_pll_lock(hba, __func__);
	if (ret)
		return ret;

	ufshcd_writel(hba, 0x1, priv->phy_mng_base + UFS_MPHY_BKDR_CTRL);
	udelay(20);

	ufshcd_writel(hba, 0x00, priv->atop_base + (ANA_HSGEAR_CTRL_ATTR << 2));
	ufshcd_writel(hba, 0x00, priv->atop_base + (0xC2 << 2));
	udelay(20);

	ufshcd_writel(hba, 0x0, priv->phy_mng_base + UFS_MPHY_BKDR_CTRL);
	udelay(20);

	/* HYNIX1 phone: extra settle time after MPHY tuning */
	mdelay(5);

	pr_debug("ufs: ufs_spacemit_k3_mphy_init done\n");

	return 0;
}

static void spacemit_k3_ufs_phy_shutdown(struct ufs_hba *hba,
					 struct spacemit_k3_ufs_priv *priv)
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
	u32 tx_symbol_clk_ns_us;

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
		err = ufshcd_dme_set(hba, UIC_ARG_MIB(PA_STALLNOCONFIGTIME),
				     15);
		if (err) {
			pr_err("Writing PA_STALLNOCONFIGTIME error \n");
		}

		/* RX_LS_PREPARELEN_TIME RX0 */
		err = ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(RX_LS_PRE_LEN_CAP, 4),
				     0x0B);
		if (err) {
			pr_err("Writing RX_LS_PREPARELEN_TIME RX0 error \n");
		}

		/* RX_LS_PREPARELEN_TIME RX1 */
		err = ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(RX_LS_PRE_LEN_CAP, 5),
				     0X0B);
		if (err) {
			pr_err("Writing RX_LS_PREPARELEN_TIME RX1 error \n");
		}

		/* RX_HIBERNATE_BKEN RX0 */
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_LANE_HB8_BKDOOR_ATTR,
					UIC_ARG_MPHY_RX_GEN_SEL_INDEX(0)),
			0x9F);
		if (err) {
			pr_err("Writing RX_HIBERNATE_BKEN RX0 error \n");
		}

		/* RX_HIBERNATE_BKEN RX1 */
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_LANE_HB8_BKDOOR_ATTR,
					UIC_ARG_MPHY_RX_GEN_SEL_INDEX(1)),
			0x9F);
		if (err) {
			pr_err("Writing RX_HIBERNATE_BKEN RX1 error \n");
		}

		/* PWM_BURST_closure_length */
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_PWRM_CLOSURE_LEN_CAP,
					UIC_ARG_MPHY_RX_GEN_SEL_INDEX(0)),
			15);
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_PWRM_CLOSURE_LEN_CAP,
					UIC_ARG_MPHY_RX_GEN_SEL_INDEX(1)),
			15);

		/* min_stall_not_config_time*/
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_MIN_STALL_CAP,
					UIC_ARG_MPHY_RX_GEN_SEL_INDEX(0)),
			0xFF);
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_MIN_STALL_CAP,
					UIC_ARG_MPHY_RX_GEN_SEL_INDEX(1)),
			0xFF);

		/* TX HB8_TIME CAP */
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(TX_HIBERN8TIME_CAPABILITY,
					UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)),
			0x64);
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(TX_HIBERN8TIME_CAPABILITY,
					UIC_ARG_MPHY_TX_GEN_SEL_INDEX(1)),
			0x64);

		/*RX HB8_TIME CAP*/
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_HIBERN8TIME_CAPABILITY,
					UIC_ARG_MPHY_RX_GEN_SEL_INDEX(0)),
			0x64);
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_HIBERN8TIME_CAPABILITY,
					UIC_ARG_MPHY_RX_GEN_SEL_INDEX(1)),
			0x64);

		/*TX EQ 3DB*/
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(ANA_EQ_CTRL_REG_ATTR,
					UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)),
			0x5);

		/*RX garbage cnt = 32 SI*/
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_LANE_SOF_BKDOOR_ATT,
					UIC_ARG_MPHY_RX_GEN_SEL_INDEX(0)),
			0x9F);
		err = ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(RX_LANE_SOF_BKDOOR_ATT,
					UIC_ARG_MPHY_RX_GEN_SEL_INDEX(1)),
			0x9F);
	}

	/* bypass B0 reduce phy power ECO */
	err = ufshcd_dme_set(hba, UIC_ARG_MIB(0xfc), 0xfc);
	if (err)
		pr_err("Writing 0xfc error \n");

	pr_debug("ufs: ufs_spacemit_k3_uniprov1p6_init done.\n");

	err = spacemit_k3_ufs_get_sys1clk_1us(hba, &real_sysclk);
	if (err) {
		dev_err(hba->dev, "ufs: failed to derive SYS1CLK_1US: %d\n",
			err);
		return err;
	}

	ufshcd_writel(hba, real_sysclk, UFS_SYS1CLK_1US_REG);

	tx_symbol_clk_ns_us = DIV_ROUND_CLOSEST(1000U, real_sysclk);
	if (!tx_symbol_clk_ns_us)
		tx_symbol_clk_ns_us = 1;

	reg_val = tx_symbol_clk_ns_us << 10;
	ufshcd_writel(hba, reg_val, UFS_TX_SYMBOL_CLK_NS_US_REG);

	reg_val = real_sysclk * 100000;
	reg_val &= ~0xfU;
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

static int
spacemit_k3_ufs_link_startup_notify(struct ufs_hba *hba,
				    enum ufs_notify_change_status status)
{
	uint32_t reg_val;

	pr_debug("ufs: spacemit_k3_ufs_link_startup_notify, status:%d\n",
		 status);
	if (status == PRE_CHANGE) {
		/* init is done in hce_enable_notify(POST_CHANGE) */
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
			ufshcd_writel(hba, UIC_LINK_STARTUP,
				      REG_INTERRUPT_STATUS);
		if (reg_val & UIC_ERROR)
			ufshcd_writel(hba, UIC_ERROR, REG_INTERRUPT_STATUS);

		reg_val = ufshcd_readl(hba, REG_INTERRUPT_STATUS);
		pr_debug("ufs: REG_INTERRUPT_STATUS after clear (0x%x):0x%x\n",
			 REG_INTERRUPT_STATUS, reg_val);

		reg_val =
			ufshcd_readl(hba, REG_UIC_ERROR_CODE_PHY_ADAPTER_LAYER);
		pr_debug(
			"ufs: REG_UIC_ERROR_CODE_PHY_ADAPTER_LAYER(0x%x):0x%x\n",
			REG_UIC_ERROR_CODE_PHY_ADAPTER_LAYER, reg_val);

		/* add 0xe8 make UFS2.1 run GEAR3+2Lane@409M*/
		mdelay(5);
		ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(0xe8, UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)),
			0x97);
		mdelay(1);
		ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(0xe8, UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)),
			0xd7);
		mdelay(1);
		ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(0xe8, UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)),
			0x17);

		/* DL_AFC0REQTIMEOUTVAL_MAX */
		ufshcd_dme_set(hba, UIC_ARG_MIB(DL_AFC0REQTIMEOUTVAL),
			       UFS_DL_AFC0REQTIMEOUTVAL_MAX);

		/*LCC_DISABLE*/
		mdelay(5);
		ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(TX_LCC_ENABLE,
					UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)),
			0);
		mdelay(1);
		ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(TX_LCC_ENABLE,
					UIC_ARG_MPHY_TX_GEN_SEL_INDEX(1)),
			0);

		/*TX_Min_ActivateTime*/
		mdelay(1);
		ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(TX_MIN_ACTIVATETIME,
					UIC_ARG_MPHY_TX_GEN_SEL_INDEX(0)),
			0x0);
		mdelay(1);
		ufshcd_dme_set(
			hba,
			UIC_ARG_MIB_SEL(TX_MIN_ACTIVATETIME,
					UIC_ARG_MPHY_TX_GEN_SEL_INDEX(1)),
			0x0);
		mdelay(10);

		/* use backdoor reg to pre-set TX RATE/GEAR to let PLL lock before set_power_mode
		 * switch */
		ufshcd_dme_set(hba, UIC_ARG_MIB(ANA_HSGEAR_CTRL_ATTR), 0x25);
		mdelay(10);

		if (spacemit_k3_ufs_wait_mphy_pll_lock(hba, __func__))
			return -ETIMEDOUT;
	}

	return 0;
}

static int
spacemit_k3_ufs_hce_enable_notify(struct ufs_hba *hba,
				  enum ufs_notify_change_status status)
{
	int ret;

	pr_debug("ufs: spacemit_k3_ufs_hce_enable_notify, status:%d\n", status);

	if (status == PRE_CHANGE) {
		/*do nothing*/
	}

	if (status == POST_CHANGE) {
		ret = spacemit_k3_ufs_mphy_init(hba);
		if (ret)
			return ret;

		ret = spacemit_k3_ufs_unipro_init(hba);
		if (ret)
			return ret;

		/* Disable auto-hibern8 during bringup */
		ufshcd_writel(hba, 0, REG_AUTO_HIBERNATE_IDLE_TIMER);
	}

	return 0;
}

static int spacemit_k3_ufs_init(struct ufs_hba *hba)
{
	/* Mirror Linux behavior: disable LCC for controller stability */
	hba->quirks |= UFSHCD_QUIRK_BROKEN_LCC;

	return 0;
}

static const struct ufs_hba_ops spacemit_k3_ufs_vops = {
	.init = spacemit_k3_ufs_init,
	.hce_enable_notify = spacemit_k3_ufs_hce_enable_notify,
	.link_startup_notify = spacemit_k3_ufs_link_startup_notify,
	.device_reset = spacemit_k3_ufs_silent_reset,
	.set_ref_clk = spacemit_k3_ufs_set_ref_clk,
	.set_power_mode = spacemit_k3_ufs_set_power_mode,
};

static int spacemit_k3_ufs_pltfm_bind(struct udevice *dev)
{
	struct udevice *scsi_dev;
	int ret;

	ret = ufs_scsi_bind(dev, &scsi_dev);
	if (ret)
		pr_err("ufs: ufs_scsi_bind failed: %d\n", ret);
	return ret;
}

static int spacemit_k3_ufs_pltfm_probe(struct udevice *dev)
{
	struct spacemit_k3_ufs_priv *priv = dev_get_priv(dev);
	struct ufs_hba *hba = dev_get_uclass_priv(dev);
	struct ufs_hba_ops *hba_ops = (struct ufs_hba_ops *)dev->driver_data;
	struct udevice *scsi_dev;
	struct scsi_plat *scsi_plat;
	int ret;
	int retries;

	/* Bring clocks/reset up as early as possible */
	ret = spacemit_k3_ufs_clk_enable(dev);
	if (ret)
		return ret;

#if defined(CONFIG_SPL_BUILD)
	/*
	 * SPL often starts from a warm hardware state. Pre-shutdown once so the
	 * first ufshcd_probe starts from the same clean state as retry path.
	 */
	hba->dev = dev;
	hba->ops = hba_ops;
	hba->mmio_base = (void *)dev_read_addr(dev);
	if (hba->ops && hba->ops->device_reset) {
		hba->ops->device_reset(hba);
	}
#endif

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
#if !defined(CONFIG_SPL_BUILD)
		int lun_cfg_ret;

		if (spacemit_k3_ufs_should_enforce_single_lun()) {
			/* Check and configure single LUN if needed - skip in SPL */
			lun_cfg_ret = spacemit_k3_ufs_check_and_config_single_lun(dev);
			if (lun_cfg_ret > 0) {
				dev_info(
					hba->dev,
					"restarting UFS once to apply single-LUN layout\n");
				hba->ops->device_reset(hba);
				ret = ufshcd_probe(dev, hba_ops);
				if (ret) {
					spacemit_k3_ufs_phy_shutdown(hba, priv);
					spacemit_k3_ufs_clk_disable(priv);
					dev_err(hba->dev,
						"ufs reprobe after LUN config failed: %d\n",
						ret);
					return ret;
				}

				lun_cfg_ret =
					spacemit_k3_ufs_check_and_config_single_lun(
						dev);
			}

			if (lun_cfg_ret < 0) {
				dev_warn(hba->dev,
					 "failed to enforce single-LUN layout: %d\n",
					 lun_cfg_ret);
			} else if (lun_cfg_ret > 0) {
				dev_warn(
					hba->dev,
					"single-LUN config pending, a cold power cycle may be required\n");
			}
		}
#endif
		/* Limit to single LUN - use only the main user data partition */
		device_find_first_child(dev, &scsi_dev);
		if (scsi_dev) {
			scsi_plat = dev_get_uclass_plat(scsi_dev);
			scsi_plat->max_id = 1; /* UFS has single target */
			scsi_plat->max_lun = 1; /* Use only main LUN */
		} else {
			pr_err("ufs: scsi_dev child not found!\n");
		}
	}

	return ret;
}

static int spacemit_k3_ufs_of_to_plat(struct udevice *dev)
{
	const char *compat;
	const void *prop;
	int compat_length;
	int ret;
	u32 ref_clk_freq;

	struct spacemit_k3_ufs_priv *priv = dev_get_priv(dev);

	compat = ofnode_get_property(dev->node_, "compatible", &compat_length);
	if (!compat) {
		return -1;
	}

	if (!strcmp(compat, "spacemit,k3-ufshci")) {
		priv->phy_mng_base = UFS_ARASAN_PHY_MNG_BASE;
		priv->atop_base = UFS_ARASAN_TOP_BASE;
	}

	/*
	 * Read reference clock frequency from DTS. It must be expressed in Hz
	 * and match one of the UFS-spec reference clock frequencies.
	 */
	prop = dev_read_prop(dev, "ref-clk-freq", NULL);
	if (!prop) {
		/* Default to 19.2MHz if not specified in DTS */
		priv->ref_clk_freq = UFS_REF_CLK_FREQ_19_2_MHZ;
		dev_dbg(dev,
			"ufs: ref-clk-freq not found in DTS, using default 19.2MHz\n");
	} else {
		ret = dev_read_u32(dev, "ref-clk-freq", &ref_clk_freq);
		if (ret) {
			dev_err(dev,
				"ufs: malformed ref-clk-freq property, ret=%d\n",
				ret);
			return ret;
		}

		ret = spacemit_k3_ufs_parse_ref_clk_freq(ref_clk_freq,
								&priv->ref_clk_freq);
		if (ret) {
			dev_err(dev,
				"ufs: invalid ref-clk-freq %u, expected "
				"19200000/26000000/38400000/52000000 Hz\n",
				ref_clk_freq);
			return ret;
		}
	}

	ret = dev_read_u32(dev, "clock-freq", &priv->clock_freq_hz);
	if (ret)
		priv->clock_freq_hz = 0;

	ret = clk_get_by_name(dev, "aclk", &priv->aclk);
	if (ret) {
		dev_warn(dev,
			 "ufs: failed to get aclk by name, fallback to index 0, ret=%d\n",
			 ret);
		ret = clk_get_by_index(dev, 0, &priv->aclk);
	}
	if (ret) {
		dev_err(dev, "ufs: failed to get aclk, ret=%d\n", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "aclk_reset", &priv->reset);
	if (ret) {
		dev_warn(dev,
			 "ufs: failed to get reset by name, fallback to index 0, ret=%d\n",
			 ret);
		ret = reset_get_by_index(dev, 0, &priv->reset);
	}
	if (ret) {
		dev_err(dev, "ufs: failed to get reset, ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static int spacemit_k3_ufs_pltfm_remove(struct udevice *dev)
{
	struct spacemit_k3_ufs_priv *priv = dev_get_priv(dev);

	spacemit_k3_ufs_clk_disable(priv);

	return 0;
}

static const struct udevice_id spacemit_k3_ufs_pltfm_ids[] = {
	{
		.compatible = "spacemit,k3-ufshci",
		.data = (ulong)&spacemit_k3_ufs_vops,
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
