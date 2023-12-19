// SPDX-License-Identifier: GPL-2.0
/*
 * spacemit k1x gpio driver
 *
 * Copyright (C) 2023 Spacemit
 *
 */

#include <common.h>
#include <asm/arch/gpio.h>
#include <asm/io.h>
#include <errno.h>
#include <asm/gpio.h>
#include <linux/bitops.h>
#include <clk.h>
#include "k1x_gpio.h"

#ifdef CONFIG_DM_GPIO
#include <dm/read.h>
#include <dm/device.h>

static void __iomem *k1x_gpio_base;
#endif

#ifndef K1X_MAX_GPIO
#define K1X_MAX_GPIO	128
#endif

#define GPIO_TO_REG(gp)		(gp >> 5)
#define GPIO_TO_BIT(gp)		(1 << (gp & 0x1f))
#define GPIO_VAL(gp, val)	((val >> (gp & 0x1f)) & 0x01)

static inline void *get_gpio_base(int bank)
{
	const unsigned long offset[] = {0, 4, 8, 0x100};
	/* gpio register bank offset */
#ifdef K1X_GPIO_BASE
	return (struct gpio_reg *)(K1X_GPIO_BASE + offset[bank]);
#else
	return (struct gpio_reg *)(k1x_gpio_base + offset[bank]);
#endif
}

static int _gpio_direction_input(unsigned gpio)
{
	struct gpio_reg *gpio_reg_bank;

	if (gpio >= K1X_MAX_GPIO) {
		printf("%s: Invalid GPIO %d\n", __func__, gpio);
		return -1;
	}

	gpio_reg_bank = get_gpio_base(GPIO_TO_REG(gpio));
	writel(GPIO_TO_BIT(gpio), &gpio_reg_bank->gcdr);
	return 0;
}

static int _gpio_set_value(unsigned gpio, int value)
{
	struct gpio_reg *gpio_reg_bank;

	if (gpio >= K1X_MAX_GPIO) {
		printf("%s: Invalid GPIO %d\n", __func__, gpio);
		return -1;
	}

	gpio_reg_bank = get_gpio_base(GPIO_TO_REG(gpio));
	if (value)
		writel(GPIO_TO_BIT(gpio), &gpio_reg_bank->gpsr);
	else
		writel(GPIO_TO_BIT(gpio), &gpio_reg_bank->gpcr);

	return 0;
}

static int _gpio_direction_output(unsigned gpio, int value)
{
	struct gpio_reg *gpio_reg_bank;

	if (gpio >= K1X_MAX_GPIO) {
		printf("%s: Invalid GPIO %d\n", __func__, gpio);
		return -1;
	}

	gpio_reg_bank = get_gpio_base(GPIO_TO_REG(gpio));
	writel(GPIO_TO_BIT(gpio), &gpio_reg_bank->gsdr);
	_gpio_set_value(gpio, value);
	return 0;
}

static int _gpio_get_value(unsigned gpio)
{
	struct gpio_reg *gpio_reg_bank;
	u32 gpio_val;

	if (gpio >= K1X_MAX_GPIO) {
		printf("%s: Invalid GPIO %d\n", __func__, gpio);
		return -1;
	}

	gpio_reg_bank = get_gpio_base(GPIO_TO_REG(gpio));
	gpio_val = readl(&gpio_reg_bank->gplr);

	return GPIO_VAL(gpio, gpio_val);
}

#ifdef CONFIG_DM_GPIO
static int gpio_k1x_bind(struct udevice *dev)
{
	k1x_gpio_base = dev_remap_addr_index(dev, 0);

	return 0;
}
static int gpio_k1x_probe(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct clk gpio_clk;
	int ret = 0;

	uc_priv->gpio_count = dev_read_u32_default(dev, "gpio-count", 0);

	ret = clk_get_by_index(dev, 0, &gpio_clk);
	if (ret) 
		return ret; 
	clk_enable(&gpio_clk);

	return 0;
}

static const struct udevice_id gpio_k1x_ids[] = {
	{ .compatible = "spacemit,k1x-gpio" },
	{ }
};

static int k1x_gpio_get_value(struct udevice *dev, unsigned int gpio)
{
	return _gpio_get_value(gpio);
}

static int k1x_gpio_set_value(struct udevice *dev, unsigned int gpio,
				   int value)
{
	return _gpio_set_value(gpio, value);
}

static int k1x_gpio_direction_input(struct udevice *dev, unsigned int gpio)
{
	return _gpio_direction_input(gpio);
}

static int k1x_gpio_direction_output(struct udevice *dev, unsigned int gpio,
					  int value)
{
	return _gpio_direction_output(gpio, value);
}

static const struct dm_gpio_ops gpio_k1x_ops = {
	.direction_input	= k1x_gpio_direction_input,
	.direction_output	= k1x_gpio_direction_output,
	.get_value		= k1x_gpio_get_value,
	.set_value		= k1x_gpio_set_value,
};
U_BOOT_DRIVER(gpio_k1x) = {
	.name	= "gpio_k1x",
	.id	= UCLASS_GPIO,
	.ops	= &gpio_k1x_ops,
	.of_match = gpio_k1x_ids,
	.bind	= gpio_k1x_bind,
	.probe	= gpio_k1x_probe,
};

#else

int gpio_request(unsigned gpio, const char *label)
{
	if (gpio >= K1X_MAX_GPIO) {
		printf("%s: Invalid GPIO requested %d\n", __func__, gpio);
		return -1;
	}
	return 0;
}

int gpio_free(unsigned gpio)
{
	return 0;
}

int gpio_direction_input(unsigned gpio)
{
	return _gpio_direction_input(gpio);
}

int gpio_direction_output(unsigned gpio, int value)
{
	return _gpio_direction_output(gpio, value);
}

int gpio_get_value(unsigned gpio)
{
	return _gpio_get_value(gpio);
}

int gpio_set_value(unsigned gpio, int value)
{
	return _gpio_set_value(gpio, value);
}
#endif
