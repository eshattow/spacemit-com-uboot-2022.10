// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#include <config.h>
#include <common.h>
#include <blk.h>
#include <fb_mtd.h>
#include <fastboot.h>
#include <image-sparse.h>
#include <linux/mtd/mtd.h>
#include <fb_spacemit.h>
#include <fastboot-internal.h>


static int fb_mtd_lookup(const char *partname,
			  struct mtd_info **mtd,
			  struct part_info **part,
			  char *response)
{
	struct mtd_device *dev;
	int ret;
	u8 pnum;

	ret = mtdparts_init();
	if (ret) {
		pr_err("Cannot initialize MTD partitions\n");
		fastboot_fail("cannot init mtdparts", response);
		return ret;
	}

	ret = find_dev_and_part(partname, &dev, &pnum, part);
	if (ret) {
		pr_err("cannot find partition: '%s'", partname);
		fastboot_fail("cannot find partition", response);
		return ret;
	}

	printf("dev->id->type:%d\n", dev->id->type);

	return 0;
}

static int _fb_mtd_erase(struct mtd_info *mtd, struct part_info *part)
{

	printf("........ erased mtd part\n");

	return 0;
}

static int _fb_mtd_write(struct mtd_info *mtd, struct part_info *part,
			  void *buffer, u32 offset,
			  size_t length, size_t *written)
{
	return 0;
}


/**
 * fastboot_mtd_get_part_info() - Lookup MTD partion by name
 *
 * @part_name: Named device to lookup
 * @part_info: Pointer to returned part_info pointer
 * @response: Pointer to fastboot response buffer
 */
int fastboot_mtd_get_part_info(const char *part_name,
				struct part_info **part_info, char *response)
{
	struct mtd_info *mtd = NULL;

	return fb_mtd_lookup(part_name, &mtd, part_info, response);
}

/**
 * fastboot_mtd_flash_write() - Write image to MTD for fastboot
 *
 * @cmd: Named device to write image to
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 * @response: Pointer to fastboot response buffer
 */
void fastboot_mtd_flash_write(const char *cmd, void *download_buffer,
			       u32 download_bytes, char *response)
{
	struct part_info *part;
	struct mtd_info *mtd = NULL;
	static u32 __maybe_unused start_offset = 0;
	int ret;
	struct flash_dev *fdev;

	if (!strncmp(cmd, "mtd", 3)){
		fastboot_oem_flash_gpt(cmd, fastboot_buf_addr, download_bytes,
						response, fdev);

		return;
	}

	ret = fb_mtd_lookup(cmd, &mtd, &part, response);
	if (ret) {
		pr_err("invalid NAND device");
		fastboot_fail("invalid NAND device", response);
		return;
	}

	if (is_sparse_image(download_buffer)) {
		printf("not support writing sparse image at mtd flash\n");
		fastboot_fail("not support sparse image", response);
		return;
	} else {
		printf("Flashing raw image at offset \n");

		ret = _fb_mtd_write(mtd, part, download_buffer, part->offset,
				     download_bytes, NULL);

		printf("........ wrote %u bytes to '%s'\n",
		       download_bytes, cmd);
	}

	if (ret) {
		fastboot_fail("error writing the image", response);
		return;
	}

	fastboot_okay(NULL, response);
}

/**
 * fastboot_mtd_flash_erase() - Erase MTD for fastboot
 *
 * @cmd: Named device to erase
 * @response: Pointer to fastboot response buffer
 */
void fastboot_mtd_flash_erase(const char *cmd, char *response)
{
	struct part_info *part;
	struct mtd_info *mtd = NULL;
	int ret;

	ret = fb_mtd_lookup(cmd, &mtd, &part, response);
	if (ret) {
		pr_err("invalid NAND device");
		fastboot_fail("invalid NAND device", response);
		return;
	}

	ret = _fb_mtd_erase(mtd, part);
	if (ret) {
		pr_err("failed erasing from device %s", mtd->name);
		fastboot_fail("failed erasing from device", response);
		return;
	}

	fastboot_okay(NULL, response);
}
