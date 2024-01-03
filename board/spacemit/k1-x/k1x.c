// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#include <common.h>
#include <dm.h>
#include <dm/ofnode.h>
#include <dm/lists.h>
#include <env.h>
#include <fdtdec.h>
#include <image.h>
#include <log.h>
#include <mapmem.h>
#include <spl.h>
#include <init.h>
#include <virtio_types.h>
#include <virtio.h>
#include <asm/io.h>
#include <asm/sections.h>
#include <stdlib.h>
#include <linux/io.h>
#include <asm/global_data.h>
#include <part.h>
#include <env.h>
#include <env_internal.h>
#include <asm/arch/ddr.h>
#include <power/regulator.h>
#include <fb_spacemit.h>
#include <net.h>
#include <i2c.h>
#include <linux/delay.h>
#include <tlv_eeprom.h>

DECLARE_GLOBAL_DATA_PTR;

void set_boot_mode(enum board_boot_mode boot_mode)
{
	writel(boot_mode, (void *)BOOT_DEV_FLAG_REG);
}

enum board_boot_mode get_boot_pin_select(void)
{
	/*if not set boot mode, try to return boot pin select*/
	u32 boot_select = readl((void *)BOOT_PIN_SELECT) & BOOT_STRAP_BIT_STORAGE_MASK;
	boot_select = boot_select >> BOOT_STRAP_BIT_OFFSET;
	debug("boot_select:%x\n", boot_select);

	/*select spl boot device:
 
	     b'(bit1)(bit0)
	emmc:b'00, //BOOT_STRAP_BIT_EMMC
	nor :b'10, //BOOT_STRAP_BIT_NOR
	nand:b'01, //BOOT_STRAP_BIT_NAND
	sd  :b'11, //BOOT_STRAP_BIT_SD
*/
	switch (boot_select) {
	case BOOT_STRAP_BIT_EMMC:
		return BOOT_MODE_EMMC;
	case BOOT_STRAP_BIT_NAND:
		return BOOT_MODE_NAND;
	case BOOT_STRAP_BIT_NOR:
		return BOOT_MODE_NOR;
	case BOOT_STRAP_BIT_SD:
	default:
		return BOOT_MODE_SD;
	}
}

enum board_boot_mode get_boot_mode(void)
{
	/*if usb boot or has set boot mode, return boot mode*/
	u32 boot_mode = readl((void *)BOOT_DEV_FLAG_REG);
	debug("%s, boot_mode:%x\n", __func__, boot_mode);

	switch (boot_mode) {
	case BOOT_MODE_USB:
		return BOOT_MODE_USB;
	case BOOT_MODE_EMMC:
		return BOOT_MODE_EMMC;
	case BOOT_MODE_NAND:
		return BOOT_MODE_NAND;
	case BOOT_MODE_NOR:
		return BOOT_MODE_NOR;
	case BOOT_MODE_SD:
		return BOOT_MODE_SD;
	}

	/*else return boot pin select*/
	return get_boot_pin_select();
}


int mmc_get_env_dev(void)
{
	u32 boot_mode = 0;
	boot_mode = get_boot_mode();
	debug("%s, uboot boot_mode:%x\n", __func__, boot_mode);

	if (boot_mode == BOOT_MODE_EMMC)
		return MMC_DEV_EMMC;
	else
		return MMC_DEV_SD;
}


void run_fastboot_command(void)
{
	u32 boot_mode = get_boot_mode();

	/*if define BOOT_MODE_USB flag in BOOT_CIU_DEBUG_REG0, it would excute fastboot*/
	u32 cui_flasg = readl((void *)BOOT_CIU_DEBUG_REG0);
	if (boot_mode == BOOT_MODE_USB || cui_flasg == BOOT_MODE_USB){
		/*would reset debug_reg0*/
		writel(0, (void *)BOOT_CIU_DEBUG_REG0);

		char *cmd_para = "fastboot 0";
		run_command(cmd_para, 0);
	}
}

void import_env_from_bootfs(void)
{
#ifdef CONFIG_MMC
	/*
	TODO:
		load env from bootfs, if bootfs is fat/ext4 at blk dev, use fatload/ext4load.
	*/
	int err, dev;
	u32 part;
	char cmd[128];
	struct mmc *mmc;
	struct disk_partition info;

	dev = mmc_get_env_dev();
	mmc = find_mmc_device(dev);
	if (!mmc) {
		printf("Cannot find mmc device\n");
		return;
	}
	if (mmc_init(mmc)){
		return;
	}

	for (part = 1; part <= MAX_SEARCH_PARTITIONS; part++) {
		err = part_get_info(mmc_get_blk_desc(mmc), part, &info);
		if (err)
			continue;
		if (!strcmp(BOOTFS_NAME, info.name)){
			debug("match info.name:%s\n", info.name);
			break;
		}
	}
	if (part > MAX_SEARCH_PARTITIONS)
		return;

	env_set("bootfs_part", simple_itoa(part));

	/*load env.txt and import to uboot*/
	sprintf(cmd, "fatload mmc %d:%d 0x%x env_%s.txt",
			dev, part, CONFIG_SPL_LOAD_FIT_ADDRESS, CONFIG_SYS_CONFIG_NAME);
	debug("cmd:%s\n", cmd);
	if (run_command(cmd, 0))
		return;

	memset(cmd, '\0', 128);
	sprintf(cmd, "env import -t 0x%x", CONFIG_SPL_LOAD_FIT_ADDRESS);
	debug("cmd:%s\n", cmd);
	if (!run_command(cmd, 0))
		printf("load env%s.txt from bootfs successful\n", CONFIG_SYS_CONFIG_NAME);
#endif
	return;
}

void run_cardfirmware_flash_command(void)
{
	struct mmc *mmc;
	struct disk_partition info;
	int part_dev, err;
	char cmd[128] = {"\0"};

#ifdef CONFIG_MMC
	mmc = find_mmc_device(MMC_DEV_SD);
	if (!mmc)
		return;
	if (mmc_init(mmc))
		return;

	for (part_dev = 1; part_dev <= MAX_SEARCH_PARTITIONS; part_dev++) {
		err = part_get_info(mmc_get_blk_desc(mmc), part_dev, &info);
		if (err)
			continue;
		if (!strcmp(BOOTFS_NAME, info.name))
			break;

	}

	if (part_dev > MAX_SEARCH_PARTITIONS)
		return;

	/*check json file exist or not in sd card*/
	sprintf(cmd, "fatsize mmc %d:%d %s", MMC_DEV_SD, part_dev, CARD_FLASH_FILE);
	debug("cmd:%s\n", cmd);
	if (!run_command(cmd, 0))
		run_command("spacemit_flashing mmc", 0);
#endif
	return;
}

void setenv_boot_mode(void)
{
	u32 boot_mode = get_boot_mode();
	switch (boot_mode) {
	case BOOT_MODE_NAND:
		env_set("boot_device", "nand");
		break;
	case BOOT_MODE_NOR:
		env_set("boot_device", "nor");
		break;
	case BOOT_MODE_EMMC:
		env_set("boot_device", "mmc");
		env_set("boot_devnum", simple_itoa(MMC_DEV_EMMC));
		break;
	case BOOT_MODE_SD:
		env_set("boot_device", "mmc");
		env_set("boot_devnum", simple_itoa(MMC_DEV_SD));
		break;
	default:
		env_set("boot_device", "");
		break;
	}
}

void set_env_ethaddr(void)
{
	int ret = 0, ethaddr_valid = 0, eth1addr_valid = 0;
	uint8_t mac_addr[6], mac1_addr[6];
	char cmd_str[128] = {0};

	/* get mac address from eeprom */
	ret = mac_read_from_eeprom();
	if (ret < 0) {
		printf("read mac address from eeprom failed!\n");
		return ;
	}

	/* check ethaddr valid */
	ethaddr_valid = eth_env_get_enetaddr("ethaddr", mac_addr);
	eth1addr_valid = eth_env_get_enetaddr("eth1addr", mac1_addr);
	if (ethaddr_valid && eth1addr_valid) {
		printf("valid ethaddr: %02x:%02x:%02x:%02x:%02x:%02x\n",
			mac_addr[0], mac_addr[1], mac_addr[2],
			mac_addr[3], mac_addr[4], mac_addr[5]);
		return ;
	}

	/*create random ethaddr*/
	net_random_ethaddr(mac_addr);
	mac_addr[0] = 0xfe;
	mac_addr[1] = 0xfe;
	mac_addr[2] = 0xfe;

	memcpy(mac1_addr, mac_addr, sizeof(mac1_addr));
	mac1_addr[5] = mac_addr[5] + 1;

	/* write to env ethaddr and eth1addr */
	eth_env_set_enetaddr("ethaddr", mac_addr);
	eth_env_set_enetaddr("eth1addr", mac1_addr);

	/* save mac address to eeprom */
	snprintf(cmd_str, (sizeof(cmd_str) - 1), "tlv_eeprom set 0x24 %02x:%02x:%02x:%02x:%02x:%02x", \
			mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	run_command(cmd_str, 0);

	memset(cmd_str, 0, sizeof(cmd_str));
	snprintf(cmd_str, (sizeof(cmd_str) - 1), "tlv_eeprom set 0x2A 2");
	run_command(cmd_str, 0);

	memset(cmd_str, 0, sizeof(cmd_str));
	snprintf(cmd_str, (sizeof(cmd_str) - 1), "tlv_eeprom write");
	run_command(cmd_str, 0);
}

void set_dev_serial_no(void)
{
	u8 eeprom_data[256], sn[6] = {0};
	char cmd_str[128] = {0};
	struct tlvinfo_header *tlv_hdr = NULL;
	struct tlvinfo_tlv *tlv_entry;
	unsigned int tlv_offset, tlv_len;
	int ret = 0, i = 0, sn_found = 0;
	unsigned int seed = 0;

	ret = read_tlvinfo_tlv_eeprom(eeprom_data, &tlv_hdr,
					      &tlv_entry, 0);
	if (ret < 0) {
		printf("read tlvinfo from eeprom failed!\n");
		return;
	}
	tlv_offset = sizeof(struct tlvinfo_header);
	tlv_len = sizeof(struct tlvinfo_header) + be16_to_cpu(tlv_hdr->totallen);
	while (tlv_offset < tlv_len) {
		tlv_entry = (struct tlvinfo_tlv *)&eeprom_data[tlv_offset];

		printf("tlv entry type = 0x%x\n", tlv_entry->type);
		if (tlv_entry->type == TLV_CODE_SERIAL_NUMBER) {
			printf("Serial number found\n");
			sn_found = 1;
			break;
		}

		tlv_offset += sizeof(struct tlvinfo_tlv) + tlv_entry->length;
	}

	if (sn_found && tlv_entry->length == 12) {
			if (tlv_entry->value[0] | tlv_entry->value[1] | tlv_entry->value[2] |
				tlv_entry->value[3] | tlv_entry->value[4] | tlv_entry->value[5] |
				tlv_entry->value[6] | tlv_entry->value[7] | tlv_entry->value[8] |
				tlv_entry->value[9] | tlv_entry->value[10] | tlv_entry->value[11]) {
				printf("Serial number is valid.\n");
				return ;
			}
	}

	printf("Generate rand serial number:\n");
	/* Generate rand serial number */
	seed = get_ticks();
	for (i = 0; i < 6; i++) {
		sn[i] = rand_r(&seed);
		printf("%02x", sn[i]);
	}
	printf("\n");

	/* save serial number to eeprom */
	snprintf(cmd_str, (sizeof(cmd_str) - 1), "tlv_eeprom set 0x23 %02x%02x%02x%02x%02x%02x", \
			sn[0], sn[1], sn[2], sn[3], sn[4], sn[5]);
	run_command(cmd_str, 0);

	memset(cmd_str, 0, sizeof(cmd_str));
	snprintf(cmd_str, (sizeof(cmd_str) - 1), "tlv_eeprom write");
	run_command(cmd_str, 0);
}

int board_init(void)
{
#ifdef CONFIG_DM_REGULATOR_SPM8XX
	int ret;

	ret = regulators_enable_boot_on(true);
	if (ret)
		debug("%s: Cannot enable boot on regulator\n", __func__);
#endif
	return 0;
}

int board_late_init(void)
{
	ulong kernel_start;
	ofnode chosen_node;
	int ret;

	if (IS_ENABLED(CONFIG_SYSRESET_SPACEMIT))
		device_bind_driver(gd->dm_root, "spacemit_sysreset",
					"spacemit_sysreset", NULL);

	run_fastboot_command();

	run_cardfirmware_flash_command();

	/*import env.txt from bootfs*/
	import_env_from_bootfs();

	setenv_boot_mode();

	set_env_ethaddr();
	set_dev_serial_no();

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


void board_boot_order(u32 *spl_boot_list)
{
	u32 boot_mode = get_boot_mode();
	debug("boot_mode:%x\n", boot_mode);
	if (boot_mode == BOOT_MODE_USB){
		spl_boot_list[0] = BOOT_DEVICE_BOARD;
	}else{
		spl_boot_list[0] = BOOT_DEVICE_MMC1;
		if (boot_mode != BOOT_MODE_SD){
			switch (boot_mode) {
			case BOOT_MODE_EMMC:
				spl_boot_list[1] = BOOT_DEVICE_MMC2;
				break;
			case BOOT_MODE_NAND:
				spl_boot_list[1] = BOOT_DEVICE_NAND;
				break;
			case BOOT_MODE_NOR:
				spl_boot_list[1] = BOOT_DEVICE_NOR;
				break;
			default:
				spl_boot_list[1] = BOOT_DEVICE_RAM;
				break;
			}

			//reserve for fpga to load/run uboot from ram.
			spl_boot_list[2] = BOOT_DEVICE_RAM;
		}else{
			//reserve for fpga to load/run uboot from ram.
			spl_boot_list[1] = BOOT_DEVICE_RAM;
		}
	}
}


enum env_location env_get_location(enum env_operation op, int prio)
{
	if (prio >= 1)
		return ENVL_UNKNOWN;

	u32 boot_mode = get_boot_mode();
	switch (boot_mode) {
#ifdef CONFIG_ENV_IS_IN_NAND
	case BOOT_MODE_NAND:
		return ENVL_NAND;
#endif
#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
	case BOOT_MODE_NOR:
		return ENVL_SPI_FLASH;
#endif
#ifdef CONFIG_ENV_IS_IN_MMC
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
		return ENVL_MMC;
#endif
	default:
#ifdef CONFIG_ENV_IS_NOWHERE
		return ENVL_NOWHERE;
#else
		return ENVL_UNKNOWN;
#endif
	}
}

int misc_init_r(void)
{
#ifdef CONFIG_DYNAMIC_DDR_CLK_FREQ
	int ret;

	ret = ddr_freq_max();
	if(ret < 0) {
		debug("%s: Try to adjust ddr freq failed!\n", __func__);
		return ret;
	}
#endif

	return 0;
}

int dram_init(void)
{
	u64 dram_size = (u64)ddr_get_density() * SZ_1MB;

	gd->ram_base = CONFIG_SYS_SDRAM_BASE;

	if(dram_size > SZ_2GB) {
		gd->ram_size = SZ_2GB;
	} else {
		gd->ram_size = dram_size;
	}

	return 0;
}

int dram_init_banksize(void)
{
	u64 dram_size = (u64)ddr_get_density() * SZ_1MB;

	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	if(dram_size > SZ_2GB) {
		gd->bd->bi_dram[0].size = SZ_2G;
		gd->bd->bi_dram[1].start = 0x100000000;
		gd->bd->bi_dram[1].size = dram_size - SZ_2G;
	} else {
		gd->bd->bi_dram[0].size = dram_size;
		gd->bd->bi_dram[1].start = 0;
		gd->bd->bi_dram[1].size = 0;
	}

	return 0;
}

ulong board_get_usable_ram_top(ulong total_size)
{
	u64 dram_size = (u64)ddr_get_density() * SZ_1MB;

        /* Some devices (like the EMAC) have a 32-bit DMA limit. */
	if(dram_size > SZ_2GB) {
		return 0x80000000;
	} else {
		return dram_size;
	}
}

