/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2023,  chris.huang<chris.huang@spacemit.com>
 */

#ifndef _FB_SPACEMIT_H_
#define _FB_SPACEMIT_H_


/**
 * fastboot_oem_flash_gpt() - parse flash config and write gpt table.
 *
 * @cmd: Named partition to write image to
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 * @response: Pointer to fastboot response buffer
 */
void fastboot_oem_flash_gpt(const char *cmd, void *download_buffer, u32 download_bytes,
							char *response, u32 *start_offset);

/**
 * fastboot_mmc_flash_fsbl() - Write fsbl image to eMMC
 *
 * @start_offset: start offset to write.
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 */
int fastboot_mmc_flash_fsbl(u32 start_offset, void *download_buffer, u32 download_bytes);


/**
 * @brief check image crc at emmc. if crc is same it would return RESULT_OK(0).
 * 
 * @param dev_desc struct blk_desc.
 * @param crc_compare need to be compare crc.
 * @param part_start_cnt read from emmc offset.
 * @param blksz normally is 0x200.
 * @param image_size 
 * @return int 
 */
int check_mmc_image_crc(struct blk_desc *dev_desc, ulong crc_compare, lbaint_t part_start_cnt,
			ulong blksz, int image_size);

#endif
