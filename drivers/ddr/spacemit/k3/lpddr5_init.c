// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Spacemit
 */

#include "k3_ddr.h"

#include <linux/kernel.h>

void lpddr_training_table_init(uint32_t ddrc_base,
	const phy_init_config* train_table[], const ddr_phy_reg_config* override_table)
{
	uint32_t reg_base, reg_offset;
	unsigned long DPHY_BASE = ddrc_base + 0x800000;
	volatile uint32_t* phy_reg = (uint32_t*)DPHY_BASE;
	uint16_t* phy_data;
	const phy_init_config* sub_table;
	int i, j, k;
	bool need_override = false;

	for (i = 0, k = 0; NULL != train_table[i]; i++) {
		sub_table = train_table[i];
		reg_base = sub_table->base;

		if ((NULL != override_table) && ((override_table[k].offset & ~0x7FFF) == reg_base)) {
			need_override = true;
		} else {
			need_override = false;
		}

		if (sub_table->is_linear_increase) {
			phy_data = (uint16_t*)sub_table->sequence;
			for (j = 0; j < sub_table->count; j++) {
				reg_offset = reg_base + j;
				if (need_override && (override_table[k].offset == reg_offset)) {
					// skip the PHY setting when value is 0xdeadbeef
					if (DDR_CONFIG_BYPASS_MAGIC != override_table[k].value) {
						phy_reg[reg_offset] = override_table[k].value;
					}
					k++;
					if ((override_table[k].offset & ~0x7FFF) != reg_base) {
						need_override = false;
					}
				} else {
					phy_reg[reg_offset] = phy_data[j];
				}
			}
		} else {
			for (j = 0; j < sub_table->count; j++) {
				reg_offset = reg_base + sub_table->sequence[j].a.offset;
				if (need_override && (override_table[k].offset == reg_offset)) {
					// skip the PHY setting when value is 0xdeadbeef
					if (DDR_CONFIG_BYPASS_MAGIC != override_table[k].value) {
						phy_reg[reg_offset] = override_table[k].value;
					}
					k++;
					if ((override_table[k].offset & ~0x7FFF) != reg_base) {
						need_override = false;
					}
				} else {
					phy_reg[reg_offset] = sub_table->sequence[j].a.value;
				}
			}
		}
	}
}

static void phyinit_lp5_pre_training(uint32_t ddrc_base, uint32_t ddr_size_mbyte)
{
	uint32_t offset = 0;
	unsigned long DPHY_BASE = ddrc_base + 0x800000;
	const ddr_phy_reg_config* override_table;

	if (4096 == ddr_size_mbyte) {
		override_table = phy_override_pre_seq_lp5_4g;
	} else if (16384 == ddr_size_mbyte) {
		override_table = phy_override_pre_seq_lp5_16g;
	} else {
		override_table = NULL;
		pr_info("Use default pre-training talbe\n");
	}

	lpddr_training_table_init(ddrc_base, lp5_pre_train_table, override_table);

	for (offset = 0x584d2; offset < 0x60000; offset++)
		REG32(DPHY_BASE + offset * 4) = 0x0;
}

static void phyinit_lp5_training(uint32_t ddrc_base, uint32_t ddr_size_mbyte)
{
	const ddr_phy_reg_config* override_table;

	if (4096 == ddr_size_mbyte) {
		lpddr_training_table_init(ddrc_base, lp5_4g_train_table, NULL);
	} else {
		if (16384 == ddr_size_mbyte) {
			override_table = phy_override_seq_lp5_16g;
		} else {
			override_table = NULL;
		}
		lpddr_training_table_init(ddrc_base, lp5_train_table, override_table);
	}
}

#if TRAINING_DEBUG
void translate_streaming(uint32_t* d)
{
}
#endif

void accept_message(uint32_t dphy_base)
{
	uint32_t read_data;

	REG32(dphy_base + 0x000d0031 * 4) = 0x00000000;
	read_data = REG32(dphy_base + 0x000d0004 * 4);
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = REG32(dphy_base + 0x000d0004 * 4);
	}
	REG32(dphy_base + 0x000d0031 * 4) = 0x00000001;
}

uint32_t major_message_all(uint32_t dphy_base)
{
	uint32_t read_data;
	uint32_t i;
	uint32_t j;
	uint32_t cnt = 0x1000;
#if TRAINING_DEBUG
	uint32_t read_data1;
	static uint32_t dmsg[50];
#endif

	read_data = REG32(dphy_base + 0x000d0004 * 4);
	while ((read_data & 0x00000001) != 0x00000000) {
		read_data = REG32(dphy_base + 0x000d0004 * 4);
	}
	read_data = REG32(dphy_base + 0x000d0032 * 4);

	while ((read_data & 0x000000ff) != 0x00000007) {
		if ((read_data & 0x000000ff) == 0x00000008) {
			accept_message(dphy_base);

			read_data = REG32(dphy_base + 0x000d0004 * 4);
			while ((read_data & 0x00000001) != 0x00000000) {
				read_data = REG32(dphy_base + 0x000d0004 * 4);
			}
			j = REG32(dphy_base + 0x000d0032 * 4);
			i = 0;
			while (i <= j) {

				read_data = REG32(dphy_base + 0x000d0032 * 4);
#if TRAINING_DEBUG
				read_data1 = REG32(dphy_base + 0x000d0034 * 4);
				dmsg[i] = (read_data1 << 16) | read_data;
				// LogMsg(0,"read dmsg 0x%08X\n",dmsg[i]);
				if (i == j) {
					translate_streaming(dmsg);
				}
#endif
				accept_message(dphy_base);
				i++;
				read_data = REG32(dphy_base + 0x000d0004 * 4);
				while ((read_data & 0x00000001) != 0x00000000) {
					read_data = REG32(dphy_base + 0x000d0004 * 4);
				}
			}

		} else {
			LogMsg(0, "== Training major message ==\n");
			LogMsg(0, "%02x\n", read_data);

			if (read_data == 0xff) {
				REG32(dphy_base + 0xd0099 * 4) = 0x1;
				while (cnt--)
					;
				REG32(dphy_base + 0xd0000 * 4) = 0x0;
				read_data = REG32(dphy_base + 0x200c9 * 4);
				LogMsg(0, "plllockstatus is %02x\n", read_data);
				return read_data;
			}
			// while(cnt--);
			LogMsg(0, "============================\n");
			accept_message(dphy_base);
			read_data = REG32(dphy_base + 0x000d0004 * 4);
			while ((read_data & 0x00000001) != 0x00000000) {
				read_data = REG32(dphy_base + 0x000d0004 * 4);
			}
		}
		read_data = REG32(dphy_base + 0x000d0032 * 4);
	}

	LogMsg(0, "== Training major message ==\n");
	LogMsg(0, "%02x\n", read_data);
	LogMsg(0, "============================\n");

	accept_message(dphy_base);

	return 0;
}

void init_snps_lp5_ddrc(unsigned DDRC_BASE, uint32_t rst_code, uint32_t ddr_size_mbyte)
{
	uint32_t read_data;
	uint32_t CFG_BASE = DDRC_BASE + 0x600000;
	uint32_t DPHY_BASE = DDRC_BASE + 0x800000;
	uint32_t count = 0x100;

	if (8192 == ddr_size_mbyte || 16384 == ddr_size_mbyte) {
		REG32(DDRC_BASE + 0x00010000) = 0x03080008;
	} else if (4096 == ddr_size_mbyte) {
		REG32(DDRC_BASE + 0x00010000) = 0x01080008;
	}
	REG32(0xD4282CE8) &= 0x00ffffff;
	REG32(0xD4282CE8) |= ((REG32(DDRC_BASE + 0x00010000) & 0xff) << 24);

	REG32(DDRC_BASE + 0x00010008) = 0x00000000;
	REG32(DDRC_BASE + 0x00010510) = 0x00010005;
	REG32(DDRC_BASE + 0x00010518) = 0x70000000;
	REG32(DDRC_BASE + 0x00010208) = 0x00000000;
	if (4096 == ddr_size_mbyte)
		REG32(DDRC_BASE + 0x00010200) = 0x010003f3;
	else // dsty_16GB
		REG32(DDRC_BASE + 0x00010200) = 0x00000361;

	REG32(DDRC_BASE + 0x00010280) = 0x00000000;
	REG32(DDRC_BASE + 0x00010220) = 0x0a000100;
	REG32(DDRC_BASE + 0x00010224) = 0x00000000;
	REG32(DDRC_BASE + 0x00010288) = 0x00000000;
	REG32(DDRC_BASE + 0x00010380) = 0x80012014;
	REG32(DDRC_BASE + 0x00010100) = 0x00000020;
	if (16384 == ddr_size_mbyte) {
		REG32(DDRC_BASE + 0x00010104) = 0x0000000f;
		REG32(DDRC_BASE + 0x00010108) = 0x0000000f;
	} else {
		// dsty_4GB || dsty_8GB
		REG32(DDRC_BASE + 0x00010104) = 0x00000005;
		REG32(DDRC_BASE + 0x00010108) = 0x00000005;
	}
	REG32(DDRC_BASE + 0x00010118) = 0x00000000;
	REG32(DDRC_BASE + 0x00010c90) = 0x0000000f;
	REG32(DDRC_BASE + 0x00010b80) = 0x00000000;
	REG32(DDRC_BASE + 0x00010300) = 0x00400040;
	REG32(DDRC_BASE + 0x00010384) = 0x00002000;
	REG32(DDRC_BASE + 0x0001038c) = 0x04040208;
	REG32(DDRC_BASE + 0x00010390) = 0x08400810;
	REG32(DDRC_BASE + 0x00010500) = 0x00100000;
	REG32(DDRC_BASE + 0x00010508) = 0xc0000000;
	REG32(DDRC_BASE + 0x00010ca0) = 0x00000000;
	REG32(DDRC_BASE + 0x00010c80) = 0x00000001;
	REG32(DDRC_BASE + 0x00010f00) = 0x80008200;
	REG32(DDRC_BASE + 0x00010580) = 0x00110011;
	REG32(DDRC_BASE + 0x00010ca4) = 0x00000000;
	REG32(DDRC_BASE + 0x00010308) = 0x00000000;
	REG32(DDRC_BASE + 0x0001018c) = 0x0000003f;
	REG32(DDRC_BASE + 0x00010184) = 0x00000003;
	REG32(DDRC_BASE + 0x00010114) = 0x00000001;
	REG32(DDRC_BASE + 0x00010128) = 0x00000000;

	REG32(DDRC_BASE + 0x00010c94) = 0x00000001;

	REG32(DDRC_BASE + 0x00010b84) = 0x00000003;
	REG32(DDRC_BASE + 0x00010180) = 0x00000011;
	REG32(DDRC_BASE + 0x00010d00) = 0x00020002;
	REG32(DDRC_BASE + 0x00010010) = 0x00000100;
	REG32(DDRC_BASE + 0x00010084) = 0x00000000;
	REG32(DDRC_BASE + 0x00010284) = 0x00000000;
	REG32(DDRC_BASE + 0x00010b8c) = 0x00000000;
	REG32(DDRC_BASE + 0x00010b98) = 0x00000000;
	REG32(DDRC_BASE + 0x000005a8) = 0x71a4000d;
	REG32(DDRC_BASE + 0x000005a0) = 0x00000000;
	REG32(DDRC_BASE + 0x000005a4) = 0x00000300;
	REG32(DDRC_BASE + 0x000005b0) = 0x00000004;
	REG32(DDRC_BASE + 0x00000d00) = 0x00000001;
	REG32(DDRC_BASE + 0x000005b4) = 0xe000012c;

	if (16384 == ddr_size_mbyte) {
		REG32(DDRC_BASE + 0x00000000) = 0x29103622;
		REG32(DDRC_BASE + 0x00000004) = 0x00100630;
		REG32(DDRC_BASE + 0x00000008) = 0x09121219;
		REG32(DDRC_BASE + 0x0000000c) = 0x000c2230;
		REG32(DDRC_BASE + 0x00000010) = 0x0f04040f;
		REG32(DDRC_BASE + 0x00000014) = 0x02040c09;
		REG32(DDRC_BASE + 0x00000018) = 0x00000012;
		REG32(DDRC_BASE + 0x0000001c) = 0x00000003;
		REG32(DDRC_BASE + 0x00000024) = 0x00020412;
		REG32(DDRC_BASE + 0x00000030) = 0x00030000;
		REG32(DDRC_BASE + 0x00000034) = 0x0c100002;
		REG32(DDRC_BASE + 0x00000038) = 0x002000e6;
		REG32(DDRC_BASE + 0x00000060) = 0x0010170e;
		REG32(DDRC_BASE + 0x00000064) = 0x00002906;
		REG32(DDRC_BASE + 0x00000078) = 0x001a1419;
		REG32(DDRC_BASE + 0x00000080) = 0x00030408;

		REG32(DDRC_BASE + 0x00000600) = 0xc03d0c34;
		REG32(DDRC_BASE + 0x00000604) = 0x00e00070;
		REG32(DDRC_BASE + 0x00000608) = 0x06480000;
		REG32(DDRC_BASE + 0x0000060c) = 0x3f000000;
		REG32(DDRC_BASE + 0x00000610) = 0x00000000;

		REG32(DDRC_BASE + 0x00000800) = 0x001804d7;
		REG32(DDRC_BASE + 0x00000804) = 0x02800100;
		REG32(DDRC_BASE + 0x00000d0c) = 0x00400010;
		REG32(DDRC_BASE + 0x00000c84) = 0x0f00007f;
		REG32(DDRC_BASE + 0x00000b80) = 0x00000000;
		REG32(DDRC_BASE + 0x00000b04) = 0x1024100a;
		REG32(DDRC_BASE + 0x00000b08) = 0x00000033;
		REG32(DDRC_BASE + 0x00000b00) = 0x00800000;
		REG32(DDRC_BASE + 0x00000d04) = 0x00000e12;
		REG32(DDRC_BASE + 0x00000580) = 0x0343021f;
		REG32(DDRC_BASE + 0x00000584) = 0x00080303;
		REG32(DDRC_BASE + 0x00000588) = 0x0018431f;
		REG32(DDRC_BASE + 0x00000590) = 0x1c0c0403;
		REG32(DDRC_BASE + 0x00000594) = 0x0410000f;
		REG32(DDRC_BASE + 0x00000500) = 0x00000000;
		REG32(DDRC_BASE + 0x00000504) = 0x00000000;
		REG32(DDRC_BASE + 0x00000508) = 0x00000000;
		REG32(DDRC_BASE + 0x0000050c) = 0x00000000;
		REG32(DDRC_BASE + 0x0000005c) = 0x009d0009;
		REG32(DDRC_BASE + 0x00000c00) = 0x00000000;
		REG32(DDRC_BASE + 0x000005ac) = 0x00010001;
		REG32(DDRC_BASE + 0x000005b8) = 0x00000147;
		REG32(DDRC_BASE + 0x00000a80) = 0x00000070;
		REG32(DDRC_BASE + 0x00000d08) = 0x0000160a;
	} else {
		// dsty_4GB || dsty_8GB
		REG32(DDRC_BASE + 0x00000000) = 0x28103622;
		REG32(DDRC_BASE + 0x00000004) = 0x00100630;
		REG32(DDRC_BASE + 0x00000008) = 0x09111117;
		REG32(DDRC_BASE + 0x0000000c) = 0x000c212f;
		REG32(DDRC_BASE + 0x00000010) = 0x0f04040f;
		REG32(DDRC_BASE + 0x00000014) = 0x02040c09;
		REG32(DDRC_BASE + 0x00000018) = 0x00000012;
		REG32(DDRC_BASE + 0x0000001c) = 0x00000003;
		REG32(DDRC_BASE + 0x00000024) = 0x00020410;
		REG32(DDRC_BASE + 0x00000030) = 0x00030000;
		REG32(DDRC_BASE + 0x00000034) = 0x0c100002;
		REG32(DDRC_BASE + 0x00000038) = 0x002000e6;
		REG32(DDRC_BASE + 0x00000060) = 0x000f160e;
		REG32(DDRC_BASE + 0x00000064) = 0x00002806;
		REG32(DDRC_BASE + 0x00000078) = 0x00191318;
		REG32(DDRC_BASE + 0x00000080) = 0x00030408;

		REG32(DDRC_BASE + 0x00000600) = 0xc03d0c34;
		REG32(DDRC_BASE + 0x00000604) = 0x00e00070;
		REG32(DDRC_BASE + 0x00000608) = 0x06480000;
		REG32(DDRC_BASE + 0x0000060c) = 0x3f000000;
		REG32(DDRC_BASE + 0x00000610) = 0x00000000;

		REG32(DDRC_BASE + 0x00000800) = 0x001804d7;
		REG32(DDRC_BASE + 0x00000804) = 0x02800100;
		REG32(DDRC_BASE + 0x00000d0c) = 0x00400010;
		REG32(DDRC_BASE + 0x00000c84) = 0x0f00007f;
		REG32(DDRC_BASE + 0x00000b80) = 0x00000000;
		REG32(DDRC_BASE + 0x00000b04) = 0x1024100a;
		REG32(DDRC_BASE + 0x00000b08) = 0x00000033;
		REG32(DDRC_BASE + 0x00000b00) = 0x00800000;
		REG32(DDRC_BASE + 0x00000d04) = 0x00000e12;
		REG32(DDRC_BASE + 0x00000580) = 0x033f021f;
		REG32(DDRC_BASE + 0x00000584) = 0x00080303;
		REG32(DDRC_BASE + 0x00000588) = 0x00183f1f;
		REG32(DDRC_BASE + 0x00000590) = 0x180c0403;
		REG32(DDRC_BASE + 0x00000594) = 0x0410000f;
		REG32(DDRC_BASE + 0x00000500) = 0x00000000;
		REG32(DDRC_BASE + 0x00000504) = 0x00000000;
		REG32(DDRC_BASE + 0x00000508) = 0x00000000;
		REG32(DDRC_BASE + 0x0000050c) = 0x00000000;
		REG32(DDRC_BASE + 0x0000005c) = 0x009d0009;
		REG32(DDRC_BASE + 0x00000c00) = 0x00000000;
		REG32(DDRC_BASE + 0x000005ac) = 0x00010001;
		REG32(DDRC_BASE + 0x000005b8) = 0x00000147;
		REG32(DDRC_BASE + 0x00000a80) = 0x00000070;
		REG32(DDRC_BASE + 0x00000d08) = 0x0000150b;
	}
	REG32(DDRC_BASE + 0x00000c80) = 0x0f000001;
	REG32(DDRC_BASE + 0x00000c88) = 0x0f00007f;
	REG32(DDRC_BASE + 0x00000650) = 0x00000098;
	REG32(DDRC_BASE + 0x00000d30) = 0x002faf09;
	REG32(DDRC_BASE + 0x00000d34) = 0x180009c5;
	REG32(DDRC_BASE + 0x00020000) = 0x00000000;
	REG32(DDRC_BASE + 0x00020004) = 0x0000501f;
	REG32(DDRC_BASE + 0x00021004) = 0x0000501f;
	REG32(DDRC_BASE + 0x00022004) = 0x0000501f;
	REG32(DDRC_BASE + 0x00023004) = 0x0000501f;
	REG32(DDRC_BASE + 0x00024004) = 0x0000501f;
	REG32(DDRC_BASE + 0x00020008) = 0x0000501f;
	REG32(DDRC_BASE + 0x00021008) = 0x0000501f;
	REG32(DDRC_BASE + 0x00022008) = 0x0000501f;
	REG32(DDRC_BASE + 0x00023008) = 0x0000501f;
	REG32(DDRC_BASE + 0x00024008) = 0x0000501f;
	REG32(DDRC_BASE + 0x00020090) = 0x00000000;
	REG32(DDRC_BASE + 0x00021090) = 0x00000000;
	REG32(DDRC_BASE + 0x00022090) = 0x00000000;
	REG32(DDRC_BASE + 0x00023090) = 0x00000000;
	REG32(DDRC_BASE + 0x00024090) = 0x00000000;
	REG32(DDRC_BASE + 0x00020094) = 0x00000000;
	REG32(DDRC_BASE + 0x00021094) = 0x00000000;
	REG32(DDRC_BASE + 0x00022094) = 0x00000000;
	REG32(DDRC_BASE + 0x00023094) = 0x00000000;
	REG32(DDRC_BASE + 0x00024094) = 0x00000000;
	REG32(DDRC_BASE + 0x00020098) = 0x00000000;
	REG32(DDRC_BASE + 0x00021098) = 0x00000000;
	REG32(DDRC_BASE + 0x00022098) = 0x00000000;
	REG32(DDRC_BASE + 0x00023098) = 0x00000000;
	REG32(DDRC_BASE + 0x00024098) = 0x00000000;
	REG32(DDRC_BASE + 0x0002009c) = 0x00000e00;
	REG32(DDRC_BASE + 0x0002109c) = 0x00000e00;
	REG32(DDRC_BASE + 0x0002209c) = 0x00000e00;
	REG32(DDRC_BASE + 0x0002309c) = 0x00000e00;
	REG32(DDRC_BASE + 0x0002409c) = 0x00000e00;
	REG32(DDRC_BASE + 0x000200a0) = 0x00000000;
	REG32(DDRC_BASE + 0x000210a0) = 0x00000000;
	REG32(DDRC_BASE + 0x000220a0) = 0x00000000;
	REG32(DDRC_BASE + 0x000230a0) = 0x00000000;
	REG32(DDRC_BASE + 0x000240a0) = 0x00000000;

	if (8192 == ddr_size_mbyte) {
		REG32(DDRC_BASE + 0x000200c0) = 0x00000008; // SARBASE0
		REG32(DDRC_BASE + 0x000200c4) = 0x00000007; // SARSIZE0
		REG32(DDRC_BASE + 0x000200c8) = 0x00000010; // SARBASE1
		REG32(DDRC_BASE + 0x000200cc) = 0x00000007; // SARSIZE1
		REG32(DDRC_BASE + 0x000200d0) = 0x00000018; // SARBASE2
		REG32(DDRC_BASE + 0x000200d4) = 0x00000007; // SARSIZE2
		REG32(DDRC_BASE + 0x000200d8) = 0x00000020; // SARBASE3
		REG32(DDRC_BASE + 0x000200dc) = 0x00000007; // SARSIZE3

		REG32(DDRC_BASE + 0x00030004) = 0x00000018; // ADDRMAPX
		REG32(DDRC_BASE + 0x0003000c) = 0x003f0903;
		REG32(DDRC_BASE + 0x00030010) = 0x00000101;
		REG32(DDRC_BASE + 0x00030014) = 0x1f030303;
		REG32(DDRC_BASE + 0x00030018) = 0x03030300;
		REG32(DDRC_BASE + 0x0003001c) = 0x1f1f0808;
		REG32(DDRC_BASE + 0x00030020) = 0x08080808;
		REG32(DDRC_BASE + 0x00030024) = 0x08080808;
		REG32(DDRC_BASE + 0x00030028) = 0x08080808;
		REG32(DDRC_BASE + 0x0003002c) = 0x00000808;
	} else if (4096 == ddr_size_mbyte) {
		REG32(DDRC_BASE + 0x000200c0) = 0x00000008; // SARBASE0
		REG32(DDRC_BASE + 0x000200c4) = 0x00000003; // SARSIZE0
		REG32(DDRC_BASE + 0x000200c8) = 0x0000000c; // SARBASE1
		REG32(DDRC_BASE + 0x000200cc) = 0x00000003; // SARSIZE1
		REG32(DDRC_BASE + 0x000200d0) = 0x00000010; // SARBASE2
		REG32(DDRC_BASE + 0x000200d4) = 0x00000003; // SARSIZE2
		REG32(DDRC_BASE + 0x000200d8) = 0x00000014; // SARBASE3
		REG32(DDRC_BASE + 0x000200dc) = 0x00000003; // SARSIZE3

		REG32(DDRC_BASE + 0x00030004) = 0x0000003f; // ADDRMAPX
		REG32(DDRC_BASE + 0x0003000c) = 0x003f0903;
		REG32(DDRC_BASE + 0x00030010) = 0x00000101;
		REG32(DDRC_BASE + 0x00030014) = 0x1f030303;
		REG32(DDRC_BASE + 0x00030018) = 0x03030300;
		REG32(DDRC_BASE + 0x0003001c) = 0x1f1f0808;
		REG32(DDRC_BASE + 0x00030020) = 0x08080808;
		REG32(DDRC_BASE + 0x00030024) = 0x08080808;
		REG32(DDRC_BASE + 0x00030028) = 0x08080808;
		REG32(DDRC_BASE + 0x0003002c) = 0x00000808;
	} else {
		// dsty_16GB
		REG32(DDRC_BASE + 0x000200c0) = 0x00000008; // SARBASE0
		REG32(DDRC_BASE + 0x000200c4) = 0x0000000f; // SARSIZE0
		REG32(DDRC_BASE + 0x000200c8) = 0x00000018; // SARBASE1
		REG32(DDRC_BASE + 0x000200cc) = 0x0000000f; // SARSIZE1
		REG32(DDRC_BASE + 0x000200d0) = 0x00000028; // SARBASE2
		REG32(DDRC_BASE + 0x000200d4) = 0x0000000f; // SARSIZE2
		REG32(DDRC_BASE + 0x000200d8) = 0x00000038; // SARBASE3
		REG32(DDRC_BASE + 0x000200dc) = 0x0000000f; // SARSIZE3

		REG32(DDRC_BASE + 0x00030004) = 0x00000019; // ADDRMAPX
		REG32(DDRC_BASE + 0x0003000c) = 0x003f0903;
		REG32(DDRC_BASE + 0x00030010) = 0x00000101;
		REG32(DDRC_BASE + 0x00030014) = 0x1f030303;
		REG32(DDRC_BASE + 0x00030018) = 0x03030300;
		REG32(DDRC_BASE + 0x0003001c) = 0x1f080808;
		REG32(DDRC_BASE + 0x00030020) = 0x08080808;
		REG32(DDRC_BASE + 0x00030024) = 0x08080808;
		REG32(DDRC_BASE + 0x00030028) = 0x08080808;
		REG32(DDRC_BASE + 0x0003002c) = 0x00000808;
	}

	REG32(DDRC_BASE + 0x00030030) = 0x00000000;
	REG32(DDRC_BASE + 0x00010b84) = 0x00000002;
	REG32(DDRC_BASE + 0x00010d00) = 0xc0020002;
	REG32(DDRC_BASE + 0x00010180) = 0x00000811;
	REG32(DDRC_BASE + 0x00010180) = 0x00000800;
	REG32(DDRC_BASE + 0x00010208) = 0x00000001;
	REG32(CFG_BASE + 0x18) |= (1 << rst_code); // RELEASE FOR DCLK
	REG32(CFG_BASE + 0x18) |= (1 << 1);
	REG32(DDRC_BASE + 0x00010280) = 0x80000000;
	REG32(DDRC_BASE + 0x000005b4) = 0xc000012c;
	REG32(DDRC_BASE + 0x00010c80) = 0x00000000;
	REG32(DDRC_BASE + 0x00010288) = 0x00000001;
	REG32(DDRC_BASE + 0x00010c80) = 0x00000001;
	read_data = REG32(DDRC_BASE + 0x00010c84);
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010c84);
	}
	REG32(DDRC_BASE + 0x00010c80) = 0x00000000;
	REG32(DDRC_BASE + 0x00010510) = 0x00010014;
	REG32(DDRC_BASE + 0x00010c80) = 0x00000001;
	read_data = REG32(DDRC_BASE + 0x00010c84);
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010c84);
	}

	REG32(DDRC_BASE + 0x00010208) = 0x00000000;

	phyinit_lp5_pre_training(DDRC_BASE, ddr_size_mbyte);

	REG32(DDRC_BASE + 0x00010180) |= (0x1 << 11);

	REG32(DPHY_BASE + 0xd0000 * 4) = 0x1;
	REG32(DPHY_BASE + 0xd0099 * 4) = 0x9;
	REG32(DPHY_BASE + 0xd0099 * 4) = 0x1;
	REG32(DPHY_BASE + 0xd0099 * 4) = 0x0;
	read_data = major_message_all(DPHY_BASE);
	if (read_data == 0xff)
		return;
	REG32(DPHY_BASE + 0xd0099 * 4) = 0x1;
	while (count--)
		;
	count = 0x100;
	REG32(DPHY_BASE + 0xd0000 * 4) = 0x0;

	phyinit_lp5_training(DDRC_BASE, ddr_size_mbyte);

	if (16384 != ddr_size_mbyte) {
		REG32(DDRC_BASE + 0x00010c80) = 0x00000000;
		REG32(DDRC_BASE + 0x00000060) = 0x0010160e;
		REG32(DDRC_BASE + 0x00000024) = 0x00020410;
		REG32(DDRC_BASE + 0x00010c80) = 0x00000001;
		read_data = REG32(DDRC_BASE + 0x00010c84);
		while ((read_data & 0x00000001) != 0x00000001) {
			read_data = REG32(DDRC_BASE + 0x00010c84);
		}
	}
	REG32(DDRC_BASE + 0x00010c80) = 0x00000000;
	REG32(DDRC_BASE + 0x00010510) = 0x00010034;
	REG32(DDRC_BASE + 0x00010c80) = 0x00000001;
	read_data = REG32(DDRC_BASE + 0x00010c84);
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010c84);
	}
	read_data = REG32(DDRC_BASE + 0x00010514);
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010514);
	}
	REG32(DDRC_BASE + 0x00010c80) = 0x00000000;
	REG32(DDRC_BASE + 0x00010510) = 0x00010015;
	REG32(DDRC_BASE + 0x00010c80) = 0x00000001;
	read_data = REG32(DDRC_BASE + 0x00010c84);
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010c84);
	}
	REG32(DDRC_BASE + 0x00010180) = 0x00000000;
	read_data = REG32(DDRC_BASE + 0x00010014);
	while ((read_data & 0x00000003) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010014);
	}
	REG32(DDRC_BASE + 0x00010c80) = 0x00000000;
	REG32(DDRC_BASE + 0x00010508) = 0xc0000000;
	REG32(DDRC_BASE + 0x00010c80) = 0x00000001;
	read_data = REG32(DDRC_BASE + 0x00010c84);
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010c84);
	}
	REG32(DDRC_BASE + 0x00010280) = 0x00000000;
	REG32(DDRC_BASE + 0x00010c80) = 0x00000000;
	REG32(DDRC_BASE + 0x00010288) = 0x00000000;
	REG32(DDRC_BASE + 0x00010c80) = 0x00000001;
	read_data = REG32(DDRC_BASE + 0x00010c84);
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010c84);
	}
	REG32(DDRC_BASE + 0x00010208) = 0x00000000;
	REG32(DDRC_BASE + 0x00010180) = 0x00000011;
	REG32(DDRC_BASE + 0x000005b4) = 0xe000012c;
	REG32(DDRC_BASE + 0x00010b84) = 0x00000000;
	REG32(DDRC_BASE + 0x00020090) = 0x00000001;
	REG32(DDRC_BASE + 0x00021090) = 0x00000001;
	REG32(DDRC_BASE + 0x00022090) = 0x00000001;
	REG32(DDRC_BASE + 0x00023090) = 0x00000001;
	REG32(DDRC_BASE + 0x00024090) = 0x00000001;

	REG32(DPHY_BASE + 0xd0000 * 4) = 0x0;
	REG32(DPHY_BASE + 0xc0080 * 4) = 0x3;
}

static void init_ddr_clock(uint32_t DDRC_BASE, uint32_t data_rate_mtps)
{
	uint32_t read_data;
	uint32_t CFG_BASE = DDRC_BASE + 0x600000;

	if (5500 == part_info->data_rate_mtps) {
		/* DPLL 2750MHz*/
		REG32(CFG_BASE + 0x8) = 0x0b3912aa;
		REG32(CFG_BASE + 0x10) = 0xa0558b8b;
		REG32(CFG_BASE + 0xc) |= (0x1 << 22) | (0x1 << 16) | (0xff) | (0xab << 8);
	} else if (6000 == part_info->data_rate_mtps) {
		/* DPLL 3000MHz*/
		REG32(CFG_BASE + 0x8) = 0x0b3e2000;
		REG32(CFG_BASE + 0x10) = 0xa0558c8c;
		REG32(CFG_BASE + 0xc) |= (0x1 << 22) | (0x1 << 16) | (0xff) | (0x00 << 8);
	} else {
		/* DPLL 3200MHz*/
		REG32(CFG_BASE + 0xc) |= (0x1 << 22) | (0x1 << 16) | (0xff);
	}

	read_data = REG32(CFG_BASE + 0x1c);
	while ((read_data & 0x00000001) != 0x1) {
		read_data = REG32(CFG_BASE + 0x1c);
	}
	// clear frequency divider
	REG32(CFG_BASE + 0x18) &= ~(0x3f << 16);

	if (1066 == part_info->data_rate_mtps) {
		REG32(CFG_BASE + 0x18) |= (0x1 << 19) | (0x7 << 16); // sel 2, div 8
	} else if (4266 == part_info->data_rate_mtps) {
		REG32(CFG_BASE + 0x18) |= (0x1 << 19) | (0x1 << 16); // sel 2, div 2
		// REG32(CFG_BASE + 0x18) |= (0x2 << 19) | (0x1 << 16); // sel 2, div 2 3200mbps
	} else if (5120 == part_info->data_rate_mtps) {
		REG32(CFG_BASE + 0x18) |= (0x7 << 19) | (0x0 << 16); // sel 3, div 1 5120mbps
	} else {
		REG32(CFG_BASE + 0x18) |= (0x2 << 19) | (0x0 << 16); // sel 3, div 1 6400mbps
		// REG32(CFG_BASE + 0x18) |= (0x1 << 19) | (0x1 << 16); // sel 3, div 1
	}

	// initial frequency change
	REG32(CFG_BASE + 0x18) |= (1 << 25);
	LogMsg(0, "read 6400 reg 0x%08X 0x%08X\n", CFG_BASE + 0x18, REG32(CFG_BASE + 0x18));
	REG32(0xD4282CE8) = REG32(CFG_BASE + 0x18);
	LogMsg(0, "check setting reg 0x%08X 0x%08X\n", 0xD4282CE8, REG32(0xD4282CE8));
	read_data = REG32(CFG_BASE + 0x18);
	while ((read_data & 0x2000000) != 0x0) {
		read_data = REG32(CFG_BASE + 0x18);
	}
	REG32(CFG_BASE + 0x18) |= 0x1;
}

void init_snps_lp45(unsigned DDRC_BASE, ddr_part_info* part_info)
{
	uint32_t rst_code = 22;

	init_ddr_clock(DDRC_BASE, part_info->data_rate_mtps);

	if (DDR_TYPE_LPDDR5 == part_info->type) {
		init_snps_lp5_ddrc(DDRC_BASE, rst_code, part_info->size_mb);
	} else if (DDR_TYPE_LPDDR4X == part_info->type) {
		init_snps_lp4x_ddrc(DDRC_BASE, rst_code, part_info->size_mb);
	}
}

void lpddr_silicon_init(uint64_t ddrc_reg_base, ddr_part_info* part_info)
{
	LogMsg(0, "=== start init_lpddr() ===\n");
	init_snps_lp45(ddrc_reg_base, part_info);
	LogMsg(0, "=== finish init_lpddr() ===\n");
}
