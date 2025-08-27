// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 Spacemit, Inc
 */

#include <common.h>
#include <spl.h>
#include <image.h>
#include <errno.h>

static int spl_ufs_load_image(struct spl_image_info *spl_image,
				 struct spl_boot_device *bootdev)
{
	printf("[SPL][UFS] Placeholder loader invoked (boot_device=%u). Not implemented yet.\n",
	       bootdev ? bootdev->boot_device : 0);
	return -ENODEV;
}

/* Just placeholder */
SPL_LOAD_IMAGE_METHOD("ufs", 9, BOOT_DEVICE_UFS, spl_ufs_load_image);
