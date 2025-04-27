# SPDX-License-Identifier: GPL-2.0
#
# Copyright (C) 2025 SPACEMIT Micro Limited
#

ifdef CONFIG_SPL_BUILD
obj-y += ddr-snps-lp4x-fpga-init.o ddr_freq.o
else
obj-$(CONFIG_DYNAMIC_DDR_CLK_FREQ) += ddr_freq.o
endif
