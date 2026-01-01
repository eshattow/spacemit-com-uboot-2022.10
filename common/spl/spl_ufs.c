// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 Spacemit, Inc
 *
 * SPL UFS support - derived from spl_sata.c
 */

#include <common.h>
#include <spl.h>
#include <asm/u-boot.h>
#include <ufs.h>
#include <scsi.h>
#include <errno.h>
#include <fat.h>
#include <image.h>
#include <part.h>

#ifndef CONFIG_SPL_UFS_RAW_U_BOOT_SECTOR
/* Default sector to load U-Boot from UFS */
#define CONFIG_SPL_UFS_RAW_U_BOOT_SECTOR 0x1000
#endif

static int spl_ufs_load_image_raw(struct spl_image_info *spl_image,
		struct spl_boot_device *bootdev,
		struct blk_desc *stor_dev, unsigned long sector)
{
	struct image_header *header;
	unsigned long count;
	u32 image_size_sectors;
	u32 image_offset_sectors;
	u32 image_offset;
	int ret;

	header = spl_get_load_buffer(-sizeof(*header), stor_dev->blksz);

	count = blk_dread(stor_dev, sector, 1, header);
	if (count == 0) {
		pr_err("SPL UFS: Failed to read header from sector %lu\n", sector);
		return -EIO;
	}

	ret = spl_parse_image_header(spl_image, bootdev, header);
	if (ret)
		return ret;

	image_size_sectors = DIV_ROUND_UP(spl_image->size, stor_dev->blksz);
	image_offset_sectors = spl_image->offset / stor_dev->blksz;
	image_offset = spl_image->offset % stor_dev->blksz;

	count = blk_dread(stor_dev, sector + image_offset_sectors,
			image_size_sectors,
			(void *)spl_image->load_addr);
	if (count != image_size_sectors) {
		pr_err("SPL UFS: Read error: expected %u, got %lu sectors\n",
		       image_size_sectors, count);
		return -EIO;
	}

	if (image_offset)
		memmove((void *)spl_image->load_addr,
			(void *)spl_image->load_addr + image_offset,
			spl_image->size);

	return 0;
}

#ifdef CONFIG_SYS_LOAD_IMAGE_PARTITION_NAME
/**
 * spl_ufs_load_image_partition - Load image from a named partition
 *
 * This function looks up a partition by name and attempts to load the
 * boot image from it using FAT filesystem or raw image loading.
 */
static int spl_ufs_load_image_partition(struct spl_image_info *spl_image,
					struct spl_boot_device *bootdev,
					struct blk_desc *stor_dev)
{
	int err = -ENOSYS;
	int partition;
	struct disk_partition info;

	/* Find partition by name */
	partition = part_get_info_by_name(stor_dev,
			CONFIG_SYS_LOAD_IMAGE_PARTITION_NAME, &info);
	if (partition < 0)
		return -ENOENT;

#if IS_ENABLED(CONFIG_SPL_FS_FAT)
	err = spl_load_image_fat(spl_image, bootdev, stor_dev,
				 partition,
				 CONFIG_SPL_FS_LOAD_PAYLOAD_NAME);
	if (!err)
		return 0;
#endif

	/* Try raw load from partition start */
	err = spl_ufs_load_image_raw(spl_image, bootdev, stor_dev, info.start);
	return err;
}
#endif /* CONFIG_SYS_LOAD_IMAGE_PARTITION_NAME */

static int spl_ufs_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
	int err = 0;
	struct blk_desc *stor_dev;

	/* Probe UFS devices first */
	err = ufs_probe();
	if (err)
		return err;

	/* Scan for SCSI devices (UFS creates SCSI devices) */
	scsi_scan(false);

	/* Get the first SCSI block device */
	stor_dev = blk_get_devnum_by_type(IF_TYPE_SCSI, 0);
	if (!stor_dev)
		return -ENODEV;

#if CONFIG_IS_ENABLED(OS_BOOT)
	if (spl_start_uboot() ||
	    spl_load_image_fat_os(spl_image, bootdev, stor_dev, 1))
#endif
	{
		err = -ENOSYS;

#ifdef CONFIG_SYS_LOAD_IMAGE_PARTITION_NAME
		/* First try to load from named partition */
		err = spl_ufs_load_image_partition(spl_image, bootdev, stor_dev);
		if (!err)
			goto done;
#endif

#if IS_ENABLED(CONFIG_SPL_FS_FAT)
		/* Fallback: try FAT from partition 1 */
		err = spl_load_image_fat(spl_image, bootdev, stor_dev,
				1,
				CONFIG_SPL_FS_LOAD_PAYLOAD_NAME);
		if (!err)
			goto done;
#endif

#if IS_ENABLED(CONFIG_SPL_UFS_RAW_U_BOOT_USE_SECTOR)
		/* Fallback: try raw sector load */
		err = spl_ufs_load_image_raw(spl_image, bootdev, stor_dev,
			CONFIG_SPL_UFS_RAW_U_BOOT_SECTOR);
		if (!err)
			goto done;
#endif
	}

done:
	if (err)
		pr_err("SPL UFS: All load methods failed, err=%d\n", err);

	return err;
}

SPL_LOAD_IMAGE_METHOD("UFS", 0, BOOT_DEVICE_UFS, spl_ufs_load_image);
