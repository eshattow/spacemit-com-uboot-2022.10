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
#include <mapmem.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#define MAX_IDR_ID	16

struct idr_layer {
	int	used;
	void	*ptr;
};

struct idr {
	struct idr_layer id[MAX_IDR_ID];
	bool updated;
};

/* IDR function declarations */
void *idr_find(struct idr *idp, int id);
void idr_remove(struct idr *idp, int id);
int idr_alloc(struct idr *idp, void *ptr, int start, int end, gfp_t gfp_mask);

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

int spl_mtd_read(struct mtd_info *mtd, ulong sector, ulong count, void *buf)
{
	bool read, raw, woob, has_pages = false;
	u64 start_off, off, len, remaining;
	struct mtd_oob_ops io_op = {};
	uint npages;
	int ret = -1;

	u8 *buffer = map_sysmem((u64)buf, 0);
	if (!buffer)
		return -1;

	debug("sector:%lx, count:%lx, buffer:%lx\n", sector, count, (ulong)buffer);
	start_off = sector;
	if (!mtd_is_aligned_with_min_io_size(mtd, start_off)) {
		pr_debug("Offset not aligned with a page (0x%x)\n",
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
			pr_debug("Failure while %s at offset 0x%llx\n",
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
	debug("%s, get mtd:%p\n", __func__, mtd);
	ret = spl_mtd_read(mtd, sector, count, buf);
	if (!ret)
		return count;
	else
		return 0;
}


static int mtd_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev, struct mtd_info *mtd)
{
	struct image_header *header;
	ulong len;
	int err = 0;
	len = sizeof(*header);
	if (!mtd_is_aligned_with_min_io_size(mtd, len)) {
		len = round_up(len, mtd->writesize);
		pr_debug("Size not on a page boundary (0x%x), rounding to 0x%lx\n",
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

static void compact_mtd_device_list(void)
{
	struct mtd_info *devices[MAX_IDR_ID];
	int device_count = 0;
	int i, new_index = 0;

	extern struct idr mtd_idr;

	/* Collect all valid MTD devices */
	for (i = 0; i < MAX_IDR_ID; i++) {
		struct mtd_info *mtd = idr_find(&mtd_idr, i);
		if (mtd && device_count < MAX_IDR_ID) {
			devices[device_count] = mtd;
			device_count++;
		}
	}

	if (device_count == 0) {
		return;
	}

	/* Clear the IDR table */
	for (i = 0; i < MAX_IDR_ID; i++) {
		if (idr_find(&mtd_idr, i)) {
			idr_remove(&mtd_idr, i);
		}
	}

	/* Re-add devices with consecutive indices */
	for (i = 0; i < device_count; i++) {
		int ret = idr_alloc(&mtd_idr, devices[i], new_index, new_index + 1, GFP_KERNEL);
		if (ret >= 0) {
			devices[i]->index = new_index;
			new_index++;
		}
	}

	/* Mark IDR as updated */
	mtd_idr.updated = true;
}


static void remove_nor_partitions(void)
{
	struct mtd_info *mtd;
	struct mtd_info *nor_devices[2];
	int nor_count = 0;

	mtd_for_each_device(mtd) {
		if (mtd->parent == NULL &&
			(mtd->type == MTD_NORFLASH || mtd->type == MTD_DATAFLASH ||
				mtd->type == MTD_ROM)) {
			if (nor_count < 10) {
				nor_devices[nor_count++] = mtd;
			}
		} else {
			pr_info("Skipping non-NOR device: %s (type=%d)\n",
					mtd->name, mtd->type);
		}
	}

	for (int i = 0; i < nor_count; i++) {
		mtd = nor_devices[i];
		struct mtd_info *slave, *tmp;
		int partition_count = 0;
		int ret;

		list_for_each_entry_safe(slave, tmp, &mtd->partitions, node) {
			pr_info("  Removing NOR partition: %s (offset: 0x%llx, size: 0x%llx)\n",
					slave->name, slave->offset, slave->size);
			ret = del_mtd_device(slave);
			if (ret) {
				pr_info("  Failed to remove partition %s (ret=%d)\n", slave->name, ret);
			} else {
				partition_count++;
			}
		}
	}

	compact_mtd_device_list();
}


static int spl_spi_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
	struct mtd_info *mtd;
	int err = 0;
	bool has_nand = false;
	struct mtd_info *check_mtd;
	__maybe_unused int load_others_res = -1;

	mtd_probe_devices();
	mtd_for_each_device(check_mtd) {
		if (mtd_type_is_nand(check_mtd)) {
			has_nand = true;
			pr_info("Found NAND device: %s\n", check_mtd->name);
			break;
		}
	}

	if (has_nand) {
		/* NAND and NOR Flash devices may share identical partition names,
		   Spl may erroneously boot from NOR rather than the intended NAND Flash
		   partitions
		*/
		pr_info("NAND device detected, removing NOR and partitions...\n");
		remove_nor_partitions();
	}

#ifdef CONFIG_SYS_LOAD_IMAGE_SEC_PARTITION_NAME
	mtd = get_mtd_device_nm(CONFIG_SYS_LOAD_IMAGE_SEC_PARTITION_NAME);
	if (IS_ERR_OR_NULL(mtd)){
		debug("MTD device %s not found\n", CONFIG_SYS_LOAD_IMAGE_SEC_PARTITION_NAME);
		return -1;
	}
	load_others_res = mtd_load_image(spl_image, bootdev, mtd);
#endif

	mtd = get_mtd_device_nm(CONFIG_SYS_LOAD_IMAGE_PARTITION_NAME);
	if (IS_ERR_OR_NULL(mtd)){
		debug("MTD device %s not found\n", CONFIG_SYS_LOAD_IMAGE_PARTITION_NAME);
		return -1;
	}
	err = mtd_load_image(spl_image, bootdev, mtd);

	if (!err || !load_others_res)
		return 0;
	else
		return -1;
}

/* Use priorty 1 so that boards can override this */
SPL_LOAD_IMAGE_METHOD("MTD-NOR", 0, BOOT_DEVICE_NOR, spl_spi_load_image);
SPL_LOAD_IMAGE_METHOD("MTD-NAND", 0, BOOT_DEVICE_NAND, spl_spi_load_image);
