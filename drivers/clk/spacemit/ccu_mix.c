// SPDX-License-Identifier: GPL-2.0-only
/*
 * Spacemit clock type mix(div/mux/gate/factor)
 *
 * Copyright (c) 2023, spacemit Corporation.
 *
 */
#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <clk-uclass.h>
#include <dm/device.h>
#include <dm/devres.h>
#include <linux/bitops.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/iopoll.h>
#include <clk.h>
#include <div64.h>

#include "ccu_mix.h"

#define TIMEOUT_LIMIT (20000) /* max timeout 10000us */
static int twsi8_reg_val = 0x04;
static int ccu_mix_disable(struct clk *clk)
{
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	struct ccu_gate_config *gate = mix->gate;
	u32 tmp;

	if (!gate)
		return 0;

	if (clk->id == CLK_TWSI8){
		twsi8_reg_val &= ~0x7;
		twsi8_reg_val |= 0x4;
		tmp = twsi8_reg_val;
		if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
			writel(tmp, common->base + common->reg_ctrl);
		else
			writel(tmp, common->base + common->reg_sel);

		return 0;
	}

	if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
		tmp = readl(common->base + common->reg_ctrl);
	else
		tmp = readl(common->base + common->reg_sel);

	tmp &= ~gate->gate_mask;
	tmp |= gate->val_disable;

	if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
		writel(tmp, common->base + common->reg_ctrl);
	else
		writel(tmp, common->base + common->reg_sel);

	if (gate->flags & SPACEMIT_CLK_GATE_NEED_DELAY) {
		udelay(200);
	}

	return 0;
}

static int ccu_mix_enable(struct clk *clk)
{
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	struct ccu_gate_config *gate = mix->gate;
	u32 tmp;
	u32 val = 0;
	int timeout_power = 1;

	if (!gate)
		return 0;

	if (clk->id == CLK_TWSI8){
		twsi8_reg_val &= ~0x7;
		twsi8_reg_val |= 0x3;
		tmp = twsi8_reg_val;
		if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
			writel(tmp, common->base + common->reg_ctrl);
		else
			writel(tmp, common->base + common->reg_sel);

		return 0;
	}

	if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
		tmp = readl(common->base + common->reg_ctrl);
	else
		tmp = readl(common->base + common->reg_sel);

	tmp &= ~gate->gate_mask;
	tmp |= gate->val_enable;

	if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
		writel(tmp, common->base + common->reg_ctrl);
	else
		writel(tmp, common->base + common->reg_sel);

	if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
		val = readl(common->base + common->reg_ctrl);
	else
		val = readl(common->base + common->reg_sel);

	while ((val & gate->gate_mask) != gate->val_enable && (timeout_power < TIMEOUT_LIMIT)) {
		udelay(timeout_power);
		if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
			|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
			val = readl(common->base + common->reg_ctrl);
		else
			val = readl(common->base + common->reg_sel);
		timeout_power *= 10;
	}

	if (timeout_power > 1) {
		if (val == tmp)
			pr_err("write clk_gate %s timeout occur, read pass after %d us delay\n",
			clk_hw_get_name(&common->clk), timeout_power);
		else
			pr_err("write clk_gate  %s timeout after %d us!\n", clk_hw_get_name(&common->clk), timeout_power);
	}

	if (gate->flags & SPACEMIT_CLK_GATE_NEED_DELAY) {
		udelay(200);
	}

	return 0;
}

static ulong ccu_mix_get_rate(struct clk *clk)
{
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	struct ccu_div_config *div = mix->div;
	unsigned long parent_rate = clk_get_parent_rate(clk);
	unsigned long val;
	u32 reg;

	if (clk->id == CLK_TWSI8){
		val = parent_rate;
		return val;
	}

	if (!div){
		if (mix->factor)
			val =  parent_rate * mix->factor->mul / mix->factor->div;
		else
		    val =  parent_rate;
		return val;
	}
	if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
		reg = readl(common->base + common->reg_ctrl);
	else
		reg = readl(common->base + common->reg_sel);

	val = reg >> div->shift;
	val &= (1 << div->width) - 1;

	val = divider_recalc_rate(clk, parent_rate, val, div->table,
				  div->flags, div->width);

	return val;
}

static ulong ccu_mix_round_rate(struct clk *clk, ulong rate)
{
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_div_config *div_config = mix->div;
	unsigned long parent_rate = clk_get_parent_rate(clk);
	ulong best_rate;
	unsigned int div;

	if (!div_config)
		return parent_rate;

	div = DIV_ROUND_UP_ULL((u64)parent_rate, rate);

	if (div > BIT(div_config->width))
		return parent_rate / (BIT(div_config->width));
	if (div == 1)
		return parent_rate;
	if(abs(rate - parent_rate / div) > abs(rate - parent_rate / (div - 1)))
		best_rate = parent_rate / (div - 1);
	else
		best_rate = parent_rate / div;

	return best_rate;
}

static ulong ccu_mix_set_rate(struct clk *clk, unsigned long rate)
{
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	struct ccu_div_config *div_config = mix->div;
	unsigned long parent_rate = clk_get_parent_rate(clk);

	unsigned long val;
	u32 reg;
	int ret, timeout = 50;
	unsigned int div;
	const struct clk_div_table *clkt;

	if (!div_config)
		return 0;

	if (clk->id == CLK_TWSI8)
		return 0;

	div = DIV_ROUND_UP_ULL((u64)parent_rate, rate);

	if(div_config->table){
		for (clkt = div_config->table; clkt->div; clkt++)
			if (clkt->div == div)
				val = clkt->val;
			else
				val = div - 1;
	}else {
		val = div - 1;
	}
	if (val > BIT(div_config->width) - 1)
		return 0;

	if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
		reg = readl(common->base + common->reg_ctrl);
	else
		reg = readl(common->base + common->reg_sel);

	reg &= ~GENMASK(div_config->width + div_config->shift - 1, div_config->shift);

	if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
		writel(reg | (val << div_config->shift),
	       common->base + common->reg_ctrl);
	else
		writel(reg | (val << div_config->shift),
	       common->base + common->reg_sel);

	if (common->reg_type == CLK_DIV_TYPE_1REG_FC_V2
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4) {
			timeout = 50;
			val = readl(common->base + common->reg_ctrl);
			val |= common->fc;
			writel(val, common->base + common->reg_ctrl);
			do {
				val = readl(common->base + common->reg_ctrl);
				timeout--;
				if (!(val & (common->fc)))
					break;
			} while (timeout);

			if (timeout == 0) {
				pr_err("%s of %s timeout\n", __func__, clk_hw_get_name(&common->clk));
				timeout = 5000;
				do {
					val = readl(common->base + common->reg_ctrl);
					timeout--;
					if (!(val & (common->fc)))
						break;
				} while (timeout);
				if (timeout != 0) {
					ret = 0;
				} else {
					ret = -EBUSY;
					goto error;
			   }
		   }
	   }

error:

	return ret;
}

unsigned int ccu_mix_get_parent(struct clk *clk)
{
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	struct ccu_mux_config *mux = mix->mux;
	u32 reg;
	unsigned int parent;

	if(!mux)
		return 0;

	if (clk->id == CLK_TWSI8){
		parent = (twsi8_reg_val >> 4) & 0x7;
		return parent;
	}

	if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
		reg = readl(common->base + common->reg_ctrl);
	else
		reg = readl(common->base + common->reg_sel);

	parent = reg >> mux->shift;
	parent &= (1 << mux->width) - 1;

	if (mux->table) {
		int num_parents = common->num_parents;
		int i;

		for (i = 0; i < num_parents; i++)
			if (mux->table[i] == parent)
				return i;
	}
	return parent;
}

static int ccu_mix_set_parent(struct clk *clk, struct clk *parent)
{
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	struct ccu_mux_config *mux = mix->mux;
	int index;
	u32 reg, i;

	if (!parent)
		return -EINVAL;

	for (i = 0; i < common->num_parents; i++) {
		if (!strcmp(parent->dev->name, common->parent_names[i])){
			index = i;
			break;
		}
	}

	if (index < 0) {
		pr_err("Could not fetch index\n");
		return index;
	}

	if (clk->id == CLK_TWSI8){
		twsi8_reg_val &= ~GENMASK(mux->width + mux->shift - 1, mux->shift);
		twsi8_reg_val |= (index << mux->shift);
		reg = twsi8_reg_val;
		if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
			writel(reg, common->base + common->reg_ctrl);
		else
			writel(reg, common->base + common->reg_sel);

		return 0;
	}

	if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
		reg = readl(common->base + common->reg_ctrl);
	else
		reg = readl(common->base + common->reg_sel);

	reg &= ~GENMASK(mux->width + mux->shift - 1, mux->shift);

	if (common->reg_type == CLK_DIV_TYPE_1REG_NOFC_V1
	|| common->reg_type == CLK_DIV_TYPE_1REG_FC_V2)
		writel(reg | (index << mux->shift), common->base + common->reg_ctrl);
	else
		writel(reg | (index << mux->shift), common->base + common->reg_sel);

	return 0;
}

const struct clk_ops ccu_mix_ops = {
	.disable 	= ccu_mix_disable,
	.enable 	= ccu_mix_enable,
	.set_parent 	= ccu_mix_set_parent,
	.get_rate	= ccu_mix_get_rate,
	.set_rate 	= ccu_mix_set_rate,
	.round_rate 	= ccu_mix_round_rate,
};

U_BOOT_DRIVER(ccu_clk_mix) = {
	.name	= CCU_CLK_MIX,
	.id 	= UCLASS_CLK,
	.ops	= &ccu_mix_ops,
};

