// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <dm.h>
#include <dm/lists.h>
#include <errno.h>
#include <log.h>
#include <power/spacemit/spacemit_pmic.h>
#include <power/pmic.h>
#include <sysreset.h>

static int pm8xx_reg_count(struct udevice *dev)
{
	struct pm8xx_priv *priv = dev_get_priv(dev);

	switch (priv->variant) {
	case SPACEMIT_SPM8821_ID:
		return SPACEMIT_SPM8821_MAX_REG;
	case SPACEMIT_PM853_ID:
		return SPACEMIT_PM853_MAX_REG;
	default:
		debug("do not support this varaint: %d\n", priv->variant);
		break;
	}

	return 0;
}

static int pm8xx_read(struct udevice *dev, uint reg, uint8_t *buff, int len)
{
	int ret;

	ret = dm_i2c_read(dev, reg, buff, len);
	if (ret) {
		debug("read error from device: %p register: %#x!\n", dev, reg);
		return ret;
	}

	return 0;
}

static int pm8xx_write(struct udevice *dev, uint reg, const uint8_t *buff,
			  int len)
{
	int ret;

	ret = dm_i2c_write(dev, reg, buff, len);
	if (ret) {
		debug("write error to device: %p register: %#x!\n", dev, reg);
		return ret;
	}

	return 0;
}

static struct dm_pmic_ops pm8xx_ops = {
	.reg_count = pm8xx_reg_count,
	.read = pm8xx_read,
	.write = pm8xx_write,
};

static const struct udevice_id pm8xx_ids[] = {
	{ .compatible = "spacemit,spm8821", .data = SPACEMIT_SPM8821_ID_REG, },
	{ .compatible = "spacemit,pm853", .data = SPACEMIT_PM853_ID_REG, },
	{ }
};

static int pm8xx_probe(struct udevice *dev)
{
	int ret = 0;
	u8 variant;
	struct pm8xx_priv *priv = dev_get_priv(dev);
	ulong driver_data = dev_get_driver_data(dev);

	ret = pm8xx_read(dev, driver_data, &variant, 1);
	if (ret)
		return ret;

	priv->variant = variant;

	return 0;
}

#if CONFIG_IS_ENABLED(PMIC_CHILDREN)
static const struct pmic_child_info pmic_children_info[] = {
	{ .prefix = "DCDC_REG", .driver = "pm8xx_buck"},
	{ .prefix = "LDO_REG", .driver = "pm8xx_ldo"},
	{ .prefix = "SWITCH_REG", .driver = "pm8xx_switch"},
	{ },
};

static int pm8xx_bind(struct udevice *dev)
{
	ofnode regulators_node;
	int children;

	regulators_node = dev_read_subnode(dev, "regulators");
	if (!ofnode_valid(regulators_node)) {
		debug("%s: %s regulators subnode not found!\n", __func__,
		      dev->name);
		return -ENXIO;
	}

	debug("%s: '%s' - found regulators subnode\n", __func__, dev->name);

/**
 * 	:Implement this function later
 *
 *	if (CONFIG_IS_ENABLED(SYSRESET)) {
 *		ret = device_bind_driver_to_node(dev, "rk8xx_sysreset",
 *						 "rk8xx_sysreset",
 *						 dev_ofnode(dev), NULL);
 *		if (ret)
 *			return ret;
 *	}
 */
	children = pmic_bind_children(dev, regulators_node, pmic_children_info);
	if (!children)
		debug("%s: %s - no child found\n", __func__, dev->name);

	/* Always return success for this device */
	return 0;
}
#endif

U_BOOT_DRIVER(spacemit_pm8xx) = {
	.name = "spacemit_pm8xx",
	.id = UCLASS_PMIC,
	.of_match = pm8xx_ids,
#if CONFIG_IS_ENABLED(PMIC_CHILDREN)
	.bind = pm8xx_bind,
#endif
	.priv_auto	  = sizeof(struct pm8xx_priv),
	.probe = pm8xx_probe,
	.ops = &pm8xx_ops,
};
