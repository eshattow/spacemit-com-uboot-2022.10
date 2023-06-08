// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 */

#include <common.h>
#include <irq_func.h>
#include <asm/cache.h>

/*
 * cleanup_before_linux() is called just before we call linux
 * it prepares the processor for linux
 *
 * we disable interrupt and caches.
 */
int cleanup_before_linux(void)
{
	disable_interrupts();

	cache_flush();

	return 0;
}

#if CONFIG_IS_ENABLED(RISCV_ISA_ZICBOM)
int check_cache_range(unsigned long start, unsigned long end)
{
	int ok = 1;

	if (start & (CONFIG_RISCV_CBOM_BLOCK_SIZE - 1))
		ok = 0;

	if (end & (CONFIG_RISCV_CBOM_BLOCK_SIZE - 1))
		ok = 0;

	if (!ok) {
		warn_non_spl("CACHE: Misaligned operation at range [%08lx, %08lx]\n",
			start, end);
	}

	return ok;
}

void invalidate_dcache_range(unsigned long start, unsigned long end)
{
	if (!check_cache_range(start, end))
		return;

	while (start < end) {
		cbo_invalid(start);
		start += CONFIG_RISCV_CBOM_BLOCK_SIZE;
	}
}

void flush_dcache_range(unsigned long start, unsigned long end)
{
	if (!check_cache_range(start, end))
		return;

	while (start < end) {
		cbo_flush(start);
		start += CONFIG_RISCV_CBOM_BLOCK_SIZE;
	}
}
#endif
