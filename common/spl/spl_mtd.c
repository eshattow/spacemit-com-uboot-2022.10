// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#include <common.h>
#include <image.h>
#include <log.h>
#include <spl.h>
#include <asm/global_data.h>
#include <mtd.h>
#include <linux/err.h>
#include <env.h>

static uint mtd_len_to_pages(struct mtd_info *mtd, u64 len)
{
	do_div(len, mtd->writesize);

	return len;
}

static bool mtd_is_aligned_with_min_io_size(struct mtd_info *mtd, u64 size)
{
	return !do_div(size, mtd->writesize);
}

static bool mtd_is_aligned_with_block_size(struct mtd_info *mtd, u64 size)
{
	return !do_div(size, mtd->erasesize);
}

static int spl_mtd_read(struct mtd_info *mtd, ulong sector, ulong count, void *buf)
{
	bool read, raw, woob, has_pages = false;
	u64 start_off, off, len, remaining;
	struct mtd_oob_ops io_op = {};
	uint npages;
	int ret = -1;
	u8 *buffer = (u8 *)buf;
	debug("sector:%lx, count:%lx, buffer:%lx\n", sector, count, (ulong)buffer);

	start_off = sector;
	if (!mtd_is_aligned_with_min_io_size(mtd, start_off)) {
		printf("Offset not aligned with a page (0x%x)\n",
		       mtd->writesize);
		return ret;
	}

	len = count;
	if (!mtd_is_aligned_with_min_io_size(mtd, len)) {
		len = round_up(len, mtd->writesize);
		debug("Size not on a page boundary (0x%x), rounding to 0x%llx\n",
		       mtd->writesize, len);
	}
	if (mtd->type == MTD_NANDFLASH || mtd->type == MTD_MLCNANDFLASH)
		has_pages = true;

	remaining = len;
	npages = mtd_len_to_pages(mtd, len);

	io_op.mode = raw ? MTD_OPS_RAW : MTD_OPS_AUTO_OOB;
	io_op.len = has_pages ? mtd->writesize : len;
	io_op.ooblen = woob ? mtd->oobsize : 0;
	io_op.datbuf = buffer;
	io_op.oobbuf = woob ? &buffer[len] : NULL;

	/* Search for the first good block after the given offset */
	off = start_off;
	while (mtd_block_isbad(mtd, off))
		off += mtd->erasesize;

	/* Loop over the pages to do the actual read/write */
	while (remaining) {
		/* Skip the block if it is bad */
		if (mtd_is_aligned_with_block_size(mtd, off) &&
		    mtd_block_isbad(mtd, off)) {
			off += mtd->erasesize;
			continue;
		}

		ret = mtd_read_oob(mtd, off, &io_op);
		if (ret) {
			printf("Failure while %s at offset 0x%llx\n",
			       read ? "reading" : "writing", off);
			break;
		}

		off += io_op.retlen;
		remaining -= io_op.retlen;
		io_op.datbuf += io_op.retlen;
		io_op.oobbuf += io_op.oobretlen;
	}
	return ret;
}


static ulong spl_spi_load_read(struct spl_load_info *load, ulong sector,
			       ulong count, void *buf)
{
	int ret;

	debug("%s: sector %lx, count %lx, buf %lx\n",
	      __func__, sector, count, (ulong)buf);

	struct mtd_info *mtd = load->dev;
	ret = spl_mtd_read(mtd, sector, count, buf);
	if (!ret)
		return count;
	else
		return 0;
}


static int spl_spi_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
	struct image_header *header;
	struct mtd_info *mtd;
	int err = 0;
	ulong len;

	mtd_probe_devices();
	mtd = get_mtd_device_nm(SPL_SPI_BOOT_MTD_NAME);
	if (IS_ERR_OR_NULL(mtd)){
		printf("MTD device %s not found\n", SPL_SPI_BOOT_MTD_NAME);
		return -1;
	}

	debug("mtd->type:%x, mtd->writesize:%x, mtd:%p\n", mtd->type, mtd->writesize, mtd);
	len = sizeof(*header);
	if (!mtd_is_aligned_with_min_io_size(mtd, len)) {
		len = round_up(len, mtd->writesize);
		printf("Size not on a page boundary (0x%x), rounding to 0x%lx\n",
		       mtd->writesize, len);
	}

	header = spl_get_load_buffer(-sizeof(*header), sizeof(*header));
	err = spl_mtd_read(mtd, 0, len, (void *)header);
	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
	    image_get_magic(header) == FDT_MAGIC) {
		struct spl_load_info load;

		debug("Found FIT\n");
		load.dev = mtd;
		load.priv = NULL;
		load.filename = NULL;
		load.bl_len = 1;
		load.read = spl_spi_load_read;
		err = spl_load_simple_fit(spl_image, &load, 0, header);
	} else {
		debug("unsupport Legacy image\n");
		return -1;
	}

	return err;
}

/* Use priorty 1 so that boards can override this */
SPL_LOAD_IMAGE_METHOD("SPI", 0, BOOT_DEVICE_SPI, spl_spi_load_image);
SPL_LOAD_IMAGE_METHOD("SPI", 0, BOOT_DEVICE_NAND, spl_spi_load_image);
