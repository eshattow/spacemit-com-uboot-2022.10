// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 ASR Micro Limited
 */


#include <common.h>
#include <cpu_func.h>
#include <dm.h>
#include <errno.h>
#include <div64.h>
#include <fdtdec.h>
#include <init.h>
#include <log.h>
#include <ram.h>
#include <asm/cache.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <linux/sizes.h>
#include "ddr3_config.h"
#include <linux/delay.h>

#define DDR_FPGA_PHY
#define DDR_CHECK_SIZE			(0x4000)
#define DDR_CHECK_STEP			(0x2000)
#define DDR_CHECK_CNT			(0x1000)

#define REG32(x)                        (*((volatile uint32_t *)((uintptr_t)(x))))
#define LOGLEVEL 1
#define LogMsg(level, format, args...) \
	do { \
		if (level < LOGLEVEL) \
		printf(format, ##args); \
	} while (0)
static int test_pattern(fdt_addr_t base, fdt_size_t size)
{
	fdt_addr_t addr;
	fdt_size_t check_size;
	uint32_t offset;
	uint32_t *ddr_data = NULL;
	uint32_t *save_data;
	int err;

	err = 0;

	check_size = (DDR_CHECK_SIZE / DDR_CHECK_STEP) * DDR_CHECK_CNT;
	ddr_data = malloc(check_size);
	if (!ddr_data) {
		printf("test zone malloc fail size 0x%llx\n", check_size);
		return -1;
	}


	save_data = ddr_data;
	/* to avoid overlap important data as image or ramdump  */
	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			*save_data = readl((void*)addr + offset);
			save_data++;
		}
	}

	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			writel((uint32_t)(addr + offset), (void*)addr + offset);
		}
	}
	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			if (readl((void*)addr + offset) != (uint32_t)(addr + offset)) {
				printf("ddr check error %x vs %x\n", (uint32_t)(addr + offset), readl((void*)addr + offset));
				err++;
				if (err > 10)
					goto ERR_HANDLE;
			}
		}
	}

	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			writel((~(uint32_t)(addr + offset)), (void*)addr + offset);
		}
	}
	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			if (readl((void*)addr + offset) != (~(uint32_t)(addr + offset))) {
				printf("ddr check error %x vs %x\n", (uint32_t)(~(addr + offset)), readl((void*)addr + offset));
				err++;
				if (err > 10)
					goto ERR_HANDLE;

			}
		}
	}

ERR_HANDLE:
	save_data = ddr_data;
	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			writel(*save_data, (void*)addr + offset);
			save_data++;
		}
	}
	if (err == 0)
		printf("*********ch0 is pass\n");
	else
		printf("*********ch0 is fail!\n");

	free(ddr_data);

	return err;
}



void init_snps_mr(unsigned DDRC_BASE, unsigned mr)
{
	unsigned int rank;
	unsigned int read_data;
	for(rank=1; rank<=2; rank=rank+1){
		REG32(DDRC_BASE + 0x00010080 ) = (rank<<4);
		REG32(DDRC_BASE + 0x00010084 ) = mr;
		REG32(DDRC_BASE + 0x00010080 ) = (rank<<4)|(1<<31);
		read_data = REG32(DDRC_BASE + 0x00010090) ;
		while((read_data & 0x00000001) != 0x00000000) {
			read_data = REG32(DDRC_BASE + 0x00010090);
		}
	}
}

void init_snps_lp45(unsigned DDRC_BASE, unsigned rst_code) // ddr3 init
{
	unsigned int read_data;
		//unsigned int DDRC_BASE;
		unsigned int CFG_BASE;
		//ddrc addr
		//DDRC_BASE =  0xcb000000;
		//DPHY_BASE =  0xcb800000;
		//PERF_BASE =  0xcb400000;
		//     PERF_BASE = DDRC_BASE + 0x400000;
		CFG_BASE  = DDRC_BASE + 0x600000;

#ifndef DDR_FPGA_PHY
		REG32(CFG_BASE + 0xc) |= (0x1<<22) | (0x1<<16) | (0xff);
		read_data = REG32(CFG_BASE + 0x1c);
		while ((read_data & 0x00000001) != 0x1){
			read_data = REG32(CFG_BASE + 0x1c);
		}
#endif

		REG32(CFG_BASE + 0x18) &= ~(0x3f<<16); // div 0
	REG32(CFG_BASE + 0x18) |= (0x1<<19) | (0x3<<16) ; // sel 2, div 4
	REG32(CFG_BASE + 0x18) |= (1<<25); // fc
	read_data = REG32(CFG_BASE + 0x18);
		while ((read_data & 0x2000000) != 0x0){
			read_data = REG32(CFG_BASE + 0x18);
		}
	REG32(CFG_BASE + 0x18) |= 0x1;
		//     #include "snps_init_lp4_1066.c"
		REG32(DDRC_BASE + 0x00010b84 ) = 0x00000001;
		REG32(DDRC_BASE + 0x00010000 ) = 0x03080002;
		REG32(DDRC_BASE + 0x00010010 ) = 0x00000000;
		REG32(DDRC_BASE + 0x00010100 ) = 0x00000001;
		REG32(DDRC_BASE + 0x00010104 ) = 0x00000005;
		REG32(DDRC_BASE + 0x00010108 ) = 0x00000005;
		REG32(DDRC_BASE + 0x00010118 ) = 0x00000001;
		REG32(DDRC_BASE + 0x00010180 ) = 0x00020010;
		REG32(DDRC_BASE + 0x00010184 ) = 0x00000002;
		REG32(DDRC_BASE + 0x0001018c ) = 0x00000000;
		REG32(DDRC_BASE + 0x00010200 ) = 0x01000395;
		REG32(DDRC_BASE + 0x00010220 ) = 0x1f000501;
		REG32(DDRC_BASE + 0x00010224 ) = 0x00000025;
		REG32(DDRC_BASE + 0x00010280 ) = 0x20000000;
		REG32(DDRC_BASE + 0x00010288 ) = 0x00000001;
		REG32(DDRC_BASE + 0x00010300 ) = 0x004b004b;
		REG32(DDRC_BASE + 0x00010308 ) = 0x00000001;
		REG32(DDRC_BASE + 0x00010380 ) = 0xa800a010;
		REG32(DDRC_BASE + 0x00010384 ) = 0x80002000;
		REG32(DDRC_BASE + 0x00010390 ) = 0x083c0810;
		REG32(DDRC_BASE + 0x00010500 ) = 0x00110111;
		//REG32(DDRC_BASE + 0x00010508 ) = 0x40008000;
		REG32(DDRC_BASE + 0x00010508 ) = 0xc0000000;//disable ctlupd and phyupd for the device which will generate X
	REG32(DDRC_BASE + 0x00010510 ) = 0x00010005;
		REG32(DDRC_BASE + 0x00010518 ) = 0x29000000;
		REG32(DDRC_BASE + 0x00010b80 ) = 0x00000001;
		REG32(DDRC_BASE + 0x00010c90 ) = 0x00000006;
		REG32(DDRC_BASE + 0x00010d00 ) = 0x00020009;//do not skip sdram init
	REG32(DDRC_BASE + 0x00010f00 ) = 0x80186180;
		REG32(DDRC_BASE + 0x00020000 ) = 0x00000010;
		REG32(DDRC_BASE + 0x00020004 ) = 0x00004000;
		REG32(DDRC_BASE + 0x00020008 ) = 0x00004000;
		REG32(DDRC_BASE + 0x00020094 ) = 0x0021000a;
		REG32(DDRC_BASE + 0x00020098 ) = 0x06030758;
		REG32(DDRC_BASE + 0x0002009c ) = 0x01100d0b;
		REG32(DDRC_BASE + 0x000200a0 ) = 0x030d009d;

		REG32(DDRC_BASE + 0x000200c0 ) = 0x00000008;//SARBASE0
	REG32(DDRC_BASE + 0x000200c4 ) = 0x00000003;//SARSIZE0
	REG32(DDRC_BASE + 0x000200c8 ) = 0x0000000c;//SARBASE1
	REG32(DDRC_BASE + 0x000200cc ) = 0x00000003;//SARSIZE1
	REG32(DDRC_BASE + 0x000200d0 ) = 0x00000010;//SARBASE2
	REG32(DDRC_BASE + 0x000200d4 ) = 0x00000003;//SARSIZE2
	REG32(DDRC_BASE + 0x000200d8 ) = 0x00000014;//SARBASE3
	REG32(DDRC_BASE + 0x000200dc ) = 0x00000003;//SARSIZE3

		REG32(DDRC_BASE + 0x00021004 ) = 0x00006000;
		REG32(DDRC_BASE + 0x00021008 ) = 0x00002000;
		REG32(DDRC_BASE + 0x00021094 ) = 0x0011000c;
		REG32(DDRC_BASE + 0x00021098 ) = 0x03a90070;
		REG32(DDRC_BASE + 0x0002109c ) = 0x01110603;
		REG32(DDRC_BASE + 0x000210a0 ) = 0x02bd0791;
		REG32(DDRC_BASE + 0x00022004 ) = 0x00002000;
		REG32(DDRC_BASE + 0x00022008 ) = 0x00006000;
		REG32(DDRC_BASE + 0x00022094 ) = 0x00010005;
		REG32(DDRC_BASE + 0x00022098 ) = 0x043d0201;
		REG32(DDRC_BASE + 0x0002209c ) = 0x01000602;
		REG32(DDRC_BASE + 0x000220a0 ) = 0x07d3012e;
		REG32(DDRC_BASE + 0x00023004 ) = 0x00000000;
		REG32(DDRC_BASE + 0x00023008 ) = 0x00006000;
		REG32(DDRC_BASE + 0x00023094 ) = 0x00110000;
		REG32(DDRC_BASE + 0x00023098 ) = 0x0348060d;
		REG32(DDRC_BASE + 0x0002309c ) = 0x01100e0b;
		REG32(DDRC_BASE + 0x000230a0 ) = 0x002c01c1;

		REG32(DDRC_BASE + 0x00024004 ) = 0x00004000;//add p4 configurations
	REG32(DDRC_BASE + 0x00024008 ) = 0x00004000;
		REG32(DDRC_BASE + 0x00024094 ) = 0x0021000a;
		REG32(DDRC_BASE + 0x00024098 ) = 0x06030758;
		REG32(DDRC_BASE + 0x0002409c ) = 0x01100d0b;
		REG32(DDRC_BASE + 0x000240a0 ) = 0x030d009d;//config ends

		REG32(DDRC_BASE + 0x00000000 ) = 0x22162417;
		REG32(DDRC_BASE + 0x00000004 ) = 0x00050820;
		REG32(DDRC_BASE + 0x00000008 ) = 0x060a1717;
		REG32(DDRC_BASE + 0x0000000c ) = 0x000a1d2a;
		REG32(DDRC_BASE + 0x00000010 ) = 0x0a08060a;
		REG32(DDRC_BASE + 0x00000014 ) = 0x03050808;
		REG32(DDRC_BASE + 0x00000018 ) = 0x00000007;
		REG32(DDRC_BASE + 0x00000024 ) = 0x00040000;
		REG32(DDRC_BASE + 0x00000030 ) = 0x00030000;
		REG32(DDRC_BASE + 0x00000034 ) = 0x14200004;
		REG32(DDRC_BASE + 0x00000038 ) = 0x0016004a;
		REG32(DDRC_BASE + 0x00000060 ) = 0x00000000;
		REG32(DDRC_BASE + 0x00000064 ) = 0x00022208;
		REG32(DDRC_BASE + 0x00000078 ) = 0x00171d08;
		REG32(DDRC_BASE + 0x00000080 ) = 0x00000000;
#ifdef DDR_FPGA_PHY
		REG32(DDRC_BASE + 0x00000500 ) = 0x009c0011;
#else
		REG32(DDRC_BASE + 0x00000500 ) = 0x009c0009;
#endif
		REG32(DDRC_BASE + 0x00000508 ) = 0x0020004c;
		REG32(DDRC_BASE + 0x0000050c ) = 0x00000062;
		REG32(DDRC_BASE + 0x00000580 ) = 0x03050202;
		REG32(DDRC_BASE + 0x00000584 ) = 0x000b0303;
		REG32(DDRC_BASE + 0x00000588 ) = 0x00000502;
		REG32(DDRC_BASE + 0x000005a0 ) = 0x00020202;
		REG32(DDRC_BASE + 0x000005a4 ) = 0x00000201;
		REG32(DDRC_BASE + 0x000005a8 ) = 0x002f000c;
		REG32(DDRC_BASE + 0x000005ac ) = 0x0088008f;
		REG32(DDRC_BASE + 0x000005b0 ) = 0x000000f1;
		REG32(DDRC_BASE + 0x000005b4 ) = 0x4000000a;
		REG32(DDRC_BASE + 0x000005b8 ) = 0x000000ca;
		REG32(DDRC_BASE + 0x00000600 ) = 0xc0000822;
		REG32(DDRC_BASE + 0x00000604 ) = 0x00460020;
		REG32(DDRC_BASE + 0x00000608 ) = 0x00200000;
		REG32(DDRC_BASE + 0x0000060c ) = 0x03000000;
		REG32(DDRC_BASE + 0x00000650 ) = 0x0000005b;
		REG32(DDRC_BASE + 0x00000800 ) = 0x00100216;
		REG32(DDRC_BASE + 0x00000804 ) = 0x01b00055;
		REG32(DDRC_BASE + 0x00000a80 ) = 0x00003554;
		REG32(DDRC_BASE + 0x00000b00 ) = 0xdac3b8d3;
		REG32(DDRC_BASE + 0x00000b04 ) = 0x0b180b07;
		REG32(DDRC_BASE + 0x00000b08 ) = 0x00000022;
		REG32(DDRC_BASE + 0x00000b80 ) = 0x01b40000;
		REG32(DDRC_BASE + 0x00000c00 ) = 0x00000003;
		REG32(DDRC_BASE + 0x00000c88 ) = 0x0f000000;
		REG32(DDRC_BASE + 0x00000d04 ) = 0x00000c0f;
		REG32(DDRC_BASE + 0x00000d08 ) = 0x00001c12;
		REG32(DDRC_BASE + 0x00000d0c ) = 0x0142005f;
		/*REG32(DDRC_BASE + 0x00030004 ) = 0x00000017;//add 2 to add row 14&15
		  REG32(DDRC_BASE + 0x0003000c ) = 0x00070101;
		  REG32(DDRC_BASE + 0x00030010 ) = 0x00003f3f;
		  REG32(DDRC_BASE + 0x00030014 ) = 0x1f020202;
		  REG32(DDRC_BASE + 0x00030018 ) = 0x02020200;
		  REG32(DDRC_BASE + 0x0003001c ) = 0x1f1f0707;//add row 14&15
		  REG32(DDRC_BASE + 0x00030020 ) = 0x07070707;
		  REG32(DDRC_BASE + 0x00030024 ) = 0x07070707;
		  REG32(DDRC_BASE + 0x00030028 ) = 0x07070707;
		  REG32(DDRC_BASE + 0x0003002c ) = 0x00000707;*/
		REG32(DDRC_BASE + 0x00030004 ) = 0x00000017;//add 2 to add row 14&15
	REG32(DDRC_BASE + 0x0003000c ) = 0x00070707;
		REG32(DDRC_BASE + 0x00030010 ) = 0x00003f3f;
		REG32(DDRC_BASE + 0x00030014 ) = 0x1f000000;
		REG32(DDRC_BASE + 0x00030018 ) = 0x00000000;
		REG32(DDRC_BASE + 0x0003001c ) = 0x1f1f0707;//add row 14&15
	REG32(DDRC_BASE + 0x00030020 ) = 0x07070707;
		REG32(DDRC_BASE + 0x00030024 ) = 0x07070707;
		REG32(DDRC_BASE + 0x00030028 ) = 0x07070707;
		REG32(DDRC_BASE + 0x0003002c ) = 0x00000707;

		REG32(DDRC_BASE + 0x00010b84 ) = 0x00000000;
		REG32(DDRC_BASE + 0x00020090 ) = 0x00000001;
		REG32(DDRC_BASE + 0x00021090 ) = 0x00000001;
		REG32(DDRC_BASE + 0x00022090 ) = 0x00000001;
		REG32(DDRC_BASE + 0x00023090 ) = 0x00000001;
		REG32(DDRC_BASE + 0x00024090 ) = 0x00000001;
		REG32(DDRC_BASE + 0x00010208 ) = 0x00000001;
		REG32(CFG_BASE + 0x18) |= (1<<rst_code);
		REG32(CFG_BASE + 0x18) &= ~(1<<1);
		/*REG32(PMUA_MC_HW_SLP_TYPE) |= (1<<rst_code); // RELEASE FOR DCLK
#ifndef DDR2_NORMAL_INIT
REG32(PMUA_MC_HW_SLP_TYPE) |= (1<<23);
		//REG32(PMUA_MC_HW_SLP_TYPE) |= (1<<27);
		//REG32(PMUA_MC_HW_SLP_TYPE) |= (1<<28);
#endif*/

		REG32(DDRC_BASE + 0x00010180 ) = 0x00020000;
		REG32(DDRC_BASE + 0x00010100 ) = 0x00000000;
		REG32(DDRC_BASE + 0x00010c80 ) = 0x00000000;
		REG32(DDRC_BASE + 0x00010510 ) = 0x00010004;
		REG32(DDRC_BASE + 0x00010c80 ) = 0x00000001;
		read_data = REG32(DDRC_BASE + 0x00010c84) ;
		while((read_data & 0x00000001) != 0x00000001) {
			read_data = REG32(DDRC_BASE + 0x00010c84);
		}
	REG32(DDRC_BASE + 0x00010c80 ) = 0x00000000;
		REG32(DDRC_BASE + 0x00010510 ) = 0x00010014;
		REG32(DDRC_BASE + 0x00010c80 ) = 0x00000001;
		read_data = REG32(DDRC_BASE + 0x00010c84) ;
		while((read_data & 0x00000001) != 0x00000001) {
			read_data = REG32(DDRC_BASE + 0x00010c84);
		}
	REG32(DDRC_BASE + 0x00010208 ) = 0x00000000;

		//phyinit_lp4_1066(DPHY_BASE);

		REG32(DDRC_BASE + 0x00010c80 ) = 0x00000000;
		REG32(DDRC_BASE + 0x00010510 ) = 0x00010034;
		REG32(DDRC_BASE + 0x00010c80 ) = 0x00000001;
		read_data = REG32(DDRC_BASE + 0x00010c84) ;
		while((read_data & 0x00000001) != 0x00000001) {
			read_data = REG32(DDRC_BASE + 0x00010c84);
		}

		read_data = REG32(DDRC_BASE + 0x00010514) ;
		while((read_data & 0x00000001) != 0x00000001) {
			read_data = REG32(DDRC_BASE + 0x00010514);
		}
	REG32(DDRC_BASE + 0x00010c80 ) = 0x00000000;
		REG32(DDRC_BASE + 0x00010510 ) = 0x00010014;
		REG32(DDRC_BASE + 0x00010c80 ) = 0x00000001;
		read_data = REG32(DDRC_BASE + 0x00010c84) ;
		while((read_data & 0x00000001) != 0x00000001) {
			read_data = REG32(DDRC_BASE + 0x00010c84);
		}
	REG32(DDRC_BASE + 0x00010c80 ) = 0x00000000;
		REG32(DDRC_BASE + 0x00010510 ) = 0x00010015;
		REG32(DDRC_BASE + 0x00010c80 ) = 0x00000001;
		read_data = REG32(DDRC_BASE + 0x00010c84) ;
		while((read_data & 0x00000001) != 0x00000001) {
			read_data = REG32(DDRC_BASE + 0x00010c84);
		}
	REG32(DDRC_BASE + 0x00010180 ) = 0x00020000;
		read_data = REG32(DDRC_BASE + 0x00010014) ;
		while((read_data & 0x00000003) != 0x00000001) {
			read_data = REG32(DDRC_BASE + 0x00010014);
		}
	REG32(DDRC_BASE + 0x00000a80 ) = 0x00003554;
		//X:read address 0x00010090 and get 0x00000000;
		REG32(DDRC_BASE + 0x00010080 ) = 0x00000010;
		REG32(DDRC_BASE + 0x00010084 ) = 0x00001740;
		REG32(DDRC_BASE + 0x00010080 ) = 0x80000010;
		read_data = REG32(DDRC_BASE + 0x00010090) ;
		while((read_data & 0x00000001) != 0x00000000) {
			read_data = REG32(DDRC_BASE + 0x00010090);
		}
	REG32(DDRC_BASE + 0x00010080 ) = 0x00000020;
		REG32(DDRC_BASE + 0x00010084 ) = 0x00001740;
		REG32(DDRC_BASE + 0x00010080 ) = 0x80000020;
		REG32(DDRC_BASE + 0x00010208 ) = 0x00000000;
		REG32(DDRC_BASE + 0x00010180 ) = 0x00020010;
		REG32(DDRC_BASE + 0x00010100 ) = 0x00000001;
		REG32(DDRC_BASE + 0x00000a80 ) = 0x00003554;
		/*REG32(DDRC_BASE + 0x00010180 ) = 0x00020010;
		  REG32(DDRC_BASE + 0x00010180 ) = 0x00020000;
		  REG32(DDRC_BASE + 0x00010184 ) = 0x00000002;
		  REG32(DDRC_BASE + 0x00010100 ) = 0x00000005;*/
		//X:read address 0x00010090 and get 0x00000000;
		//X:read address 0x00010b88 and get 0x36000000;


}

void ddr_init(void )
{
	LogMsg(0,"=== start init_lpddr() ===\n");
	init_snps_lp45(0xcb000000,22);
	LogMsg(0,"=== finish init_lpddr() ===\n");
	LogMsg(0,"=== start init_lpddr1() ===\n");
	init_snps_lp45(0xcc000000,23);
	LogMsg(0,"=== finish init_lpddr1() ===\n");
	// while(1);

}
static int spacemit_ddr_probe(struct udevice *dev){
	int ret;

	LogMsg(0,"=== start init_lpddr() ===\n");
	init_snps_lp45(0xcb000000,22);
	LogMsg(0,"=== finish init_lpddr() ===\n");
	LogMsg(0,"=== start init_lpddr1() ===\n");
	init_snps_lp45(0xcc000000,22);
	LogMsg(0,"=== finish init_lpddr1() ===\n");
	ret = test_pattern(CONFIG_SYS_SDRAM_BASE, DDR_CHECK_SIZE);
		if (ret < 0) {
		while(1);
	}
	printf("init done\n");
	return 0;
}

static const struct udevice_id spacemit_ddr_ids[] = {
	{ .compatible = "spacemit,snps-lp45" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(spacemit_ddr) = {
	.name = "spacemit_ddr_ctrl",
	.id = UCLASS_RAM,
	.of_match = spacemit_ddr_ids,
	.probe = spacemit_ddr_probe,
};