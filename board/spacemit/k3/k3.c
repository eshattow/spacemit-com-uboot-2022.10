// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025, Spacemit
 */

#include <common.h>
#include <dm.h>
#include <dm/ofnode.h>
#include <env.h>
#include <fdtdec.h>
#include <g_dnl.h>
#include <image.h>
#include <log.h>
#include <mapmem.h>
#include <spl.h>
#include <init.h>
#include <virtio_types.h>
#include <virtio.h>
#include <asm/io.h>
#include <asm/sections.h>
#include <power/regulator.h>
#include <fb_spacemit.h>
#include <misc.h>
#include <net.h>
#include <tlv_eeprom.h>
#include "nfs_env.h"

bool is_video_connected = false;
static char found_partition[64] = {0};

DECLARE_GLOBAL_DATA_PTR;

void import_env_from_bootfs(void);
void setenv_boot_mode(void);
void set_env_ethaddr(void);
void refresh_config_info(void);

extern int get_tlvinfo(uint8_t id, uint8_t *buffer, int max_size);
extern int set_tlvinfo(int tcode, char* val);
extern int flush_tlvinfo(void);
extern int update_tlvinfo(void);

#if CONFIG_IS_ENABLED(SPACEMIT_K1X_EFUSE)
int get_chipid_from_efuse(uint64_t *chipid)
{
	struct udevice *dev;
	uint8_t fuses[9];
	int ret;

	if (NULL == chipid)
		return EACCES;

	/* retrieve the device */
	ret = uclass_get_device_by_driver(UCLASS_MISC,
			DM_DRIVER_GET(spacemit_k1x_efuse), &dev);
	if (ret) {
		return ENODEV;
	}

	// read from efuse, each bank has 32byte efuse data
	// chipid in bank7 bit191~251
	ret = misc_read(dev, 7 * 32 + 23, fuses, sizeof(fuses));
	if (0 == ret) {
		// 1. get bit 192~251
		// 2. left shift 1bit, and merge with efuse_bank[7].bit191
		*chipid = 0;
		memcpy(chipid, &fuses[1], 8);
		*chipid <<= 1;
		*chipid |= (fuses[0] & 0x80) >> 7;
		pr_debug("Get chipid %llx\n", *chipid);
	}

	return ret;
}

int get_dro_from_efuse(uint32_t *dro)
{
	struct udevice *dev;
	uint8_t fuses[2];
	int ret;

	if (NULL == dro)
		return EACCES;

	*dro = SVT_DRO_DEFAULT_VALUE;

	/* retrieve the device */
	ret = uclass_get_device_by_driver(UCLASS_MISC,
			DM_DRIVER_GET(spacemit_k1x_efuse), &dev);
	if (ret) {
		return ENODEV;
	}

	// read from efuse, each bank has 32byte efuse data
	// SVT-DRO in bank7 bit173~bit181
	ret = misc_read(dev, 7 * 32 + 21, fuses, sizeof(fuses));
	if (0 == ret) {
		// (byte1 bit0~bit5) << 3 | (byte0 bit5~7) >> 5
		*dro = (fuses[0] >> 5) & 0x07;
		*dro |= (fuses[1] & 0x3F) << 3;
	}

	return 0;
}

int get_chipinfo_from_efuse(uint32_t *product_id, uint32_t *wafer_tid)
{
	struct udevice *dev;
	uint8_t fuses[3];
	int ret;

	if ((NULL == product_id) || (NULL == wafer_tid))
		return EACCES;

	*product_id = 0;
	*wafer_tid = 0;

	/* retrieve the device */
	ret = uclass_get_device_by_driver(UCLASS_MISC,
			DM_DRIVER_GET(spacemit_k1x_efuse), &dev);
	if (ret) {
		return ENODEV;
	}

	// read from efuse, each bank has 32byte efuse data
	// product id in bank7 bit182~bit190
	ret = misc_read(dev, 7 * 32 + 22, fuses, sizeof(fuses));
	if (0 == ret) {
		// (byte1 bit0~bit6) << 2 | (byte0 bit6~7) >> 6
		*product_id = (fuses[0] >> 6) & 0x03;
		*product_id |= (fuses[1] & 0x7F) << 2;
	}

	// read from efuse, each bank has 32byte efuse data
	// product id in bank7 bit139~bit154
	ret = misc_read(dev, 7 * 32 + 17, fuses, sizeof(fuses));
	if (0 == ret) {
		// (byte1 bit0~bit6) << 2 | (byte0 bit3~7) >> 3
		*wafer_tid = (fuses[0] >> 3) & 0x1F;
		*wafer_tid |= fuses[1] << 5;
		*wafer_tid |= (fuses[2] & 0x07) << 13;
	}

	return ret;
}
#endif

void update_usb_serial_number(void)
{
	uint64_t chipid;
	uint32_t i, seed;
	int ret = -1;
	char temp[20];

#if CONFIG_IS_ENABLED(SPACEMIT_K1X_EFUSE)
	ret = get_chipid_from_efuse(&chipid);
#endif

	if (0 != ret) {
		seed = get_ticks();
		for (i = 0; i < sizeof(chipid); i++) {
			((uint8_t*)&chipid)[i] = rand_r(&seed);
		}
	}

	snprintf(temp, sizeof(temp), "%016llx", chipid);

#ifdef CONFIG_USB_SET_SERIAL_NUMBER
	g_dnl_set_serialnumber(temp);
#endif
}

int board_init(void)
{
	int ret = 0;

#ifdef CONFIG_DM_REGULATOR_SPM8XX
	ret = regulators_enable_boot_on(true);
	if (ret)
		pr_debug("%s: Cannot enable boot on regulator\n", __func__);
#endif

#ifdef CONFIG_ESPI
	ret = uclass_probe_all(UCLASS_ESPI);
	if (ret) {
		pr_err("eSPI: Probe failed (ret=%d)\n", ret);
		return CMD_RET_FAILURE;
	}
#endif

	return ret;
}

void run_fastboot_command(void)
{
	if (BOOT_MODE_USB == get_boot_mode()) {
		/* show flash log*/
		env_set("stdout", env_get("stdout_flash"));

		update_usb_serial_number();

		char *cmd_para = "fastboot 0";
		run_command(cmd_para, 0);

		// it may update tlv during USB fastboot stage
		refresh_config_info();
	}
}

void try_flash_image_from_card(void)
{
#ifdef CONFIG_MMC
	struct mmc *mmc;
	struct disk_partition info;
	int part;
	char cmd[128] = {"\0"};

	mmc = find_mmc_device(MMC_DEV_SD);
	if ((NULL == mmc) || (0 != mmc_init(mmc)))
		return;

	part = part_get_info_by_name(mmc_get_blk_desc(mmc), BOOTFS_NAME, &info);
	if (part < 0) {
		pr_err("NO partition %s in card\n", BOOTFS_NAME);
		return;
	}

	/*check if flash config file is in sd card*/
	sprintf(cmd, "fatsize mmc %d:%d %s", MMC_DEV_SD, part, FLASH_CONFIG_FILE_NAME);
	pr_debug("cmd: %s\n", cmd);
	if (0 != run_command(cmd, 0)) {
		pr_err("Can NOT find partition table %s in card\n", FLASH_CONFIG_FILE_NAME);
		return;
	}

	/* show flash log*/
	env_set("stdout", env_get("stdout_flash"));
	run_command("flash_image mmc", 0);
#endif
	return;
}

int board_late_init(void)
{
	ulong kernel_start;
	ofnode chosen_node;
	int ret;

	set_env_ethaddr();
	refresh_config_info();

	run_fastboot_command();

	if (BOOT_MODE_SD == get_boot_mode()) {
		try_flash_image_from_card();
	}

	import_env_from_bootfs();

	setenv_boot_mode();

	chosen_node = ofnode_path("/chosen");
	if (!ofnode_valid(chosen_node)) {
		debug("No chosen node found, can't get kernel start address\n");
		return 0;
	}

	ret = ofnode_read_u64(chosen_node, "riscv,kernel-start",
			      (u64 *)&kernel_start);
	if (ret) {
		debug("Can't find kernel start address in device tree\n");
		return 0;
	}

	env_set_hex("kernel_start", kernel_start);

	return 0;
}

int misc_init_r(void)
{
	return 0;
}
extern u32 ddr_get_density(void);

int dram_init(void)
{
#if CONFIG_K3_BOARD_FPGA
	u64 dram_size = SZ_2GB;
#else
	u64 dram_size = (u64)ddr_get_density() * SZ_1MB;
#endif
	// initial 32MB of memory is invisible, reserved for openSBI and esos.
	gd->ram_base = CONFIG_SYS_SDRAM_BASE;
	gd->ram_size = dram_size - SEC_IMG_SIZE;

	return 0;
}

int dram_init_banksize(void)
{
	memset(gd->bd->bi_dram, 0, sizeof(gd->bd->bi_dram));
	gd->bd->bi_dram[0].start = gd->ram_base;
	gd->bd->bi_dram[0].size = gd->ram_size;

	return 0;
}

ulong board_get_usable_ram_top(ulong total_size)
{
	return gd->ram_base + gd->ram_size;
}

void *board_fdt_blob_setup(int *err)
{
	*err = 0;

	/* Stored the DTB address there during our init */
	if (IS_ENABLED(CONFIG_OF_SEPARATE) || IS_ENABLED(CONFIG_OF_BOARD)) {
		if (gd->arch.firmware_fdt_addr){
			if (!fdt_check_header((void *)(ulong)gd->arch.firmware_fdt_addr)){
				return (void *)(ulong)gd->arch.firmware_fdt_addr;
			}
		}
	}
	return (ulong *)&_end;
}


/******************************************************************************
 * Boot mode support function
 *******************************************************************************/
/*
 * Boot mode strap pins (4 bits total)
 *
 * bit3 bit2 bit1-0  Meaning
 *  1     0     XX   Update from USB
 *  1     1     XX   Update from UART
 *  0     X     00   Normal boot from eMMC
 *  0     X     10   Normal boot from SPI Nand
 *  0     X     01   Normal boot from SPI Nor
 *  0     X     11   Normal boot from UFS
 */
enum board_boot_mode get_boot_pin_select(void)
{
	/* Decode full 4-bit strap field starting at BOOT_STRAP_BIT_OFFSET */
	u32 strap = readl((void *)BOOT_PIN_SELECT);
	u32 pins = (strap >> BOOT_STRAP_BIT_OFFSET) & 0xF; /* bit3:bit0 */
	pr_debug("strap_pins:%#x\n", pins);

	/* boot storage only use bit1-0 */
	switch (pins & 0x3) {
	case BOOT_STRAP_BIT_EMMC: /* 00 */
		return BOOT_MODE_EMMC;
	case BOOT_STRAP_BIT_NOR:  /* 01 */
		return BOOT_MODE_NOR;
	case BOOT_STRAP_BIT_NAND: /* 10 */
		return BOOT_MODE_NAND;
	case BOOT_STRAP_BIT_UFS:  /* 11 */
		return BOOT_MODE_UFS;
	default:
		return BOOT_MODE_SD;
    }
}

/* Get boot mode based on bootrom implementation */
enum board_boot_mode get_boot_mode(void)
{
	u32 boot_flag_reg;
	enum board_boot_mode mode = BOOT_MODE_USB;

	/* Read bootrom boot flag from BOOT_DEV_FLAG_REG */
	boot_flag_reg = readl((void*)BOOT_DEV_FLAG_REG);
	pr_debug("%s boot_flag_reg:0x%x\n", __func__, boot_flag_reg);

	/* Check bootrom set boot type */
	u32 boot_type = boot_flag_reg & BOOT_TYPE_MASK;
	switch (boot_type) {
	case BOOT_MODE_USB:
		mode = BOOT_MODE_USB;
		break;
	case BOOT_MODE_UART:
		mode = BOOT_MODE_UART;
		break;
	case BOOT_MODE_EMMC:
		mode = BOOT_MODE_EMMC;
		break;
	case BOOT_MODE_NAND:
		mode = BOOT_MODE_NAND;
		break;
	case BOOT_MODE_NOR:
		mode = BOOT_MODE_NOR;
		break;
	case BOOT_MODE_UFS:
		mode = BOOT_MODE_UFS;
		break;
	case BOOT_MODE_SD:
		mode = BOOT_MODE_SD;
		break;
	default:
		mode = BOOT_MODE_SHELL;  /* Default to shell if unknown */
		break;
	}

	pr_debug("Final boot mode: 0x%x\n", mode);
	return mode;
}

void setenv_boot_mode(void)
{
#ifdef CONFIG_ENV_IS_IN_NFS
	const char *boot_override = env_get("boot_override");

	if (boot_override) {
		env_set("boot_device", boot_override);
		env_set("boot_override", NULL);
		return;
	}
#endif

	u32 boot_mode = get_boot_mode();
	switch (boot_mode) {
	case BOOT_MODE_NAND:
		env_set("boot_device", "nand");
		break;
	case BOOT_MODE_NOR:
		char *blk_name;
		int blk_index;

		if (get_available_boot_blk_dev(&blk_name, &blk_index)){
			pr_err("can not get available blk dev\n");
			return;
		}

		env_set("boot_device", "nor");
		env_set("boot_devnum", simple_itoa(blk_index));
		break;
	case BOOT_MODE_EMMC:
		env_set("boot_device", "mmc");
		env_set("boot_devnum", simple_itoa(MMC_DEV_EMMC));
		break;
	case BOOT_MODE_SD:
		env_set("boot_device", "mmc");
		env_set("boot_devnum", simple_itoa(MMC_DEV_SD));
		break;
	case BOOT_MODE_USB:
		// for fastboot image download and run test
		env_set("bootcmd", CONFIG_BOOTCOMMAND);
		break;
	default:
		env_set("boot_device", "");
		break;
	}
}

/******************************************************************************
 * Load environment support function
 *******************************************************************************/
int mmc_get_env_dev(void)
{
	u32 boot_mode = 0;
	boot_mode = get_boot_mode();
	pr_debug("%s, uboot boot_mode:%x\n", __func__, boot_mode);

	if (boot_mode == BOOT_MODE_EMMC)
		return MMC_DEV_EMMC;
	else
		return MMC_DEV_SD;
}

void _load_env_from_blk(struct blk_desc *dev_desc, const char *dev_name, int dev)
{
	int err;
	u32 part;
	char cmd[128];
	struct disk_partition info;

	for (part = 1; part <= MAX_SEARCH_PARTITIONS; part++) {
		err = part_get_info(dev_desc, part, &info);
		if (err)
			continue;
		if (!strcmp(BOOTFS_NAME, info.name)){
			pr_debug("match info.name:%s\n", info.name);
			break;
		}
	}
	if (part > MAX_SEARCH_PARTITIONS)
		return;

	env_set("bootfs_part", simple_itoa(part));
	env_set("bootfs_devname", dev_name);

	/*load env.txt and import to uboot*/
	memset((void *)CONFIG_FASTBOOT_BUF_ADDR, 0, CONFIG_ENV_SIZE);
	sprintf(cmd, "load %s %d:%d 0x%lx env_%s.txt", dev_name,
			dev, part, CONFIG_FASTBOOT_BUF_ADDR, CONFIG_SYS_CONFIG_NAME);
	pr_debug("cmd:%s\n", cmd);
	if (run_command(cmd, 0))
		return;

	memset(cmd, '\0', 128);
	sprintf(cmd, "env import -t 0x%lx", CONFIG_FASTBOOT_BUF_ADDR);
	pr_debug("cmd:%s\n", cmd);
	if (!run_command(cmd, 0)){
		pr_info("load env_%s.txt from bootfs successful\n", CONFIG_SYS_CONFIG_NAME);
	}
}

char* parse_mtdparts_and_find_bootfs(void) {
	const char *mtdparts = env_get("mtdparts");
	char cmd_buf[256];

	if (!mtdparts) {
		pr_debug("mtdparts not set\n");
		return NULL;
	}

	/* Find the last partition */
	const char *last_part_start = strrchr(mtdparts, '(');
	if (last_part_start) {
		last_part_start++; /* Skip the left parenthesis */
		const char *end = strchr(last_part_start, ')');
		if (end && (end - last_part_start < sizeof(found_partition))) {
			int len = end - last_part_start;
			strncpy(found_partition, last_part_start, len);
			found_partition[len] = '\0';

			snprintf(cmd_buf, sizeof(cmd_buf), "ubi part %s", found_partition);
			if (run_command(cmd_buf, 0) == 0) {
				/* Check if the bootfs volume exists */
				snprintf(cmd_buf, sizeof(cmd_buf), "ubi check %s", BOOTFS_NAME);
				if (run_command(cmd_buf, 0) == 0) {
					pr_info("Found bootfs in partition: %s\n", found_partition);
					return found_partition;
				}
			}
		}
	}

	pr_debug("bootfs not found in any partition\n");
	return NULL;
}

/* Load environment variables from NAND bootfs partition */
static int load_env_from_nand_bootfs(void)
{
#if CONFIG_IS_ENABLED(ENV_IS_IN_MTD)
	/*load env from nand bootfs*/
	const char *bootfs_name = BOOTFS_NAME ;
	char cmd[128];

	if (!bootfs_name) {
		pr_err("bootfs not set\n");
		return -1;
	}

	/* Parse mtdparts to find the partition containing the BOOTFS_NAME volume */
	char *mtd_partition   = parse_mtdparts_and_find_bootfs();
	if (!mtd_partition  ) {
		pr_err("Bootfs not found in any partition\n");
		return -1;
	}

	sprintf(cmd, "ubifsmount ubi0:%s", bootfs_name);
	if (run_command(cmd, 0)) {
		pr_err("Cannot mount ubifs partition '%s'\n", bootfs_name);
		return -1;
	}

	memset((void *)CONFIG_FASTBOOT_BUF_ADDR, 0, CONFIG_ENV_SIZE);
	sprintf(cmd, "ubifsload 0x%lx env_%s.txt", CONFIG_FASTBOOT_BUF_ADDR, CONFIG_SYS_CONFIG_NAME);
	if (run_command(cmd, 0)) {
		pr_err("Failed to load env_%s.txt from bootfs\n", CONFIG_SYS_CONFIG_NAME);
		return -1;
	}

	memset(cmd, '\0', 128);
	sprintf(cmd, "env import -t 0x%lx", CONFIG_FASTBOOT_BUF_ADDR);
	if (!run_command(cmd, 0)) {
		pr_debug("Imported environment from 'env_%s.txt'\n", CONFIG_SYS_CONFIG_NAME);
		return 0;
	}

	return -1;
#else
	pr_debug("ENV_IS_IN_MTD not enabled, skipping NAND bootfs env load\n");
	return 0;
#endif
}

void import_env_from_bootfs(void)
{
	u32 boot_mode = get_boot_mode();

#ifdef CONFIG_RSA_VERIFY
	/*
	 * In secure boot mode, do not load environment from external flash
	 * to prevent unauthorized environment modification
	 */
	pr_info("Secure boot enabled, skip loading environment from bootfs\n");
	return;
#endif

#ifdef CONFIG_ENV_IS_IN_NFS
	// Check if local bootfs exists
	if ((BOOT_MODE_USB != boot_mode) && check_bootfs_exists() != 0) {
		#ifdef CONFIG_CMD_NET
			eth_initialize();
		#endif
		// Local bootfs not found, try to load from NFS
		if (load_env_from_nfs() == 0) {
			return;
		}
	}
#endif

	switch (boot_mode) {
	case BOOT_MODE_NAND:
		load_env_from_nand_bootfs();
		break;
	case BOOT_MODE_NOR:
		struct blk_desc *dev_desc;
		char *blk_name;
		int blk_index;

		if (get_available_boot_blk_dev(&blk_name, &blk_index)){
			pr_err("can not get available blk dev\n");
			return;
		}

		dev_desc = blk_get_dev(blk_name, blk_index);
		if (dev_desc)
			_load_env_from_blk(dev_desc, blk_name, blk_index);
		break;
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
#ifdef CONFIG_MMC
		int dev;
		struct mmc *mmc;

		dev = mmc_get_env_dev();
		mmc = find_mmc_device(dev);
		if (!mmc) {
			pr_err("Cannot find mmc device\n");
			return;
		}
		if (mmc_init(mmc)){
			return;
		}

		_load_env_from_blk(mmc_get_blk_desc(mmc), "mmc", dev);
		break;
#endif
	default:
		break;
	}
	return;
}

static void increase_eth_addr(uint8_t *mac_addr)
{
	mac_addr[5]++;
	if (0 == mac_addr[5]) {
		mac_addr[4]++;
		if (0 == mac_addr[4]) {
			mac_addr[3]++;
		}
	}
}

int read_mac_from_tlv(void)
{
	unsigned int i;
	uint32_t mac_size;
	u8 macbase[6];
	int maccount;

	maccount = 1;
	if (get_tlvinfo(TLV_CODE_MAC_SIZE, (char*)&mac_size, 2) > 0) {
		maccount = be16_to_cpu(mac_size);
	}

	if ((get_tlvinfo(TLV_CODE_MAC_BASE, (char*)macbase, 6) <= 0)
		|| !is_valid_ethaddr(macbase)) {
		return 0;
	}

	for (i = 0; i < maccount; i++) {
		char ethaddr[18];
		char enetvar[11];

		sprintf(ethaddr, "%02X:%02X:%02X:%02X:%02X:%02X",
			macbase[0], macbase[1], macbase[2],
			macbase[3], macbase[4], macbase[5]);
		sprintf(enetvar, i ? "eth%daddr" : "ethaddr", i);
		/* Only initialize environment variables that are blank
			* (i.e. have not yet been set)
			*/
		if (!env_get(enetvar))
			env_set(enetvar, ethaddr);

		increase_eth_addr(macbase);
	}

	return maccount;
}

void set_env_ethaddr(void)
{
	uint8_t mac_addr[6];
	char mac_str[32];
	int i, maccount;

	/* Determine source of MAC address and attempt to read it */
	maccount = read_mac_from_tlv();
	if (maccount > 0) {
		pr_info("Found %d valid MAC addresses.\n", maccount);
		return;
	}

	/*if there is NO valid MAC address, create 4 random ethaddr */
	maccount = 4;
	pr_info("generate %d random ethaddr.\n", maccount);
	net_random_ethaddr(mac_addr);
	mac_addr[0] = 0xfe;
	mac_addr[1] = 0xfe;
	mac_addr[2] = 0xfe;

	/* save mac address to TLV */
	snprintf(mac_str, (sizeof(mac_str) - 1), "%02x:%02x:%02x:%02x:%02x:%02x",
		mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	set_tlvinfo(TLV_CODE_MAC_BASE, mac_str);
	sprintf(mac_str, "%d", maccount);
	set_tlvinfo(TLV_CODE_MAC_SIZE, mac_str);
	flush_tlvinfo();

	/* write ethaddr to env */
	eth_env_set_enetaddr("ethaddr", mac_addr);
	for (i = 1; i < maccount; i++) {
		/* Increase the last byte of MAC address for each additional interface */
		increase_eth_addr(mac_addr);
		sprintf(mac_str, "eth%daddr", i);
		pr_info("Update env %s with mac address: %02x:%02x:%02x:%02x:%02x:%02x\n", mac_str,
			mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
		eth_env_set_enetaddr(mac_str, mac_addr);
	}
}

void refresh_config_info(void)
{
	char *strval = malloc(64);
	int i, num, ret;

	const struct code_desc_info {
		u8    m_code;
		u8    is_data;
		char *m_name;
	} info[] = {
		{ TLV_CODE_PRODUCT_NAME,   false, "product_name"},
		{ TLV_CODE_PART_NUMBER,    false, "part#"},
		{ TLV_CODE_SERIAL_NUMBER,  false, "serial#"},
		{ TLV_CODE_MANUF_DATE,     false, "manufacture_date"},
		{ TLV_CODE_MANUF_NAME,     false, "manufacturer"},
		{ TLV_CODE_WIFI_MAC_ADDR,  false, "wifi_addr"},
		{ TLV_CODE_BLUETOOTH_ADDR, false, "bt_addr"},
		{ TLV_CODE_DEVICE_VERSION, true,  "device_version"},
		{ TLV_CODE_SDK_VERSION,    true,  "sdk_version"},
		{ TLV_CODE_DDR_DATARATE,   true,  "ddr_datarate"},
		{ TLV_CODE_DDR_PARTNUMBER, false,  "ddr_partnumber"},
	};

	for (i = 0; i < ARRAY_SIZE(info); i++) {
		ret = get_tlvinfo(info[i].m_code, strval, 64 - 1);
		if (ret <= 0) {
			continue;
		}

		if (info[i].is_data) {
			num = 0;
			// Convert the numeric value to string
			for (int j = 0; j < ret && j < sizeof(num); j++) {
				num = (num << 8) | strval[j];
			}
			sprintf(strval, "%d", num);
		} else {
			strval[ret] = '\0';
		}
		pr_info("TLV item: %s = %s\n", info[i].m_name, strval);
		env_set(info[i].m_name, strval);
	}

	free(strval);
}

#if !defined(CONFIG_SPL_BUILD)
int board_fit_config_name_match(const char *name)
{
	char *product_name = env_get("product_name");

	if (NULL == product_name) {
		product_name = DEFAULT_PRODUCT_NAME;
	}

	if (0 == strcmp(product_name, name)) {
		log_info("Boot from fit configuration %s\n", name);
		return 0;
	}
	else
		return -1;
}
#endif
