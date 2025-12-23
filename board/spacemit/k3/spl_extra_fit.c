// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 Spacemit, Inc
 * Board extra FIT loader: load additional ITB files from MTD partitions
 */

#include <common.h>
#include <image.h>
#include <log.h>
#include <spl.h>
#include <asm/global_data.h>
#include <cpu_func.h>
#include <mtd.h>
#include <linux/err.h>
#include <env.h>
#include <mapmem.h>
#include <linux/libfdt.h>
#include <string.h>
#include <part.h>
#include <fs.h>
#include <asm/io.h>

#ifdef CONFIG_SPL_MMC
#include <mmc.h>
#include <blk.h>
#endif

enum board_boot_mode get_boot_mode(void);

#ifdef CONFIG_SPL_MMC
/* ================= MMC support ================= */
static ulong spl_extra_mmc_read(struct spl_load_info *load, ulong sector, ulong count, void *buf)
{
	struct blk_desc *bd = load->dev;
	ulong n;

	pr_debug("%s: sector %lx, count %lx, buf %lx\n", __func__, sector, count, (ulong) buf);
	n = blk_dread(bd, sector, count, buf);
	return n == count ? count : 0;
}

static int extra_mmc_load_image(struct spl_image_info *spl_image, struct blk_desc *bd,
				ulong start_lba)
{
	struct image_header *header;
	int err = 0;
	ulong hdr_cnt = DIV_ROUND_UP(sizeof(*header), bd->blksz);

	header = spl_get_load_buffer(-sizeof(*header), sizeof(*header));
	if (blk_dread(bd, start_lba, hdr_cnt, header) != hdr_cnt) {
		pr_err("MMC: read header failed\n");
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) && image_get_magic(header) == FDT_MAGIC) {
		struct spl_load_info load;
		pr_debug("MMC: found FIT at LBA 0x%lx\n", start_lba);
		load.dev = bd;
		load.priv = NULL;
		load.filename = NULL;
		/* bl_len must be the block size in bytes for FIT loader */
		load.bl_len = bd->blksz;
		load.read = spl_extra_mmc_read;
		err = spl_load_simple_fit(spl_image, &load, start_lba, header);
	} else {
		pr_err("MMC: unsupported legacy image\n");
		return -EINVAL;
	}
	return err;
}

/* Helper to get an MMC block device based on boot mode */
static struct blk_desc *get_default_mmc_blk_desc(void)
{
	struct mmc *mmc;
	struct blk_desc *bd;
	int mmc_dev_index;
	enum board_boot_mode boot_mode;

	/* Get current boot mode to determine MMC device index */
	boot_mode = get_boot_mode();

	/* Map boot mode to MMC device index */
	switch (boot_mode) {
	case BOOT_MODE_SD:
		mmc_dev_index = 0; /* SD card is typically MMC device 1 */
		break;
	case BOOT_MODE_EMMC:
		mmc_dev_index = 1; /* eMMC is typically MMC device 0 */
		break;
	default:
		mmc_dev_index = 0;
		break;
	}

	/* Use only the boot mode specific device */
	mmc = find_mmc_device(mmc_dev_index);
	if (mmc) {
		if (!mmc_init(mmc)) {
			bd = mmc_get_blk_desc(mmc);
			if (bd) {
				pr_debug("MMC: using device %d based on boot mode 0x%x\n",
					 mmc_dev_index, boot_mode);
				return bd;
			}
		}
	}

	pr_err("MMC: device %d not available for boot mode 0x%x\n", mmc_dev_index, boot_mode);
	return NULL;
}

/* Unified MMC loader using built-in defaults
 * image: "esos" or "uboot"; selects default LBA by compile-time macros
 * out_entry: only meaningful when image == "uboot"
 */
static int load_fit_from_mmc(struct spl_image_info *caller_spl_image, const char *image,
			     ulong *out_entry)
{
	int ret = -1;
	ulong lba = 0;
	struct blk_desc *bd = get_default_mmc_blk_desc();
	struct spl_image_info temp = { 0 };

	if (!bd)
		return -ENODEV;

	/* Strategy:
	 * 1) If 'image' is decimal digits -> treat as raw LBA
	 * 2) Else, treat 'image' as partition name -> scan partitions and use info.start
	 * 3) Fallback to legacy compile-time LBA macros (uboot/esos)
	 */

	if (image && *image) {
		bool all_digit = true;
		const char *p = image;
		while (*p) {
			if (*p < '0' || *p > '9') {
				all_digit = false;
				break;
			}
			p++;
		}

		if (all_digit) {
			/* numeric string -> raw LBA */
			lba = simple_strtoul(image, NULL, 10);
			pr_debug("MMC: interpret '%s' as LBA=%lu\n", image, lba);
			pr_debug("MMC: load FIT @ LBA 0x%lx (numeric)\n", lba);
			ret = extra_mmc_load_image(&temp, bd, lba);
		} else {
			/* treat as partition name and scan */
			struct disk_partition info;
			int found = 0;
			for (int part = 1; part <= MAX_SEARCH_PARTITIONS; part++) {
				int pe = part_get_info(bd, part, &info);
				if (pe)
					continue;
				pr_debug("MMC: scan p=%d name=%s start=%lu size=%lu\n", part,
					 info.name, info.start, info.size);
				if (!strcmp(image, info.name)) {
					lba = info.start;
					found = 1;
					break;
				}
			}
			if (found) {
				pr_debug("MMC: found partition '%s', start LBA=0x%lx\n", image,
					 lba);
				ret = extra_mmc_load_image(&temp, bd, lba);
			} else {
				pr_debug("MMC: partition '%s' not found, will fallback to default "
					 "LBA\n",
					 image);
			}
		}
	}

	/* No compile-time fallback: if neither numeric LBA nor partition name matched,
	 * keep ret as error and let caller handle it.
	 */

	pr_debug("extra_mmc_load_image ret:%d\n", ret);
	if (!ret) {
		if (temp.fdt_addr) {
			caller_spl_image->fdt_addr = temp.fdt_addr;
			pr_debug("Set DTB from loaded FIT @ %p\n", temp.fdt_addr);
		}
		if (out_entry && image && !strcmp(image, "uboot")) {
			if (temp.entry_point)
				*out_entry = temp.entry_point;
			else
				*out_entry = CONFIG_SYS_TEXT_BASE;
			pr_debug("Loaded entry: 0x%lx\n", *out_entry);
		}
	}
	return ret;
}
#endif /* CONFIG_SPL_MMC */

#ifdef CONFIG_SPL_MTD_LOAD
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

int spl_extra_mtd_read(struct mtd_info *mtd, ulong sector, ulong count, void *buf)
{
	bool read = true, raw = false, woob = false, has_pages = false;
	u64 start_off, off, len, remaining;
	struct mtd_oob_ops io_op = {};
	u32 npages;
	int ret = -1;

	u8 *buffer = map_sysmem((u64) buf, 0);
	if (!buffer)
		return -1;

	pr_debug("mtd_read sector:%lx, count:%lx, buffer:%lx\n", sector, count, (ulong) buffer);
	start_off = sector;
	if (!mtd_is_aligned_with_min_io_size(mtd, start_off)) {
		pr_err("Offset not aligned with a page (0x%x)\n", mtd->writesize);
		return ret;
	}

	len = count;
	if (!mtd_is_aligned_with_min_io_size(mtd, len)) {
		len = round_up(len, mtd->writesize);
		pr_debug("Size not on a page boundary (0x%x), rounding to 0x%llx\n", mtd->writesize,
			 len);
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

	/* Loop over the pages to do the actual read */
	while (remaining) {
		/* Skip the block if it is bad */
		if (mtd_is_aligned_with_block_size(mtd, off) && mtd_block_isbad(mtd, off)) {
			off += mtd->erasesize;
			continue;
		}

		ret = mtd_read_oob(mtd, off, &io_op);
		if (ret) {
			pr_err("Failure while %s at offset 0x%llx\n", read ? "reading" : "writing",
			       off);
			break;
		}

		off += io_op.retlen;
		remaining -= io_op.retlen;
		io_op.datbuf += io_op.retlen;
		io_op.oobbuf += io_op.oobretlen;
	}
	return ret;
}

static ulong spl_extra_load_read(struct spl_load_info *load, ulong sector, ulong count, void *buf)
{
	int ret;

	pr_debug("%s: sector %lx, count %lx, buf %lx\n", __func__, sector, count, (ulong) buf);

	struct mtd_info *mtd = load->dev;
	pr_debug("%s, get mtd:%p\n", __func__, mtd);
	ret = spl_extra_mtd_read(mtd, sector, count, buf);
	if (!ret)
		return count;
	else
		return 0;
}

static int extra_mtd_load_image(struct spl_image_info *spl_image, struct spl_boot_device *bootdev,
				struct mtd_info *mtd)
{
	struct image_header *header;
	ulong len;
	int err = 0;
	len = sizeof(*header);
	if (!mtd_is_aligned_with_min_io_size(mtd, len)) {
		len = round_up(len, mtd->writesize);
		pr_debug("Size not on a page boundary (0x%x), rounding to 0x%lx\n", mtd->writesize,
			 len);
	}

	header = spl_get_load_buffer(-sizeof(*header), sizeof(*header));
	err = spl_extra_mtd_read(mtd, 0, len, (void *) header);
	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) && image_get_magic(header) == FDT_MAGIC) {
		struct spl_load_info load;

		pr_debug("MTD: found FIT\n");
		load.dev = mtd;
		load.priv = NULL;
		load.filename = NULL;
		load.bl_len = 1;
		load.read = spl_extra_load_read;

		err = spl_load_simple_fit(spl_image, &load, 0, header);
	} else {
		pr_err("MTD: unsupported legacy image\n");
		return -1;
	}

	return err;
}

/* Unified MTD loader using partition name directly
 * part: partition name, typically "esos" or "uboot"
 * out_entry: only meaningful when part == "uboot"
 */
static int load_fit_from_mtd(struct spl_image_info *caller_spl_image, const char *part,
			     ulong *out_entry)
{
	struct mtd_info *mtd;
	int ret = -1;
	struct spl_image_info temp = { 0 };

	/* Directly use input parameter as partition name */

	pr_debug("MTD: load FIT from partition '%s'\n", part);
	mtd = get_mtd_device_nm(part);
	if (IS_ERR_OR_NULL(mtd)) {
		pr_err("MTD device %s not found\n", part);
		return -ENODEV;
	}

	pr_debug("MTD %s: mtd:%p erasesize:0x%x writesize:0x%x type:%d\n", part, mtd,
		 mtd->erasesize, mtd->writesize, mtd->type);
	ret = extra_mtd_load_image(&temp, NULL, mtd);
	pr_debug("extra_mtd_load_image ret:%d\n", ret);
	if (!ret) {
		if (temp.fdt_addr) {
			caller_spl_image->fdt_addr = temp.fdt_addr;
			pr_debug("Set DTB from loaded FIT @ %p\n", temp.fdt_addr);
		}
		if (out_entry && part && !strcmp(part, "uboot") && temp.entry_point) {
			*out_entry = temp.entry_point;
			pr_debug("Loaded entry: 0x%lx\n", *out_entry);
		}
	}
	return ret;
}
#endif /* CONFIG_SPL_MTD_LOAD */

#if defined(CONFIG_SYS_BOOTLOADER_FS_PARTITION_NAME)
/**
 * Load firmware from bootloader file system partition
 * Loads u-boot-opensbi.itb or esos.itb FIT images, same as legacy mode
 *
 * @image:      SPL image info structure
 * @image_path: Image path and name.
 * @return: 0 on success, negative on error
 */
static int load_image_from_mmc_blfs(struct spl_image_info *image, const char *image_path)
{
	struct blk_desc *bd;
	struct disk_partition info;
	int ret = -1, part;
	const char *blfs_name;

	/* Get mmc block device */
	bd = get_default_mmc_blk_desc();
	if (!bd) {
		pr_err("BLFS: failed to get block device\n");
		return -ENODEV;
	}

	blfs_name = CONFIG_SYS_BOOTLOADER_FS_PARTITION_NAME;
	part = part_get_info_by_name(bd, blfs_name, &info);
	if (part < 0) {
		pr_err("Partition %s NOT exist\n", blfs_name);
		return -ENOENT;
	}

	memset(image, 0, sizeof(struct spl_image_info));
#ifdef CONFIG_SPL_FS_FAT
	// first try in FAT
	ret = spl_load_image_fat(image, NULL, bd, part, image_path);
#endif
#ifdef CONFIG_SPL_FS_EXT4
	// then try in EXT4
	if (ret)
		ret = spl_load_image_ext(image, NULL, bd, part, image_path);
#endif
	if (ret) {
		pr_err("BLFS: image(%s) load failed (non-fatal)\n", image_path);
	}

	return ret;
}
#endif /* CONFIG_SPL_FS_FAT || CONFIG_SPL_FS_EXT4 */

int board_load_extra_fits(struct spl_image_info *spl_image, ulong *uboot_entry)
{
	/* Use board_boot_order to infer primary boot device */
	extern void board_boot_order(u32 * spl_boot_list);
	u32 spl_boot_list[2] = { 0 };
	int load_esos_res = -1, load_uboot_res = -1;

	board_boot_order(spl_boot_list);

	switch (spl_boot_list[0]) {
#ifdef CONFIG_SPL_MMC
	case BOOT_DEVICE_MMC1:
	case BOOT_DEVICE_MMC2:
	case BOOT_DEVICE_MMC2_2: {
#if defined(CONFIG_SYS_BOOTLOADER_FS_PARTITION_NAME)
		const char *blfs_mode_str;
		int blfs_load_mode = 0;
		bool blfs_load_failed = false;
		const char *uboot_itb_path, *esos_itb_path;
		struct spl_image_info image;

		/* Check if bootloader file system load mode is enabled, default to be enabled */
		blfs_mode_str = env_get("bootloader_from_fs");
		if (blfs_mode_str)
			blfs_load_mode = simple_strtol(blfs_mode_str, NULL, 10);
		else
			blfs_load_mode = 1;

		if (blfs_load_mode) {
			/* Get environment variables for file paths */
			/* Use blfs-specific env vars for file names (not partition names) */
			esos_itb_path = env_get("esos_itb_path");
			uboot_itb_path = env_get("uboot_itb_path");

			/* Fallback to hardcoded paths if env_get fails or returns wrong value */
			if (!uboot_itb_path) {
				uboot_itb_path = "u-boot.itb";
			}
			if (!esos_itb_path || !strcmp(esos_itb_path, uboot_itb_path)) {
				esos_itb_path = "esos.itb";
			}

			/* load firmware from bootloader file system, MUST load uboot at the last */
			if ((0 == load_image_from_mmc_blfs(&image, esos_itb_path)) &&
				(0 == load_image_from_mmc_blfs(&image, uboot_itb_path))) {
				load_esos_res = 0;
				load_uboot_res = 0;

				/* Copy DTB address to caller's spl_image (shared between opensbi and uboot) */
				if (image.fdt_addr)
					spl_image->fdt_addr = image.fdt_addr;

				/* Extract U-Boot entry point for opensbi to jump to */
				if (uboot_entry && image.entry_point)
					*uboot_entry = image.entry_point;
			} else {
				blfs_load_failed = true;
			}
		}

		/* Fallback to legacy mode if bootloader file system disabled or failed */
		if (blfs_load_failed)
#endif /* CONFIG_SYS_BOOTLOADER_FS_PARTITION_NAME */
		{
			/* Legacy mode: load FIT images from partitions */
			const char *tmp;
			char *part_esos = NULL, *part_uboot = NULL;
			pr_debug("MMC: using legacy FIT load mode\n");
			tmp = env_get("extra_esos_partition");
			if (tmp)
				part_esos = strdup(tmp);
			tmp = env_get("extra_uboot_partition");
			if (tmp)
				part_uboot = strdup(tmp);

			if (part_esos && *part_esos) {
				load_esos_res = load_fit_from_mmc(spl_image, part_esos, NULL);
			} else {
				pr_debug("extra_esos_partition not set, skip MMC esos\n");
			}
			if (part_uboot && *part_uboot) {
				load_uboot_res =
					load_fit_from_mmc(spl_image, part_uboot, uboot_entry);
			} else {
				pr_debug("extra_uboot_partition not set, skip MMC uboot\n");
			}

			if (part_esos)
				free(part_esos);
			if (part_uboot)
				free(part_uboot);
		}
		break;
	}
#endif
#ifdef CONFIG_SPL_MTD_LOAD
	case BOOT_DEVICE_NOR:
	case BOOT_DEVICE_NAND: {
		const char *tmp;
		char *part_esos = NULL, *part_uboot = NULL;
		tmp = env_get("extra_esos_partition");
		if (tmp)
			part_esos = strdup(tmp);
		tmp = env_get("extra_uboot_partition");
		if (tmp)
			part_uboot = strdup(tmp);

		mtd_probe_devices();
		if (part_esos && *part_esos) {
			load_esos_res = load_fit_from_mtd(
				spl_image, strcmp(part_esos, "1") == 0 ? "esos" : part_esos, NULL);
		} else {
			pr_debug("extra_esos_partition not set, skip MTD esos\n");
		}
		if (part_uboot && *part_uboot) {
			load_uboot_res = load_fit_from_mtd(
				spl_image, strcmp(part_uboot, "1") == 0 ? "uboot" : part_uboot,
				uboot_entry);
		} else {
			pr_debug("extra_uboot_partition not set, skip MTD uboot\n");
		}

		if (part_esos)
			free(part_esos);
		if (part_uboot)
			free(part_uboot);
		break;
	}
#endif
	default:
		pr_info("unknown boot device: %u\n", spl_boot_list[0]);
		break;
	}

	if (!load_uboot_res || !load_esos_res) {
		pr_info("load result: uboot=%d esos=%d\n", load_uboot_res, load_esos_res);
		return 0;
	} else {
		pr_err("load failed: uboot=%d esos=%d\n", load_uboot_res, load_esos_res);
		return -1;
	}
}