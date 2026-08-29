// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Spacemit
 */

#include "k3_ddr.h"

static const uint32_t tx_impedance_array[] = { 0x10040, 0x10042, 0x10043, 0x10044, 0x10045 };
static const uint32_t tx_impedance_array1[] = { 0x30040, 0x30041, 0x30042, 0x30043 };
static int add_soc_phy_write_ds_config(ddr_phy_reg_config* reg_table,
	uint32_t max_item, uint32_t phy_write_ds)
{
	int i, j, k;

	for (i = 0, k = 0; i < 4; i++) {
		for (j = 0; (j < ARRAY_SIZE(tx_impedance_array)) && (k < max_item); j++, k++) {
			reg_table[k].offset = tx_impedance_array[j] + i * 0x1000;
			reg_table[k].value = phy_write_ds + (phy_write_ds << 8);
		}
	}
	for (i = 0; i < 2; i++) {
		for (j = 0; (j < ARRAY_SIZE(tx_impedance_array1)) && (k < max_item); j++, k++) {
			reg_table[k].offset = tx_impedance_array1[j] + i * 0x1000;
			reg_table[k].value = phy_write_ds + (phy_write_ds << 8);
		}
	}
	return k;
}

static const uint32_t odt_impedance_array[] = { 0x10048, 0x1004a, 0x1004b, 0x1004c, 0x1004d };
static int add_soc_phy_rx_odt_config(ddr_phy_reg_config* reg_table,
	uint32_t max_item, uint32_t phy_rx_odt)
{
	int i, j, k;
	for (i = 0, k = 0; i < 4; i++) {
		for (j = 0; (j < ARRAY_SIZE(odt_impedance_array)) && (k < max_item); j++, k++) {
			reg_table[k].offset = odt_impedance_array[j] + i * 0x1000;
			reg_table[k].value = phy_rx_odt << 8;
		}
	}

	return k;
}

static int add_ddr_pdds_config(ddr_phy_reg_config* reg_table, uint32_t max_item,
	uint32_t pdds)
{
	uint16_t value;

	if (max_item < 2) {
		pr_err("Must have at least 2 items for ddr pdds config");
		return 0;
	}

	value = pdds | (pdds << 8);
	reg_table[0].offset = 0x58031;
	reg_table[0].value = value;
	reg_table[1].offset = 0x58032;
	reg_table[1].value = value;
	return 2;
}

static int add_ddr_odt_config(ddr_phy_reg_config* reg_table, uint32_t max_item,
	uint32_t dq_odt, uint32_t ca_odt, uint32_t nt_odt)
{
	int enable = nt_odt == R_OFF ? 0 : 1;
	uint16_t value;

	if (max_item < 2) {
		pr_err("Must have at least 2 items for ddr odt config");
		return 0;
	}

	value = dq_odt | (ca_odt << 4) | (enable << 3);
	value |= value << 8;
	reg_table[0].offset = 0x58035;
	reg_table[0].value = value;
	reg_table[1].offset = 0x58036;
	reg_table[1].value = value;

	return 2;
}

static int add_ddr_socodt_config(ddr_phy_reg_config* reg_table, uint32_t max_item,
	uint32_t soc_odt, uint32_t x8_mode)
{
	uint16_t value;

	if (max_item < 2) {
		pr_err("Must have at least 2 items for ddr soc odt config");
		return 0;
	}

	value = BIT(3) | BIT(5);
	value |= soc_odt | (soc_odt << 8);
	value |= (x8_mode << 7) | (x8_mode << 15);
	reg_table[0].offset = 0x58041;
	reg_table[0].value = value;
	reg_table[1].offset = 0x58042;
	reg_table[1].value = value;

	return 2;
}

static int add_ddr_ntodt_config(ddr_phy_reg_config* reg_table, uint32_t max_item,
	uint32_t nt_odt)
{
	uint16_t value;

	if (max_item < 2) {
		pr_err("Must have at least 2 items for ddr ntodt config");
		return 0;
	}

	value = nt_odt << 5;
	value |= value << 8;
	reg_table[0].offset = 0x58065;
	reg_table[0].value = value;
	reg_table[1].offset = 0x58066;
	reg_table[1].value = value;

	return 2;
}

static int add_ddr_2dtraining_config(ddr_phy_reg_config* reg_table, uint32_t max_item,
	uint32_t enable_2d_training)
{
	if (max_item < 1) {
		pr_err("Must have at least 1 item for ddr 2d training config");
		return 0;
	}

	reg_table[0].offset = 0x58067;
	if (0 != enable_2d_training) {
		// 0: enable 2D training; 1: disable 2D training
		reg_table[0].value = 0;
	} else {
		reg_table[0].value = 1;
	}

	return 1;
}

void build_lpddr5_io_para(const ddr_config_t* io_para, ddr_part_info* part_info)
{
	int i = 0;

	memset(io_override_table, 0, sizeof(io_override_table));

	// MUST NOT change function sequence below
	i += add_soc_phy_write_ds_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->phy_write_ds);
	i += add_soc_phy_rx_odt_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->phy_rx_odt);
	i += add_ddr_pdds_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->pdds);
	i += add_ddr_odt_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->dq_odt, io_para->ca_odt, io_para->nt_odt);
	i += add_ddr_socodt_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->soc_odt, part_info->x8_mode);
	i += add_ddr_ntodt_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->nt_odt);
	i += add_ddr_2dtraining_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->enable_2d_training);

	pr_info("build %d ddr io parameters complete\n", i);
}

static void phyinit_lp5_pre_training(uint32_t ddrc_base, ddr_part_info* part_info)
{
	uint32_t offset = 0;
	unsigned long DPHY_BASE = ddrc_base + 0x800000;
	volatile uint32_t* dphy_reg = (volatile uint32_t*)(size_t)DPHY_BASE;
	const ddr_phy_reg_config* override_table = NULL;

	if (1 == part_info->ranks && 0 == part_info->x8_mode) {
		// single rank, x16 DDR
		if (6400 == part_info->data_rate_mtps) {
			override_table = phy_override_pre_seq_lp5_srank_x16_6400;
		} else if (4800 == part_info->data_rate_mtps) {
			override_table = phy_override_pre_seq_lp5_srank_x16_4800;
		}
	} else if (part_info->ranks > 1 && 0 == part_info->x8_mode) {
		// dual rank, x16 DDR
		if (6400 == part_info->data_rate_mtps) {
			override_table = phy_override_pre_seq_lp5_drank_x16_6400;
		} else if (4800 == part_info->data_rate_mtps) {
			override_table = phy_override_pre_seq_lp5_drank_x16_4800;
		}
	} else {
		// dual rank, x8 DDR
		if (4800 == part_info->data_rate_mtps) {
			override_table = phy_override_pre_seq_lp5_drank_x8_4800;
		}
	}

	lpddr_training_table_init(ddrc_base, lp5_pre_train_table, override_table, io_override_table);

	for (offset = 0x584d2; offset < 0x60000; offset++)
		dphy_reg[offset] = 0x0;
}

static void phyinit_lp5_training(uint32_t ddrc_base, ddr_part_info* part_info)
{
	const ddr_phy_reg_config* override_table = NULL;

	if (1 == part_info->ranks && 0 == part_info->x8_mode) {
		// single rank, x16 DDR
		if (6400 == part_info->data_rate_mtps) {
			override_table = phy_override_seq_lp5_srank_x16_6400;
		} else if (4800 == part_info->data_rate_mtps) {
			override_table = phy_override_seq_lp5_srank_x16_4800;
		}
	} else if (part_info->ranks > 1 && 0 == part_info->x8_mode) {
		// dual rank, x16 DDR
		if (6400 == part_info->data_rate_mtps) {
			override_table = phy_override_seq_lp5_drank_x16_6400;
		} else if (4800 == part_info->data_rate_mtps) {
			override_table = phy_override_seq_lp5_drank_x16_4800;
		}
	} else {
		// dual rank, x8 DDR
		if (4800 == part_info->data_rate_mtps) {
			override_table = phy_override_seq_lp5_drank_x8_4800;
		}
	}

	lpddr_training_table_init(ddrc_base, lp5_train_table, override_table, NULL);
}

void config_lp5_addrmap(volatile uint32_t* ddrc_reg, ddr_part_info* part_info)
{
	uint32_t ba0_pos, ba1_pos, ba0_field, ba1_field, i, j, msb, ba_msb, region_blks;
	// field value of col0~col9, row0~row17
	uint8_t col_field[10], row_field[18];
	uint32_t addrmap3, addrmap5, addrmap6;
	uint32_t addrmap7, addrmap8, addrmap9, addrmap10, addrmap11, addrmap12;
	bool non_binary_density = false;

	memset(col_field, 0, sizeof(col_field));
	memset(row_field, 0x1F, sizeof(row_field));

	// convert MByte to Byte
	msb = fls(part_info->size_mb) - 1 + 20;
	if (!is_power_of_2(part_info->size_mb)) {
		msb++;
		non_binary_density = true;
	}
	ba_msb = msb;
	pr_info("MSB: %d\n", msb);
	if (part_info->ranks > 1) {
		// MSB is reserved for CS bit
		ba_msb--;
	}

	// address bit position
	ba0_pos = 9;
	ba1_pos = 16;

	pr_info("Configuring LPDDR5 address map with BA0 position: %d, BA1 position: %d\n",
		ba0_pos, ba1_pos);

	// addrmap_bank_b0 Internal Base=3, addrmap_bank_b1 Internal Base=4
	// field_value = HIF_bit - Internal_Base
	// ba0 base=3, HIF=ba0_pos - 3, field=HIF-3
	ba0_field = ba0_pos - 3 - 3;
	// ba1 base=4, HIF=ba1_pos - 3, field=HIF-4
	ba1_field = ba1_pos - 3 - 4;

	// BA0/BA1/COLUMN4~9 use bit position BIT9~BIT16
	// addrmap_col_bN Internal Base=N
	for (i = 9, j = 4; (i <= ba_msb) && (j < ARRAY_SIZE(col_field)); i++) {
		if (i == ba0_pos || i == ba1_pos || i == 8) {
			continue;
		}

		col_field[j] = i - j - 3;
		j++;
	}

	// BA0/BA1/ROW0~17
	// addrmap_row_bN Internal Base=N+6
	for (j = 0; (i <= ba_msb) && (j < ARRAY_SIZE(row_field)); i++) {
		if (i == ba0_pos || i == ba1_pos) {
			continue;
		}

		row_field[j] = i - j - 6 - 3;
		j++;
	}

	if (non_binary_density) {
		// ADDRMAP12: [row_msb:row_msb-1]==0b11 is invalid
		addrmap12 = j - 13;
	} else {
		// ADDRMAP12: address mapping extension register, use default value
		addrmap12 = 0x00000000;
	}

	// ADDRMAP3: [13:8]=addrmap_ba_b1, [5:0]=addrmap_ba_b0, ba2 is unused
	addrmap3 = (0x3F << 16) | (ba1_field << 8) | ba0_field;
	// ADDRMAP5: [28:24]=col10(unused=0x1f), [20:16]=col9, [12:8]=col8, [4:0]=col7
	addrmap5 = (0x1f << 24) | ((uint32_t)col_field[9] << 16)
		| ((uint32_t)col_field[8] << 8) | ((uint32_t)col_field[7] << 0);
	// ADDRMAP6: [27:24]=col6, [19:16]=col5, [11:8]=col4, [3:0]=col3(fixed 0)
	addrmap6 = ((uint32_t)col_field[6] << 24) | ((uint32_t)col_field[5] << 16)
		| ((uint32_t)col_field[4] << 8) | 0;
	// ADDRMAP7: [27:24]=row17, [20:16]=row16, [12:8]=row15, [4:0]=row14
	addrmap7 = ((uint32_t)row_field[17] << 24) | ((uint32_t)row_field[16] << 16)
		| ((uint32_t)row_field[15] << 8) | ((uint32_t)row_field[14] << 0);
	// ADDRMAP8: [27:24]=row13, [20:16]=row12, [12:8]=row11, [4:0]=row10
	addrmap8 = ((uint32_t)row_field[13] << 24) | ((uint32_t)row_field[12] << 16)
		| ((uint32_t)row_field[11] << 8) | ((uint32_t)row_field[10] << 0);
	// ADDRMAP9: [27:24]=row9, [20:16]=row8, [12:8]=row7, [4:0]=row6
	addrmap9 = ((uint32_t)row_field[9] << 24) | ((uint32_t)row_field[8] << 16)
		| ((uint32_t)row_field[7] << 8) | ((uint32_t)row_field[6] << 0);
	// ADDRMAP10: [27:24]=row5, [20:16]=row4, [12:8]=row3, [4:0]=row2
	addrmap10 = ((uint32_t)row_field[5] << 24) | ((uint32_t)row_field[4] << 16)
		| ((uint32_t)row_field[3] << 8) | ((uint32_t)row_field[2] << 0);
	// ADDRMAP11: [12:8]=row1, [4:0]=row0
	addrmap11 = ((uint32_t)row_field[1] << 8) | ((uint32_t)row_field[0]);

	// ---- SAR (System Address Region) and address mapping configuration ----
	// split to 4 regions, each 256MB aligned (SARBASE unit: 256MB)
	region_blks = part_info->size_mb / 4 / 256;
	// SARBASE0: SAR Region0 base address = 8×256MB = 2GB
	ddrc_reg[0x000200c0 / 4] = 8;
	// SARSIZE0: SAR Region0 size = region_blks×256MB
	ddrc_reg[0x000200c4 / 4] = region_blks - 1;
	// SARBASE1: SAR Region1 base address
	ddrc_reg[0x000200c8 / 4] = 8 + region_blks * 1;
	// SARSIZE1: SAR Region1 size
	ddrc_reg[0x000200cc / 4] = region_blks - 1;
	// SARBASE2: SAR Region2 base address
	ddrc_reg[0x000200d0 / 4] = 8 + region_blks * 2;
	// SARSIZE2: SAR Region2 size
	ddrc_reg[0x000200d4 / 4] = region_blks - 1;
	// SARBASE3: SAR Region3 base address
	ddrc_reg[0x000200d8 / 4] = 8 + region_blks * 3;
	// SARSIZE3: SAR Region3 size
	ddrc_reg[0x000200dc / 4] = region_blks - 1;

	if (part_info->ranks > 1) {
		// ADDRMAP1: addrmap_cs_bit0=msb, dual rank = msb - interal base(6) - offset(3)
		ddrc_reg[0x00030004 / 4] = msb - 6 - 3;
	} else {
		// ADDRMAP1: addrmap_cs_bit0=0x3f, single rank, CS mapping disabled
		ddrc_reg[0x00030004 / 4] = 0x0000003f;
	}

	// ADDRMAP3: bank address mapping (BA0/BA1/BA2)
	ddrc_reg[0x0003000c / 4] = addrmap3;
	// ADDRMAP4: bank group address mapping (BG0/BG1)
	ddrc_reg[0x00030010 / 4] = 0x00000101;
	// ADDRMAP5: column address mapping (col7~col10)
	ddrc_reg[0x00030014 / 4] = addrmap5;
	// ADDRMAP6: column address mapping (col3~col6)
	ddrc_reg[0x00030018 / 4] = addrmap6;
	// ADDRMAP7: row address mapping (row14~row17)
	ddrc_reg[0x0003001c / 4] = addrmap7;
	// ADDRMAP8: row address mapping (row10~row13)
	ddrc_reg[0x00030020 / 4] = addrmap8;
	// ADDRMAP9: row address mapping (row6~row9)
	ddrc_reg[0x00030024 / 4] = addrmap9;
	// ADDRMAP10: row address mapping (row2~row5)
	ddrc_reg[0x00030028 / 4] = addrmap10;
	// ADDRMAP11: row address mapping (row0~row1)
	ddrc_reg[0x0003002c / 4] = addrmap11;
	// ADDRMAP12: address mapping extension register
	ddrc_reg[0x00030030 / 4] = addrmap12;
}

static int init_snps_lp5_ddrc_dfi(volatile uint32_t* ddrc_reg, ddr_part_info* part_info)
{
	uint32_t reg_val, rate, x8_mode, ranks;

	rate = part_info->data_rate_mtps;
	x8_mode = part_info->x8_mode;
	ranks = part_info->ranks;

	// only support 6400 and 4800
	if ((6400 != rate) && (4800 != rate)) {
		pr_err("unsupported ddr rate %d\n", rate);
		return -1;
	}

	/* mstr0 (0x10000) */
	reg_val = 0;
	reg_val |= (0x8 << 16); // burst_rdwr [20:16]
	reg_val |= (0x0 << 12); // data_bus_width [13:12]
	reg_val |= (0x1 << 3); // lpddr5 [3]
	reg_val |= (0x0 << 1); // lpddr4 [1]
	if (2 == ranks) {
		reg_val |= (0x3 << 24); // active_ranks [25:24]
	} else {
		reg_val |= (0x1 << 24); // active_ranks [25:24]
	}
	ddrc_reg[0x10000 / 4] = reg_val;

	ddrc_reg[0x10510 / 4] = 0x00010005; /* dfimisc */
	ddrc_reg[0x10518 / 4] = 0x70000000; /* dfiphymstr */

	if (2 == part_info->ranks) {
		ddrc_reg[0x10200 / 4] = 0x00000361; /* rfshmod0 */
	} else {
		ddrc_reg[0x10200 / 4] = 0x010003f3; /* rfshmod0 */
	}

	// disable page close
	ddrc_reg[0x10380 / 4] = 0x80012010; /* sched0 */

	/* deratectl1 (0x10104) */
	reg_val = 0;
	if (x8_mode) {
		reg_val |= (0xf << 0); // active_derate_byte_rank0 [7:0]
	} else {
		reg_val |= (0x5 << 0); // active_derate_byte_rank0 [7:0]
	}
	ddrc_reg[0x10104 / 4] = reg_val;

	/* deratectl2 (0x10108) */
	reg_val = 0;
	if (x8_mode) {
		reg_val |= (0xf << 0); // active_derate_byte_rank1 [7:0]
	} else {
		reg_val |= (0x5 << 0); // active_derate_byte_rank1 [7:0]
	}
	ddrc_reg[0x10108 / 4] = reg_val;

	ddrc_reg[0x10508 / 4] = 0xc0000000; /* dfiupd0 */
	ddrc_reg[0x10b84 / 4] = 0x00000003; /* opctrl1 */
	ddrc_reg[0x10180 / 4] = 0x00000011; /* pwrctl */
	ddrc_reg[0x10d00 / 4] = 0x00020002; /* inittmg0 */

	/* dfiupdtmg0 (0x005a8) */
	reg_val = 0;
	reg_val |= (0x1c << 26); //  [31:26]
	reg_val |= (0x0 << 14); // dfi_ctrlup_unit_sel [15:14]
	reg_val |= (0xd << 0); // dfi_t_ctrlup_min [9:0]
	if (6400 == rate) {
		reg_val |= (0x1a4 << 16); // dfi_t_ctrlup_max [25:16]
	} else if (4800 == rate) {
		reg_val |= (0x140 << 16); // dfi_t_ctrlup_max [25:16]
	}
	ddrc_reg[0x005a8 / 4] = reg_val;

	ddrc_reg[0x005a4 / 4] = 0x00000300; /* dfilptmg1 */
	ddrc_reg[0x00d00 / 4] = 0x00000001; /* tmgcfg */
	ddrc_reg[0x005b4 / 4] = 0xe000012c; /* dfiupdtmg2 */

	/* dramset1tmg0 (0x00000) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0x10 << 16); // t_faw [23:16]
		reg_val |= (0x36 << 8); // t_ras_max [15:8]
		reg_val |= (0x22 << 0); // t_ras_min [7:0]
		if (x8_mode) {
			reg_val |= (0x29 << 24); // wr2pre [31:24]
		} else {
			reg_val |= (0x28 << 24); // wr2pre [31:24]
		}
	} else if (4800 == rate) {
		reg_val |= (0xc << 16); // t_faw [23:16]
		reg_val |= (0x29 << 8); // t_ras_max [15:8]
		reg_val |= (0x1a << 0); // t_ras_min [7:0]
		if (x8_mode) {
			reg_val |= (0x20 << 24); // wr2pre [31:24]
		} else {
			reg_val |= (0x1f << 24); // wr2pre [31:24]
		}
	}
	ddrc_reg[0x00000 / 4] = reg_val;

	/* dramset1tmg1 (0x00004) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0x10 << 16); // t_xp [21:16]
		reg_val |= (0x6 << 8); // rd2pre [15:8]
		reg_val |= (0x30 << 0); // t_rc [7:0]
	} else if (4800 == rate) {
		reg_val |= (0xc << 16); // t_xp [21:16]
		reg_val |= (0x5 << 8); // rd2pre [15:8]
		reg_val |= (0x24 << 0); // t_rc [7:0]
	}
	ddrc_reg[0x00004 / 4] = reg_val;

	/* dramset1tmg2 (0x00008) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0x9 << 24); // write_latency [30:24]
		if (x8_mode) {
			reg_val |= (0x12 << 16); // read_latency [22:16]
			reg_val |= (0x12 << 8); // rd2wr [15:8]
			reg_val |= (0x19 << 0); // wr2rd [7:0]
		} else {
			reg_val |= (0x11 << 16); // read_latency [22:16]
			reg_val |= (0x11 << 8); // rd2wr [15:8]
			reg_val |= (0x17 << 0); // wr2rd [7:0]
		}
	} else if (4800 == rate) {
		reg_val |= (0x7 << 24); // write_latency [30:24]
		if (x8_mode) {
			reg_val |= (0xe << 16); // read_latency [22:16]
			reg_val |= (0x10 << 8); // rd2wr [15:8]
			reg_val |= (0x14 << 0); // wr2rd [7:0]
		} else {
			reg_val |= (0xd << 16); // read_latency [22:16]
			reg_val |= (0xf << 8); // rd2wr [15:8]
			reg_val |= (0x13 << 0); // wr2rd [7:0]
		}
	}
	ddrc_reg[0x00008 / 4] = reg_val;

	/* dramset1tmg3 (0x0000c) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0xc << 16); // t_mr [22:16]
		if (x8_mode) {
			reg_val |= (0x22 << 8); // rd2mr [15:8]
			reg_val |= (0x30 << 0); // wr2mr [7:0]
		} else {
			reg_val |= (0x21 << 8); // rd2mr [15:8]
			reg_val |= (0x2f << 0); // wr2mr [7:0]
		}
	} else if (4800 == rate) {
		reg_val |= (0xa << 16); // t_mr [22:16]
		if (x8_mode) {
			reg_val |= (0x1b << 8); // rd2mr [15:8]
			reg_val |= (0x26 << 0); // wr2mr [7:0]
		} else {
			reg_val |= (0x1a << 8); // rd2mr [15:8]
			reg_val |= (0x25 << 0); // wr2mr [7:0]
		}
	}
	ddrc_reg[0x0000c / 4] = reg_val;

	/* dramset1tmg4 (0x00010) */
	reg_val = 0;
	reg_val |= (0x4 << 16); // t_ccd [21:16]
	if (6400 == rate) {
		reg_val |= (0xf << 24); // t_rcd [31:24]
		reg_val |= (0x4 << 8); // t_rrd [13:8]
		reg_val |= (0xf << 0); // t_rp [6:0]
	} else if (4800 == rate) {
		reg_val |= (0xb << 24); // t_rcd [31:24]
		reg_val |= (0x3 << 8); // t_rrd [13:8]
		reg_val |= (0xb << 0); // t_rp [6:0]
	}
	ddrc_reg[0x00010 / 4] = reg_val;

	/* dramset1tmg5 (0x00014) */
	reg_val = 0;
	reg_val |= (0x2 << 24); // t_cksrx [29:24]
	if (6400 == rate) {
		reg_val |= (0x4 << 16); // t_cksre [22:16]
		reg_val |= (0xc << 8); // t_ckesr [14:8]
		reg_val |= (0x9 << 0); // t_cke [5:0]
	} else if (4800 == rate) {
		reg_val |= (0x3 << 16); // t_cksre [22:16]
		reg_val |= (0x9 << 8); // t_ckesr [14:8]
		reg_val |= (0x7 << 0); // t_cke [5:0]
	}
	ddrc_reg[0x00014 / 4] = reg_val;

	/* dramset1tmg6 (0x00018) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0x12 << 0); // t_ckcsx [5:0]
	} else if (4800 == rate) {
		reg_val |= (0xe << 0); // t_ckcsx [5:0]
	}
	ddrc_reg[0x00018 / 4] = reg_val;

	/* dramset1tmg7 (0x0001c) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0xc9000 << 4); //  [31:4]
		reg_val |= (0x3 << 0); // t_csh [3:0]
	} else if (4800 == rate) {
		reg_val |= (0x97000 << 4); //  [31:4]
		reg_val |= (0x2 << 0); // t_csh [3:0]
	}
	ddrc_reg[0x0001c / 4] = reg_val;

	/* dramset1tmg9 (0x00024) */
	reg_val = 0;
	reg_val |= (0x2 << 16); // t_ccd_s [20:16]
	if (6400 == rate) {
		reg_val |= (0x4 << 8); // t_rrd_s [13:8]
		if (x8_mode) {
			reg_val |= (0x12 << 0); // wr2rd_s [7:0]
		} else {
			reg_val |= (0x10 << 0); // wr2rd_s [7:0]
		}
	} else if (4800 == rate) {
		reg_val |= (0x3 << 8); // t_rrd_s [13:8]
		if (x8_mode) {
			reg_val |= (0xe << 0); // wr2rd_s [7:0]
		} else {
			reg_val |= (0xd << 0); // wr2rd_s [7:0]
		}
	}
	ddrc_reg[0x00024 / 4] = reg_val;

	ddrc_reg[0x00030 / 4] = 0x00030000; /* dramset1tmg12 */

	/* dramset1tmg13 (0x00034) */
	reg_val = 0;
	reg_val |= (0x10 << 16); // t_ccd_mw [22:16]
	reg_val |= (0x2 << 0); // t_ppd [3:0]
	if (6400 == rate) {
		reg_val |= (0xc << 24); // odtloff [30:24]
	} else if (4800 == rate) {
		reg_val |= (0xa << 24); // odtloff [30:24]
	}
	ddrc_reg[0x00034 / 4] = reg_val;

	/* dramset1tmg14 (0x00038) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0x20 << 16); // t_osco [24:16]
		reg_val |= (0xe6 << 0); // t_xsr [11:0]
	} else if (4800 == rate) {
		reg_val |= (0x18 << 16); // t_osco [24:16]
		reg_val |= (0xad << 0); // t_xsr [11:0]
	}
	ddrc_reg[0x00038 / 4] = reg_val;

	/* dramset1tmg24 (0x00060) */
	reg_val = 0;
	reg_val |= (0x0 << 24); // bank_org [25:24]
	if (6400 == rate) {
		reg_val |= (0xe << 0); // max_wr_sync [7:0]
		if (x8_mode) {
			reg_val |= (0x10 << 16); // rd2wr_s [23:16]
			reg_val |= (0x17 << 8); // max_rd_sync [15:8]
		} else {
			reg_val |= (0xf << 16); // rd2wr_s [23:16]
			reg_val |= (0x16 << 8); // max_rd_sync [15:8]
		}
	} else if (4800 == rate) {
		reg_val |= (0xc << 0); // max_wr_sync [7:0]
		if (x8_mode) {
			reg_val |= (0xe << 16); // rd2wr_s [23:16]
			reg_val |= (0x13 << 8); // max_rd_sync [15:8]
		} else {
			reg_val |= (0xd << 16); // rd2wr_s [23:16]
			reg_val |= (0x12 << 8); // max_rd_sync [15:8]
		}
	}
	ddrc_reg[0x00060 / 4] = reg_val;

	/* dramset1tmg25 (0x00064) */
	reg_val = 0;
	reg_val |= (0x0 << 16); // lpddr4_diff_bank_rwa2pre [18:16]
	if (6400 == rate) {
		reg_val |= (0x6 << 0); // rda2pre [7:0]
		if (x8_mode) {
			reg_val |= (0x29 << 8); // wra2pre [15:8]
		} else {
			reg_val |= (0x28 << 8); // wra2pre [15:8]
		}
	} else if (4800 == rate) {
		reg_val |= (0x5 << 0); // rda2pre [7:0]
		if (x8_mode) {
			reg_val |= (0x20 << 8); // wra2pre [15:8]
		} else {
			reg_val |= (0x1f << 8); // wra2pre [15:8]
		}
	}
	ddrc_reg[0x00064 / 4] = reg_val;

	/* dramset1tmg30 (0x00078) */
	reg_val = 0;
	if (x8_mode) {
		if (6400 == rate) {
			reg_val |= (0x1a << 16); // mrr2mrw [23:16]
			reg_val |= (0x14 << 8); // mrr2wr [15:8]
			reg_val |= (0x19 << 0); // mrr2rd [7:0]
		} else if (4800 == rate) {
			reg_val |= (0x15 << 16); // mrr2mrw [23:16]
			reg_val |= (0x12 << 8); // mrr2wr [15:8]
			reg_val |= (0x15 << 0); // mrr2rd [7:0]
		}
	} else {
		if (6400 == rate) {
			reg_val |= (0x19 << 16); // mrr2mrw [23:16]
			reg_val |= (0x13 << 8); // mrr2wr [15:8]
			reg_val |= (0x18 << 0); // mrr2rd [7:0]
		} else if (4800 == rate) {
			reg_val |= (0x14 << 16); // mrr2mrw [23:16]
			reg_val |= (0x11 << 8); // mrr2wr [15:8]
			reg_val |= (0x14 << 0); // mrr2rd [7:0]
		}
	}
	ddrc_reg[0x00078 / 4] = reg_val;

	/* dramset1tmg32 (0x00080) */
	if (4800 == rate) {
		reg_val = 0;
		reg_val |= (0x3 << 16); // ws_off2ws_fs [19:16]
		reg_val |= (0x4 << 8); // t_wcksus [11:8]
		reg_val |= (0x7 << 0); // ws_fs2wck_sus [3:0]
		ddrc_reg[0x00080 / 4] = reg_val;
	}

	/* rfshset1tmg0 (0x00600) */
	reg_val = 0;
	reg_val |= (0x1 << 31); // t_refi_x1_sel [31]
	reg_val |= (0x1 << 30); // refresh_to_x1_sel [30]
	reg_val |= (0x0 << 24); // refresh_margin [27:24]
	reg_val |= (0x3F << 16); // refresh_to_x1_x32 [21:16]
	if (6400 == rate) {
		reg_val |= (0xC34 << 0); // t_refi_x1_x32 [13:0]
	} else if (4800 == rate) {
		reg_val |= (0x92C << 0); // t_refi_x1_x32 [13:0]
	}
	ddrc_reg[0x00600 / 4] = reg_val;

	/* rfshset1tmg1 (0x00604) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0xe0 << 16); // t_rfc_min_ab [27:16]
		reg_val |= (0xe0 / 2 << 0); // t_rfc_min [11:0]
	} else if (4800 == rate) {
		reg_val |= (0xa8 << 16); // t_rfc_min_ab [27:16]
		reg_val |= (0xa8 / 2 << 0); // t_rfc_min [11:0]
	}
	ddrc_reg[0x00604 / 4] = reg_val;

	/* rfshset1tmg2 (0x00608) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0x6 << 24); // t_pbr2act [31:24]
		reg_val |= (0x48 << 16); // t_pbr2pbr [23:16]
	} else if (4800 == rate) {
		reg_val |= (0x5 << 24); // t_pbr2act [31:24]
		reg_val |= (0x36 << 16); // t_pbr2pbr [23:16]
	}
	ddrc_reg[0x00608 / 4] = reg_val;

	/* zqset1tmg0 (0x00800) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0x18 << 16); // t_zq_short_nop [25:16]
		reg_val |= (0x4d7 << 0); // t_zq_long_nop [13:0]
	} else if (4800 == rate) {
		reg_val |= (0x12 << 16); // t_zq_short_nop [25:16]
		reg_val |= (0x3ab << 0); // t_zq_long_nop [13:0]
	}
	ddrc_reg[0x00800 / 4] = reg_val;

	/* zqset1tmg1 (0x00804) */
	reg_val = 0;
	reg_val |= (0x100 << 0); // t_zq_short_interval_x1024 [19:0]
	if (6400 == rate) {
		reg_val |= (0x28 << 20); // t_zq_reset_nop [29:20]
	} else if (4800 == rate) {
		reg_val |= (0x1e << 20); // t_zq_reset_nop [29:20]
	}
	ddrc_reg[0x00804 / 4] = reg_val;

	/* derateval0 (0x00b04) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0x10 << 24); // derated_t_rcd [31:24]
		reg_val |= (0x24 << 16); // derated_t_ras_min [23:16]
		reg_val |= (0x10 << 8); // derated_t_rp [14:8]
		reg_val |= (0xa << 0); // derated_t_rrd [5:0]
	} else if (4800 == rate) {
		reg_val |= (0xc << 24); // derated_t_rcd [31:24]
		reg_val |= (0x1b << 16); // derated_t_ras_min [23:16]
		reg_val |= (0xc << 8); // derated_t_rp [14:8]
		reg_val |= (0x8 << 0); // derated_t_rrd [5:0]
	}
	ddrc_reg[0x00b04 / 4] = reg_val;

	/* derateval1 (0x00b08) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0x33 << 0); // derated_t_rc [7:0]
	} else if (4800 == rate) {
		reg_val |= (0x27 << 0); // derated_t_rc [7:0]
	}
	ddrc_reg[0x00b08 / 4] = reg_val;

	/* ranktmg0 (0x00d04) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0xe << 8); // diff_rank_wr_gap [15:8]
		reg_val |= (0x12 << 0); // diff_rank_rd_gap [7:0]
	} else if (4800 == rate) {
		reg_val |= (0xd << 8); // diff_rank_wr_gap [15:8]
		reg_val |= (0x10 << 0); // diff_rank_rd_gap [7:0]
	}
	ddrc_reg[0x00d04 / 4] = reg_val;

	/* dfitmg0 (0x00580) */
	reg_val = 0;
	reg_val |= (0x3 << 24); // dfi_t_ctrl_delay [28:24]
	reg_val |= (0x2 << 8); // dfi_tphy_wrdata [13:8]
	if (6400 == rate) {
		reg_val |= (0x1f << 0); // dfi_tphy_wrlat [6:0]
		if (x8_mode) {
			reg_val |= (0x43 << 16); // dfi_t_rddata_en [23:16]
		} else {
			reg_val |= (0x3f << 16); // dfi_t_rddata_en [23:16]
		}
	} else if (4800 == rate) {
		reg_val |= (0x17 << 0); // dfi_tphy_wrlat [6:0]
		if (x8_mode) {
			reg_val |= (0x33 << 16); // dfi_t_rddata_en [23:16]
		} else {
			reg_val |= (0x2f << 16); // dfi_t_rddata_en [23:16]
		}
	}
	ddrc_reg[0x00580 / 4] = reg_val;

	ddrc_reg[0x00584 / 4] = 0x00080303; /* dfitmg1 */

	/* dfitmg2 (0x00588) */
	reg_val = 0;
	reg_val |= (0x18 << 16); // dfi_twck_delay [21:16]
	if (6400 == rate) {
		reg_val |= (0x1f << 0); // dfi_tphy_wrcslat [6:0]
		if (x8_mode) {
			reg_val |= (0x43 << 8); // dfi_tphy_rdcslat [14:8]
		} else {
			reg_val |= (0x3f << 8); // dfi_tphy_rdcslat [14:8]
		}
	} else if (4800 == rate) {
		reg_val |= (0x17 << 0); // dfi_tphy_wrcslat [6:0]
		if (x8_mode) {
			reg_val |= (0x33 << 8); // dfi_tphy_rdcslat [14:8]
		} else {
			reg_val |= (0x2f << 8); // dfi_tphy_rdcslat [14:8]
		}
	}
	ddrc_reg[0x00588 / 4] = reg_val;

	/* dfitmg4 (0x00590) */
	reg_val = 0;
	reg_val |= (0x4 << 8); // dfi_twck_en_fs [15:8]
	reg_val |= (0x3 << 0); // dfi_twck_dis [7:0]
	if (6400 == rate) {
		reg_val |= (0xc << 16); // dfi_twck_en_wr [23:16]
		if (x8_mode) {
			reg_val |= (0x1c << 24); // dfi_twck_en_rd [31:24]
		} else {
			reg_val |= (0x18 << 24); // dfi_twck_en_rd [31:24]
		}
	} else if (4800 == rate) {
		reg_val |= (0x8 << 16); // dfi_twck_en_wr [23:16]
		if (x8_mode) {
			reg_val |= (0x14 << 24); // dfi_twck_en_rd [31:24]
		} else {
			reg_val |= (0x10 << 24); // dfi_twck_en_rd [31:24]
		}
	}
	ddrc_reg[0x00590 / 4] = reg_val;

	/* dfitmg5 (0x00594) */
	reg_val = 0;
	reg_val |= (0x4 << 24); // dfi_twck_fast_toggle [31:24]
	reg_val |= (0x0 << 8); // dfi_twck_toggle_cs [15:8]
	reg_val |= (0xf << 0); // dfi_twck_toggle_post [7:0]
	if (6400 == rate) {
		reg_val |= (0x10 << 16); // dfi_twck_toggle [23:16]
	} else if (4800 == rate) {
		reg_val |= (0xc << 16); // dfi_twck_toggle [23:16]
	}
	ddrc_reg[0x00594 / 4] = reg_val;

	ddrc_reg[0x00500 / 4] = 0x00000000; /* initmr0 */

	/* dramset1tmg23 (0x0005c) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0x9d << 16); // t_xsr_dsm_x1024 [23:16]
		reg_val |= (0x9 << 0); // t_pdn [11:0]
	} else if (4800 == rate) {
		reg_val |= (0x76 << 16); // t_xsr_dsm_x1024 [23:16]
		reg_val |= (0x7 << 0); // t_pdn [11:0]
	}
	ddrc_reg[0x0005c / 4] = reg_val;

	/* dfiupdtmg3 (0x005b8) */
	reg_val = 0;
	reg_val |= (0x96000 << 9); //  [31:9]
	if (6400 == rate) {
		reg_val |= (0x147 << 0); // dfi_t_ctrlupd_burst_interval_x8 [8:0]
	} else if (4800 == rate) {
		reg_val |= (0xe7 << 0); // dfi_t_ctrlupd_burst_interval_x8 [8:0]
	}
	ddrc_reg[0x005b8 / 4] = reg_val;

	/* ranktmg1 (0x00d08) */
	reg_val = 0;
	if (x8_mode) {
		reg_val |= (0xa << 0); // wr2rd_dr [7:0]
		if (6400 == rate) {
			reg_val |= (0x16 << 8); // rd2wr_dr [15:8]
		} else if (4800 == rate) {
			reg_val |= (0x13 << 8); // rd2wr_dr [15:8]
		}
	} else {
		reg_val |= (0xb << 0); // wr2rd_dr [7:0]
		if (6400 == rate) {
			reg_val |= (0x15 << 8); // rd2wr_dr [15:8]
		} else if (4800 == rate) {
			reg_val |= (0x12 << 8); // rd2wr_dr [15:8]
		}
	}
	ddrc_reg[0x00d08 / 4] = reg_val;

	/* rfmset1tmg0 (0x00650) */
	reg_val = 0;
	if (6400 == rate) {
		reg_val |= (0x98 << 0); // t_rfmpb [11:0]
	} else if (4800 == rate) {
		reg_val |= (0x72 << 0); // t_rfmpb [11:0]
	}
	ddrc_reg[0x00650 / 4] = reg_val;

	return 0;
}

void init_snps_lp5_ddrc(unsigned DDRC_BASE, ddr_part_info* part_info,
	ddr_boot_mode ddr_mode, ddr_training_info_t* training_info)
{
	uint32_t read_data;
	uint32_t CFG_BASE = DDRC_BASE + 0x600000;
	uint32_t DPHY_BASE = DDRC_BASE + 0x800000;
	uint32_t count = 0x100;
	uint32_t rst_code = 22;
	volatile uint32_t* ddrc_reg = (volatile uint32_t*)(size_t)DDRC_BASE;
	volatile uint32_t* cfg_reg = (volatile uint32_t*)(size_t)CFG_BASE;
	volatile uint32_t* dphy_reg = (volatile uint32_t*)(size_t)DPHY_BASE;

	if (0 != init_snps_lp5_ddrc_dfi(ddrc_reg, part_info)) {
		return;
	}

	// ---- SAR(System Address Region)和地址映射配置 ----
	config_lp5_addrmap(ddrc_reg, part_info);

	ddrc_reg[0x00010b84 / 4] = 0x00000002;  /* opctrl1 */
	ddrc_reg[0x00010d00 / 4] = 0xc0020002;  /* inittmg0 */
	ddrc_reg[0x00010180 / 4] = 0x00000811;  /* pwrctl */
	ddrc_reg[0x00010180 / 4] = 0x00000800;  /* pwrctl */
	ddrc_reg[0x00010208 / 4] = 0x00000001;  /* rfshctl0 */

	cfg_reg[0x18 / 4] |= (1 << rst_code); // RELEASE FOR DCLK
	cfg_reg[0x18 / 4] |= (1 << 1);

	ddrc_reg[0x00010280 / 4] = 0x80000000;  /* zqctl0 */
	ddrc_reg[0x000005b4 / 4] = 0xc000012c;  /* dfiupdtmg2 */
	ddrc_reg[0x00010c80 / 4] = 0x00000000;  /* swctl */
	ddrc_reg[0x00010288 / 4] = 0x00000001;  /* zqctl2 */
	ddrc_reg[0x00010c80 / 4] = 0x00000001;  /* swctl */
	while ((ddrc_reg[0x00010c84 / 4] & 1) != 1)
		; /* poll swstat */
	ddrc_reg[0x00010c80 / 4] = 0x00000000;  /* swctl */
	ddrc_reg[0x00010510 / 4] = 0x00010014;  /* dfimisc */
	ddrc_reg[0x00010c80 / 4] = 0x00000001;  /* swctl */
	while ((ddrc_reg[0x00010c84 / 4] & 1) != 1)
		; /* poll swstat */

	if (DDR_QUICKBOOT_MODE != ddr_mode) {
		phyinit_lp5_pre_training(DDRC_BASE, part_info);

		ddrc_reg[0x00010180 / 4] |= (0x1 << 11);

		dphy_reg[0xd0000] = 0x1;
		dphy_reg[0xd0099] = 0x9;
		dphy_reg[0xd0099] = 0x1;
		dphy_reg[0xd0099] = 0x0;
		read_data = major_message_all(DPHY_BASE);
		if (read_data == 0xff)
			return;
		dphy_reg[0xd0099] = 0x1;
		while (count--)
			;
		dphy_reg[0xd0000] = 0x0;

		phyinit_lp5_training(DDRC_BASE, part_info);

		// save DDR training info
		save_snps_ddrc_training_result(DDRC_BASE, training_info);
	} else {
		init_snps_ddrc_quick(DDRC_BASE, DDR_TYPE_LPDDR5, training_info);
	}

	ddrc_reg[0x00010c80 / 4] = 0x00000000;  /* swctl */
	ddrc_reg[0x00010c80 / 4] = 0x00000001;  /* swctl */
	while ((ddrc_reg[0x00010c84 / 4] & 1) != 1)
		; /* poll swstat */
	ddrc_reg[0x00010c80 / 4] = 0x00000000;  /* swctl */
	ddrc_reg[0x00010510 / 4] = 0x00010034;  /* dfimisc */
	ddrc_reg[0x00010c80 / 4] = 0x00000001;  /* swctl */
	while ((ddrc_reg[0x00010c84 / 4] & 1) != 1)
		; /* poll swstat */
	while ((ddrc_reg[0x00010514 / 4] & 1) != 1)
		; /* poll dfistat */
	ddrc_reg[0x00010c80 / 4] = 0x00000000;  /* swctl */
	ddrc_reg[0x00010510 / 4] = 0x00010015;  /* dfimisc */
	ddrc_reg[0x00010c80 / 4] = 0x00000001;  /* swctl */
	while ((ddrc_reg[0x00010c84 / 4] & 1) != 1)
		; /* poll swstat */
	ddrc_reg[0x00010180 / 4] = 0x00000000;  /* pwrctl */
	while ((ddrc_reg[0x00010014 / 4] & 3) != 1)
		; /* poll stat */
	ddrc_reg[0x00010c80 / 4] = 0x00000000;  /* swctl */
	ddrc_reg[0x00010c80 / 4] = 0x00000001;  /* swctl */
	while ((ddrc_reg[0x00010c84 / 4] & 1) != 1)
		; /* poll swstat */
	ddrc_reg[0x00010280 / 4] = 0x00000000;  /* zqctl0 */
	ddrc_reg[0x00010c80 / 4] = 0x00000000;  /* swctl */
	ddrc_reg[0x00010288 / 4] = 0x00000000;  /* zqctl2 */
	ddrc_reg[0x00010c80 / 4] = 0x00000001;  /* swctl */
	while ((ddrc_reg[0x00010c84 / 4] & 1) != 1)
		; /* poll swstat */
	ddrc_reg[0x00010208 / 4] = 0x00000000;  /* rfshctl0 */
	ddrc_reg[0x00010180 / 4] = 0x00000010;  /* pwrctl, disable self-refresh */
	ddrc_reg[0x000005b4 / 4] = 0xe000012c;  /* dfiupdtmg2 */
	ddrc_reg[0x00010b84 / 4] = 0x00000000;  /* opctrl1 */
	ddrc_reg[0x00020090 / 4] = 0x00000001;  /* pctrl */
	ddrc_reg[0x00021090 / 4] = 0x00000001;  /* pctrl */
	ddrc_reg[0x00022090 / 4] = 0x00000001;  /* pctrl */
	ddrc_reg[0x00023090 / 4] = 0x00000001;  /* pctrl */
	ddrc_reg[0x00024090 / 4] = 0x00000001;  /* pctrl */
}
