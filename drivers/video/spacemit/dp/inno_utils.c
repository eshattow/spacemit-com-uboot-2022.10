// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Spacemit Co., Ltd.
 *
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <malloc.h>
#include "inno_utils.h"
#include "inno_conn.h"

void osal_usleep(uint32_t us)
{
	udelay(us);
}

void osal_msleep(uint32_t ms)
{
	mdelay(ms);
}

void osal_write32(uint32_t offset, uint32_t val, struct inno_conn_t *conn)
{
	*(volatile uint32_t *)(conn->reg_mmap_addr + offset) = val;
	osal_printf_func("[w] reg: %#x, val: %#x\n", offset, val);
}

uint32_t osal_read32(uint32_t offset, struct inno_conn_t *conn)
{
	uint32_t val = *(volatile uint32_t *)(conn->reg_mmap_addr + offset);

	osal_printf_func("[r] reg: %#x, val: %#x\n", offset, val);
	return val;
}

void osal_update_bits(uint32_t reg, uint32_t mask, uint32_t val,
		      struct inno_conn_t *conn)
{
	unsigned int tmp, orig;

	orig = osal_read32(reg, conn);
	tmp = orig & ~mask;
	tmp |= val & mask;

	osal_write32(reg, tmp, conn);
}

void *osal_malloc(uint32_t size)
{
	return malloc(size);
}

void osal_free(void *ptr)
{
	return free(ptr);
}

void *osal_memset(void *s, int32_t c, size_t n)
{
	return memset(s, c, n);
}

void *osal_memcpy(void *dest, const void *src, size_t n)
{
	return memcpy(dest, src, n);
}

int osal_memcmp(void *sl, const void *s2, size_t n)
{
return memcmp(sl, s2, n);
}
