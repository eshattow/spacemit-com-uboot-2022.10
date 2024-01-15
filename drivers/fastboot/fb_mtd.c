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
#include <image.h>
#include <linux/mtd/mtd.h>
#include <linux/compat.h>
#include <android_image.h>
#include <fb_spacemit.h>
#include <fastboot-internal.h>
#include <u-boot/crc.h>
#include <mapmem.h>
#include <mtd.h>

struct fb_mtd_sparse {
	struct mtd_info		*mtd;
	struct part_info	*part;
};

static bool mtd_is_aligned_with_min_io_size(struct mtd_info *mtd, u64 size)
{
	return !do_div(size, mtd->writesize);
}

static bool mtd_is_aligned_with_block_size(struct mtd_info *mtd, u64 size)
{
	return !do_div(size, mtd->erasesize);
}

int fb_mtd_lookup(const char *partname,
			  struct mtd_info **mtd,
			  struct part_info **part)
{
	struct mtd_info *mtd_info;
	int ret;
	u8 pnum;

	mtd_probe_devices();

	struct mtd_device *dev;

	ret = mtdparts_init();
	if (ret) {
		printf("Cannot initialize MTD partitions\n");
		return -1;
	}

	ret = find_dev_and_part(partname, &dev, &pnum, part);
	if (ret) {
		printf("cannot find partition: '%s'\n", partname);
		return -1;
	}

	mtd_info = get_mtd_device_nm(partname);
	if(mtd_info == NULL)
		printf("get mtd info is NULL\n");
	if(*part == NULL)
		printf("get mtd info is NULL\n");

	if (IS_ERR_OR_NULL(mtd_info)){
		printf("MTD device %s not found\n", partname);
		*mtd = NULL;
		return -1;
	}else{
		*mtd = mtd_info;
		return 0;
	}
}

int _fb_mtd_erase(struct mtd_info *mtd, struct part_info *part)
{
	bool scrub;
	u64 len;
	struct erase_info erase_op = {};
	int ret = 0;

	if (IS_ERR_OR_NULL(mtd))
		return -1;

	printf("........ erased mtd part\n");
	len = mtd->size;
	scrub = false;

	erase_op.mtd = mtd;
	erase_op.addr = 0;
	erase_op.len = mtd->erasesize;
	erase_op.scrub = scrub;

	while (len) {
		ret = mtd_erase(mtd, &erase_op);
		if (ret) {
			/* Abort if its not a bad block error */
			if (ret != -EIO)
				break;
			printf("Skipping bad block at 0x%08llx\n",
			       erase_op.addr);
		}

		len -= mtd->erasesize;
		erase_op.addr += mtd->erasesize;
	}

	if (ret && ret != -EIO)
		return -1;
	else
		return 0;
}

/**
 * @brief read or write to mtd devices.
 * 
 * @param mtd 
 * @return return 0 if read/write success.
 */
static int _fb_mtd_rw(struct mtd_info *mtd, ulong sector, ulong count,
						void *buf, bool read)
{
	bool raw, woob, has_pages = false;
	u64 start_off, off, len, remaining;
	struct mtd_oob_ops io_op = {};
	int ret = -1;

	u8 *buffer = map_sysmem((u64)buf, 0);
	if (!buffer)
		return -1;

	start_off = sector;
	if (!mtd_is_aligned_with_min_io_size(mtd, start_off)) {
		printf("Offset not aligned with a page (0x%x)\n",
		       mtd->writesize);
		return ret;
	}

	len = count;
	if (!mtd_is_aligned_with_min_io_size(mtd, len)) {
		len = round_up(len, mtd->writesize);
		printf("Size not on a page boundary (0x%x), rounding to 0x%llx\n",
		       mtd->writesize, len);
	}
	if (mtd->type == MTD_NANDFLASH || mtd->type == MTD_MLCNANDFLASH)
		has_pages = true;

	remaining = len;

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

		if (read)
			ret = mtd_read_oob(mtd, off, &io_op);
		else
			ret = mtd_write_oob(mtd, off, &io_op);

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

	if (ret) {
		printf("%s on %s failed with error %d\n",
		       read ? "Read" : "Write", mtd->name, ret);
		return -1;
	} else {
		return 0;
	}
}


int _fb_mtd_write(struct mtd_info *mtd, struct part_info *part,
			  void *buffer, u32 offset,
			  size_t length, size_t *written)
{
	int ret;

	ret = _fb_mtd_rw(mtd, offset, length, buffer, false);
	if (ret)
		return -1;
	else
		return 0;
}


int _fb_mtd_read(struct mtd_info *mtd, struct part_info *part,
			  void *buffer, u32 offset,
			  size_t length, size_t *written)
{
	int ret;

	ret = _fb_mtd_rw(mtd, offset, length, buffer, true);
	if (ret)
		return -1;
	else
		return 0;
}

static lbaint_t fb_mtd_sparse_write(struct sparse_storage *info,
		lbaint_t blk, lbaint_t blkcnt, const void *buffer)
{
	struct fb_mtd_sparse *sparse = info->priv;
	size_t written;
	int ret;

	ret = _fb_mtd_write(sparse->mtd, sparse->part, (void *)buffer,
			     blk * info->blksz,
			     blkcnt * info->blksz, &written);
	if (ret < 0) {
		printf("Failed to write sparse chunk\n");
		return ret;
	}

	/*
	 * the return value must be 'blkcnt' ("good-blocks") plus the
	 * number of "bad-blocks" encountered within this space...
	 */
	return written / info->blksz;
}

static lbaint_t fb_mtd_sparse_reserve(struct sparse_storage *info,
		lbaint_t blk, lbaint_t blkcnt)
{
	int bad_blocks = 0;
	return blkcnt + bad_blocks;
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

	if(fb_mtd_lookup(part_name, &mtd, part_info)){
		fastboot_fail("can not find mtd part", response);
		return -1;
	}
	else{
		fastboot_okay(NULL, response);
		return 0;
	}
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
	int ret;

#ifdef CONFIG_SPACEMIT_FLASH
	static struct flash_dev *fdev = NULL;
	/*save crc value to compare after flash image*/
	u32 crc_val = 0;
	crc_val = crc32_wd(crc_val, (const uchar *)download_buffer, download_bytes, CHUNKSZ_CRC32);

	if (fdev == NULL){
		fdev = malloc(sizeof(struct flash_dev));
		if (!fdev) {
			printf("Memory allocation failed!\n");
		}
		memset(fdev, 0, sizeof(struct flash_dev));
		fdev->gptinfo.fastboot_flash_gpt = false;
		/*would realloc the size while parsing the partition table*/
		fdev->gptinfo.gpt_table = malloc(10);
		fdev->mtd_table = malloc(10);
		memset(fdev->gptinfo.gpt_table, '\0', 10);
		memset(fdev->mtd_table, '\0', 10);
	}

	if (!strncmp(cmd, "mtd", 3)){
		fastboot_oem_flash_gpt(cmd, fastboot_buf_addr, download_bytes,
						response, fdev);
		return;
	}

	/*flash env*/
	if (strcmp(cmd, "env") == 0) {
		printf("flash env \n");
		fastboot_oem_flash_env(cmd, fastboot_buf_addr, download_bytes,
								response, fdev);
		return;
	}
#endif
	ret = fb_mtd_lookup(cmd, &mtd, &part);
	if (ret) {
		pr_err("invalid mtd device");
		fastboot_fail("invalid mtd device or partition", response);
		return;
	}

	/*must erase at first when write data to mtd devices*/
	ret = _fb_mtd_erase(mtd, part);
	if (ret) {
		printf("failed erasing from device %s", mtd->name);
		fastboot_fail("failed erasing from device", response);
		return;
	}

	if (is_sparse_image(download_buffer)) {
		struct fb_mtd_sparse sparse_priv;
		struct sparse_storage sparse;

		sparse_priv.mtd = mtd;
		sparse_priv.part = part;

		sparse.blksz = mtd->writesize;
		sparse.start = part->offset / sparse.blksz;
		sparse.size = part->size / sparse.blksz;
		sparse.write = fb_mtd_sparse_write;
		sparse.reserve = fb_mtd_sparse_reserve;
		sparse.mssg = fastboot_fail;

		printf("Flashing sparse image at offset %lx\n", sparse.start);
		sparse.priv = &sparse_priv;
		ret = write_sparse_image(&sparse, cmd, download_buffer,
					 response);
		if (!ret)
			fastboot_okay(NULL, response);
	} else {
		printf("Flashing raw image at offset \n");

		ret = _fb_mtd_write(mtd, part, download_buffer, 0,
				     download_bytes, NULL);

		printf("........ wrote %u bytes to '%s'\n",
		       download_bytes, part->name);
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

	ret = fb_mtd_lookup(cmd, &mtd, &part);
	if (ret) {
		printf("invalid mtd device\n");
		fastboot_fail("invalid mtd device or partition", response);
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

/**
 * fastboot_mtd_flash_read() - load data from mtd for fastboot
 *
 * @part_name: Named partition to read
 * @response: Pointer to fastboot response buffer
 */
u32 fastboot_mtd_flash_read(const char *part_name, u32 offset,
					void *download_buffer, char *response)
{
	struct part_info *part;
	struct mtd_info *mtd = NULL;
	int ret;
	u32 hdr_size, hdr_off;

	if (fb_mtd_lookup(part_name, &mtd, &part)) {
		/*can not find mtd part, try to read raw data*/
		if (strncmp("mtd", part_name, 3))
			return 0;

		mtd_for_each_device(mtd) {
			if (!mtd_is_partition(mtd))
				break;
		}

		if (IS_ERR_OR_NULL(mtd)){
			printf("can not find mtd devices\n");
			return 0;
		}

		printf("get mtd name :%s\n", mtd->name);
		if (fb_mtd_lookup(mtd->name, &mtd, &part)){
			printf("can not get the mtd part\n");
			return 0;
		}
	}

	if (offset >= part->size){
		fastboot_response("OKAY", response, "%08x", 0);
		return 0;
	}

	hdr_off = (u32)part->offset + offset;
	hdr_size = (u32)part->size - offset;

	/*if size > buffer_size, it would only load buffer_size, and return offset*/
	if (hdr_size > fastboot_buf_size){
		/* Read the boot image header */
		hdr_size = fastboot_buf_size;
	}

	ret = _fb_mtd_read(mtd, part, download_buffer, hdr_off, hdr_size, NULL);
	if (ret){
		fastboot_fail("cannot read data from mtd dev", response);
		return -1;
	}

	fastboot_response("OKAY", response, "0x%08x", hdr_size);
	return hdr_size;
}
