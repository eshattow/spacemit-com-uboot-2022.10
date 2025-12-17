// SPDX-License-Identifier: GPL-2.0+
/*
* Copyright (C) 2025 Spacemit
*/

#include "k3_ddr.h"

#include <linux/kernel.h>

static void phyinit_lp5_6400_training_pre(unsigned int ddrc_base)
{
	unsigned int reg_base, offset = 0;
	unsigned long DPHY_BASE = ddrc_base + 0x800000;
	volatile uint32_t* phy_reg = (uint32_t*)DPHY_BASE;
	int i, j;

	for (i = 0; i < ARRAY_SIZE(lp5_pre_train_table); i++) {
		reg_base = lp5_pre_train_table[i]->base;

		if (lp5_pre_train_table[i]->is_linear_increase) {
			for (j = 0; j < lp5_pre_train_table[i]->count / 2; j++) {
				phy_reg[reg_base + j * 2]
					= lp5_pre_train_table[i]->sequence[j].b.value0;
				phy_reg[reg_base + j * 2 + 1]
					= lp5_pre_train_table[i]->sequence[j].b.value1;
			}
			if (0 != (lp5_pre_train_table[i]->count % 2)) {
				phy_reg[reg_base + j * 2]
					= lp5_pre_train_table[i]->sequence[j].b.value0;
			}
		} else {
			for (j = 0; j < lp5_pre_train_table[i]->count; j++) {
				phy_reg[reg_base + lp5_pre_train_table[i]->sequence[j].a.offset]
					= lp5_pre_train_table[i]->sequence[j].a.value;
			}
		}
	}

	for (offset = 0x584d2; offset < 0x60000; offset++)
		REG32(DPHY_BASE + offset * 4) = 0x0;
}

static void phyinit_lp5_6400_training(unsigned int ddrc_base)
{
	unsigned int reg_base;
	unsigned long DPHY_BASE = ddrc_base + 0x800000;
	volatile uint32_t* phy_reg = (uint32_t*)DPHY_BASE;
	int i, j;

	for (i = 0; i < ARRAY_SIZE(lp5_train_table); i++) {
		reg_base = lp5_train_table[i]->base;

		if (lp5_train_table[i]->is_linear_increase) {
			for (j = 0; j < lp5_train_table[i]->count / 2; j++) {
				phy_reg[reg_base + j * 2]
					= lp5_train_table[i]->sequence[j].b.value0;
				phy_reg[reg_base + j * 2 + 1]
					= lp5_train_table[i]->sequence[j].b.value1;
			}
			if (0 != (lp5_train_table[i]->count % 2)) {
				phy_reg[reg_base + j * 2]
					= lp5_train_table[i]->sequence[j].b.value0;
			}
		} else {
			for (j = 0; j < lp5_train_table[i]->count; j++) {
				phy_reg[reg_base + lp5_train_table[i]->sequence[j].a.offset]
					= lp5_train_table[i]->sequence[j].a.value;
			}
		}
	}
}

#if TRAINING_DEBUG
void translate_streaming(unsigned int* d)
{
}
#endif

void accept_message(unsigned int dphy_base)
{
	unsigned int read_data;

	REG32(dphy_base + 0x000d0031 * 4) = 0x00000000;
	read_data = REG32(dphy_base + 0x000d0004 * 4);
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = REG32(dphy_base + 0x000d0004 * 4);
	}
	REG32(dphy_base + 0x000d0031 * 4) = 0x00000001;
}

void major_message_all(unsigned int dphy_base)
{
	unsigned int read_data;
	unsigned int i;
	unsigned int j;
#if TRAINING_DEBUG
	unsigned int read_data1;
	unsigned int dmsg[12];
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
			LogMsg(0, "%02x", read_data);
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
	LogMsg(0, "%02x", read_data);
	LogMsg(0, "============================\n");
	accept_message(dphy_base);
}

void init_snps_lp45(unsigned DDRC_BASE, unsigned rst_code)
{
	unsigned int read_data;
	unsigned int CFG_BASE = DDRC_BASE + 0x600000;
	unsigned int DPHY_BASE = DDRC_BASE + 0x800000;
	unsigned int count = 0x10;

	REG32(CFG_BASE + 0xc) |= (0x1 << 22) | (0x1 << 16) | (0xff);
	read_data = REG32(CFG_BASE + 0x1c);
	while ((read_data & 0x00000001) != 0x1) {
		read_data = REG32(CFG_BASE + 0x1c);
	}

#if CONFIG_DDR_DATARATE == 4266
	REG32(CFG_BASE + 0x18) &= ~(0x3f << 16); // div 0
	REG32(CFG_BASE + 0x18) |= (0x1 << 19) | (0x1 << 16); // sel 2, div 2
	REG32(CFG_BASE + 0x18) |= (1 << 25); // fc
#else //~LPDDR5_4266
	REG32(CFG_BASE + 0x18) &= ~(0x3f << 16); // div 0
	REG32(CFG_BASE + 0x18) |= (0x2 << 19) | (0x0 << 16); // sel 3, div 1
	REG32(CFG_BASE + 0x18) |= (1 << 25); // fc
#endif

	read_data = REG32(CFG_BASE + 0x18);
	while ((read_data & 0x2000000) != 0x0) {
		read_data = REG32(CFG_BASE + 0x18);
	}
	REG32(CFG_BASE + 0x18) |= 0x1;
#if CONFIG_DDR_DATARATE == 4266
#include "snps_init_lp5_4266.c"
#else //~LPDDR5_4266

	REG32(DDRC_BASE + 0x00010b84) = 0x00000001;
	REG32(DDRC_BASE + 0x00010000) = 0x03080008;
	REG32(DDRC_BASE + 0x00010010) = 0x00000111;
	REG32(DDRC_BASE + 0x00010100) = 0x00000021;
	REG32(DDRC_BASE + 0x00010104) = 0x00000005;
	REG32(DDRC_BASE + 0x00010108) = 0x00000005;
	REG32(DDRC_BASE + 0x00010118) = 0x00000001;
	REG32(DDRC_BASE + 0x00010180) = 0x00020000;
	REG32(DDRC_BASE + 0x00010184) = 0x00000002;
	REG32(DDRC_BASE + 0x0001018c) = 0x00000000;
	REG32(DDRC_BASE + 0x00010200) = 0x010003d0;
	REG32(DDRC_BASE + 0x00010220) = 0x1f000600;
	REG32(DDRC_BASE + 0x00010224) = 0x00000009;
	REG32(DDRC_BASE + 0x00010300) = 0x00570057;
	REG32(DDRC_BASE + 0x00010308) = 0x00000001;
	REG32(DDRC_BASE + 0x00010380) = 0xa8002010;
	REG32(DDRC_BASE + 0x00010384) = 0x80002000;
	REG32(DDRC_BASE + 0x00010390) = 0x083c0810;
	REG32(DDRC_BASE + 0x00010500) = 0x00110111;

	REG32(DDRC_BASE + 0x00010508) = 0xc0000000;
	REG32(DDRC_BASE + 0x00010510) = 0x00010005;
	REG32(DDRC_BASE + 0x00010518) = 0xf6000000;
	REG32(DDRC_BASE + 0x00010b80) = 0x00000001;
	REG32(DDRC_BASE + 0x00010c90) = 0x0000000a;

	REG32(DDRC_BASE + 0x00010c94) = 0x00000003;

	REG32(DDRC_BASE + 0x00010d00) = 0xc0030007;

	REG32(DDRC_BASE + 0x00010f00) = 0x00186180;
	REG32(DDRC_BASE + 0x00020004) = 0x00002000;
	REG32(DDRC_BASE + 0x00020008) = 0x00006000;
	REG32(DDRC_BASE + 0x00020094) = 0x00010000;
	REG32(DDRC_BASE + 0x00020098) = 0x005f0349;
	REG32(DDRC_BASE + 0x0002009c) = 0x01100b0a;
	REG32(DDRC_BASE + 0x000200a0) = 0x05d7048c;

	REG32(DDRC_BASE + 0x000200c0) = 0x00000008; // SARBASE0
	REG32(DDRC_BASE + 0x000200c4) = 0x0000000f; // SARSIZE0
	REG32(DDRC_BASE + 0x000200c8) = 0x00000018; // SARBASE1
	REG32(DDRC_BASE + 0x000200cc) = 0x0000000f; // SARSIZE1
	REG32(DDRC_BASE + 0x000200d0) = 0x00000028; // SARBASE2
	REG32(DDRC_BASE + 0x000200d4) = 0x0000000f; // SARSIZE2
	REG32(DDRC_BASE + 0x000200d8) = 0x00000038; // SARBASE3
	REG32(DDRC_BASE + 0x000200dc) = 0x0000000f; // SARSIZE3

	REG32(DDRC_BASE + 0x00021004) = 0x00000000;
	REG32(DDRC_BASE + 0x00021008) = 0x00000000;
	REG32(DDRC_BASE + 0x00021094) = 0x00110001;
	REG32(DDRC_BASE + 0x00021098) = 0x045400d4;
	REG32(DDRC_BASE + 0x0002109c) = 0x01000702;
	REG32(DDRC_BASE + 0x000210a0) = 0x05fb01ae;
	REG32(DDRC_BASE + 0x00022004) = 0x00004000;
	REG32(DDRC_BASE + 0x00022008) = 0x00006000;
	REG32(DDRC_BASE + 0x00022094) = 0x00000002;
	REG32(DDRC_BASE + 0x00022098) = 0x07f800fa;
	REG32(DDRC_BASE + 0x0002209c) = 0x01000a06;
	REG32(DDRC_BASE + 0x000220a0) = 0x06490719;
	REG32(DDRC_BASE + 0x00023004) = 0x00004000;
	REG32(DDRC_BASE + 0x00023008) = 0x00006000;
	REG32(DDRC_BASE + 0x00023094) = 0x00110003;
	REG32(DDRC_BASE + 0x00023098) = 0x002a06fd;
	REG32(DDRC_BASE + 0x0002309c) = 0x00110805;
	REG32(DDRC_BASE + 0x000230a0) = 0x00b906a2;
	REG32(DDRC_BASE + 0x00024004) = 0x00000000;
	REG32(DDRC_BASE + 0x00024008) = 0x00004000;
	REG32(DDRC_BASE + 0x00024094) = 0x00220000;
	REG32(DDRC_BASE + 0x00024098) = 0x061c0524;
	REG32(DDRC_BASE + 0x0002409c) = 0x01110c00;
	REG32(DDRC_BASE + 0x000240a0) = 0x04a80471;
	REG32(DDRC_BASE + 0x00000000) = 0x28103622;
	REG32(DDRC_BASE + 0x00000004) = 0x00060830;
	REG32(DDRC_BASE + 0x00000008) = 0x09110e17;
	REG32(DDRC_BASE + 0x0000000c) = 0x000c212f;
	REG32(DDRC_BASE + 0x00000010) = 0x0f04040f;
	REG32(DDRC_BASE + 0x00000014) = 0x02040c09;
	REG32(DDRC_BASE + 0x00000018) = 0x00000008;
	REG32(DDRC_BASE + 0x0000001c) = 0x00000003;
	REG32(DDRC_BASE + 0x00000024) = 0x00020410;
	REG32(DDRC_BASE + 0x00000030) = 0x00030000;
	REG32(DDRC_BASE + 0x00000034) = 0x0c100002;
	REG32(DDRC_BASE + 0x00000038) = 0x0020006e;
	REG32(DDRC_BASE + 0x0000005c) = 0x009d0009;
	REG32(DDRC_BASE + 0x00000060) = 0x000c160e;
	REG32(DDRC_BASE + 0x00000064) = 0x00002806;
	REG32(DDRC_BASE + 0x00000078) = 0x00191018;
	REG32(DDRC_BASE + 0x00000500) = 0x00000510;
	REG32(DDRC_BASE + 0x00000504) = 0x00000000;
	REG32(DDRC_BASE + 0x00000508) = 0x00000000;
	REG32(DDRC_BASE + 0x0000050c) = 0x00000000;
	REG32(DDRC_BASE + 0x00000580) = 0x033f021f;
	REG32(DDRC_BASE + 0x00000584) = 0x00080303;
	REG32(DDRC_BASE + 0x00000588) = 0x00183f1f;
	REG32(DDRC_BASE + 0x00000590) = 0x180c0411;
	REG32(DDRC_BASE + 0x00000594) = 0x0410000f;
	REG32(DDRC_BASE + 0x000005a0) = 0x00020202;
	REG32(DDRC_BASE + 0x000005a4) = 0x00000201;
	REG32(DDRC_BASE + 0x000005a8) = 0x0190000c;
	REG32(DDRC_BASE + 0x000005ac) = 0x00450063;
	REG32(DDRC_BASE + 0x000005b0) = 0x000000d3;
	REG32(DDRC_BASE + 0x000005b4) = 0x6800000d;
	REG32(DDRC_BASE + 0x000005b8) = 0x00000147;
	REG32(DDRC_BASE + 0x00000600) = 0xc03f0c34;
	REG32(DDRC_BASE + 0x00000604) = 0x00680030;
	REG32(DDRC_BASE + 0x00000608) = 0x06490000;
	REG32(DDRC_BASE + 0x0000060c) = 0x03000000;
	REG32(DDRC_BASE + 0x00000650) = 0x00000088;
	REG32(DDRC_BASE + 0x00000800) = 0x001804d6;
	REG32(DDRC_BASE + 0x00000804) = 0x02800065;
	REG32(DDRC_BASE + 0x00000a80) = 0x00003c14;
	REG32(DDRC_BASE + 0x00000b00) = 0x68a70235;
	REG32(DDRC_BASE + 0x00000b04) = 0x1024100a;
	REG32(DDRC_BASE + 0x00000b08) = 0x00000033;
	REG32(DDRC_BASE + 0x00000b80) = 0x06210000;
	REG32(DDRC_BASE + 0x00000c00) = 0x000000bd;
	REG32(DDRC_BASE + 0x00000c88) = 0x0f000063;
	REG32(DDRC_BASE + 0x00000d00) = 0x00000001;
	REG32(DDRC_BASE + 0x00000d04) = 0x00000507;
	REG32(DDRC_BASE + 0x00000d08) = 0x00001002;
	REG32(DDRC_BASE + 0x00000d0c) = 0x01de0028;

	REG32(DDRC_BASE + 0x00030004) = 0x00000019; // ADDRMAPX
	REG32(DDRC_BASE + 0x0003000c) = 0x003f0903;
	REG32(DDRC_BASE + 0x00030010) = 0x00000101;
	REG32(DDRC_BASE + 0x00030014) = 0x1f030303;
	REG32(DDRC_BASE + 0x00030018) = 0x03030300;
	REG32(DDRC_BASE + 0x0003001c) = 0x1f1f0808;
	REG32(DDRC_BASE + 0x00030020) = 0x08080808;
	REG32(DDRC_BASE + 0x00030024) = 0x08080808;
	REG32(DDRC_BASE + 0x00030028) = 0x08080808;
	REG32(DDRC_BASE + 0x0003002c) = 0x00000808;
	REG32(DDRC_BASE + 0x00010b84) = 0x00000000;
	REG32(DDRC_BASE + 0x00020090) = 0x00000001;
	REG32(DDRC_BASE + 0x00021090) = 0x00000001;
	REG32(DDRC_BASE + 0x00022090) = 0x00000001;
	REG32(DDRC_BASE + 0x00023090) = 0x00000001;
	REG32(DDRC_BASE + 0x00024090) = 0x00000001;
	REG32(DDRC_BASE + 0x00010208) = 0x00000001;

	REG32(CFG_BASE + 0x18) |= (1 << rst_code); // RELEASE FOR DCLK
	REG32(CFG_BASE + 0x18) |= (1 << 1);

	REG32(DDRC_BASE + 0x00010180) = 0x00020000;
	REG32(DDRC_BASE + 0x00010100) = 0x00000020;
	REG32(DDRC_BASE + 0x00010c80) = 0x00000000;
	REG32(DDRC_BASE + 0x00010510) = 0x00010004;
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
	REG32(DDRC_BASE + 0x00010180) = 0x00020800;

	phyinit_lp5_6400_training_pre(DDRC_BASE);

	REG32(DDRC_BASE + 0x00010180) |= (0x1 << 11);

	REG32(DPHY_BASE + 0xd0000 * 4) = 0x1;
	REG32(DPHY_BASE + 0xd0099 * 4) = 0x9;
	REG32(DPHY_BASE + 0xd0099 * 4) = 0x1;
	REG32(DPHY_BASE + 0xd0099 * 4) = 0x0;
	major_message_all(DPHY_BASE);
	REG32(DPHY_BASE + 0xd0099 * 4) = 0x1;
	while (count)
		count--;

	REG32(DPHY_BASE + 0xd0000 * 4) = 0x0;

	phyinit_lp5_6400_training(DDRC_BASE);

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
	REG32(DDRC_BASE + 0x00010510) = 0x00010014;
	REG32(DDRC_BASE + 0x00010c80) = 0x00000001;
	read_data = REG32(DDRC_BASE + 0x00010c84);
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010c84);
	}

	REG32(DDRC_BASE + 0x00010c80) = 0x00000000;
	REG32(DDRC_BASE + 0x00010510) = 0x00010015;
	REG32(DDRC_BASE + 0x00010c80) = 0x00000001;
	read_data = REG32(DDRC_BASE + 0x00010c84);
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010c84);
	}

	REG32(DDRC_BASE + 0x00010180) = 0x00020000;
	read_data = REG32(DDRC_BASE + 0x00010014);
	while ((read_data & 0x00000003) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010014);
	}

	REG32(DDRC_BASE + 0x00000a80) = 0x00003c14;
	read_data = REG32(DDRC_BASE + 0x00010090);
	while ((read_data & 0x00000001) != 0x00000000) {
		read_data = REG32(DDRC_BASE + 0x00010090);
	}

	REG32(DDRC_BASE + 0x00010208) = 0x00000000;
	REG32(DDRC_BASE + 0x00010180) = 0x00020000;
	REG32(DDRC_BASE + 0x00010100) = 0x00000021;
	REG32(DDRC_BASE + 0x00000a80) = 0x00003c14;
	REG32(DDRC_BASE + 0x00010180) = 0x00020000;
	REG32(DDRC_BASE + 0x00010184) = 0x00000002;
	REG32(DDRC_BASE + 0x00010100) = 0x00000025;
#endif
}

void lpddr5_silicon_init(void)
{
	LogMsg(0, "=== start init_lpddr() ===\n");
	init_snps_lp45(DDR_CTRL_REG_BASE, 22);
	LogMsg(0, "=== finish init_lpddr() ===\n");

	LogMsg(0, "=== start init_lpddr1() ===\n");
	init_snps_lp45(DDR_CTRL1_REG_BASE, 22);
	LogMsg(0, "=== finish init_lpddr1() ===\n");
}
