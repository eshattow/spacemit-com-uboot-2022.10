// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Spacemit
 */

#include <common.h>
#include <asm/cache.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <common.h>
#include <cpu_func.h>
#include <div64.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <fdtdec.h>
#include <init.h>
#include <linux/sizes.h>
#include <log.h>
#include <part.h>
#include <ram.h>

#include "k1_ddr.h"
#include "lpddr3_init_asic.h"

struct top_ddr_info{
	uint8_t vendor;
	uint8_t cs_num;
	uint8_t density_cs0;
	uint8_t density_cs1;
	uint8_t wds_odt;
	uint8_t rds_odt;
	uint8_t ca_ds_odt;
	uint8_t phy_rx_current;
};

extern uint32_t ddr_cs_num;

#ifndef CFG_SYS_NO_LOG_IN_TRACE
void top_Phy_reg_dump(unsigned DDRC_BASE, unsigned int fp)
{
	__maybe_unused unsigned DPHY0_BASE = DDRC_BASE + 0x040000;

	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (0xd4282800 + 0x398),
		REG32(0xd4282800 + 0x398));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (0xd4282800 + 0x3A4),
		REG32(0xd4282800 + 0x3A4));

	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + COMMON_OFFSET),
		REG32(DPHY0_BASE + COMMON_OFFSET));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + COMMON_OFFSET + subPHY_B_OFFSET),
		REG32(DPHY0_BASE + COMMON_OFFSET + subPHY_B_OFFSET));

	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x0064),
		REG32(DPHY0_BASE + 0x0064));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + FREQ_POINT_OFFSET + 0x0064),
		REG32(DPHY0_BASE + FREQ_POINT_OFFSET + 0x0064));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + FREQ_POINT_OFFSET * 2 + 0x0064),
		REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 2 + 0x0064));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + FREQ_POINT_OFFSET * 3 + 0x0064),
		REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 3 + 0x0064));

	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x0068),
		REG32(DPHY0_BASE + 0x0068));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + FREQ_POINT_OFFSET + 0x1068),
		REG32(DPHY0_BASE + FREQ_POINT_OFFSET + 0x1068));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + FREQ_POINT_OFFSET * 2 + 0x1068),
		REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 2 + 0x1068));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + FREQ_POINT_OFFSET * 3 + 0x1068),
		REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 3 + 0x1068));

	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + COMMON_OFFSET + 0x4),
		REG32(DPHY0_BASE + COMMON_OFFSET + 0x4));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4),
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4),
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4),
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4));

	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x4),
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x4));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4),
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4),
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4),
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4));

	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + COMMON_OFFSET + 0xc),
		REG32(DPHY0_BASE + COMMON_OFFSET + 0xc));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc),
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc),
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc),
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc));

	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0xc),
		REG32(DPHY0_BASE + COMMON_OFFSET + 0xc));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc),
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc),
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc),
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc));

	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + COMMON_OFFSET + 0x30),
		REG32(DPHY0_BASE + COMMON_OFFSET + 0x30));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n",
		(DPHY0_BASE + OTHER_CONTROL_OFFSET + 0x24),
		REG32(DPHY0_BASE + OTHER_CONTROL_OFFSET + 0x24));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + OTHER_CONTROL_OFFSET),
		REG32(DPHY0_BASE + OTHER_CONTROL_OFFSET));

	LogMsg(0, "\n\n \n");

	// #else

	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x70),
		REG32(DPHY0_BASE + 0x70));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x74),
		REG32(DPHY0_BASE + 0x74));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x78),
		REG32(DPHY0_BASE + 0x78));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x170),
		REG32(DPHY0_BASE + 0x170));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x1070),
		REG32(DPHY0_BASE + 0x1070));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x1074),
		REG32(DPHY0_BASE + 0x1074));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x1078),
		REG32(DPHY0_BASE + 0x1078));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x1170),
		REG32(DPHY0_BASE + 0x1170));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x3000),
		REG32(DPHY0_BASE + 0x3000));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x3004),
		REG32(DPHY0_BASE + 0x3004));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x3008),
		REG32(DPHY0_BASE + 0x3008));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x300c),
		REG32(DPHY0_BASE + 0x300c));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x3010),
		REG32(DPHY0_BASE + 0x3010));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x3014),
		REG32(DPHY0_BASE + 0x3014));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x3034),
		REG32(DPHY0_BASE + 0x3034));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x4070),
		REG32(DPHY0_BASE + 0x4070));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x4170),
		REG32(DPHY0_BASE + 0x4170));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x5070),
		REG32(DPHY0_BASE + 0x5070));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x5170),
		REG32(DPHY0_BASE + 0x5170));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x7004),
		REG32(DPHY0_BASE + 0x7004));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x7008),
		REG32(DPHY0_BASE + 0x7008));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x700c),
		REG32(DPHY0_BASE + 0x700c));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x7010),
		REG32(DPHY0_BASE + 0x7010));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x7014),
		REG32(DPHY0_BASE + 0x7014));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x8070),
		REG32(DPHY0_BASE + 0x8070));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x8170),
		REG32(DPHY0_BASE + 0x8170));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x9070),
		REG32(DPHY0_BASE + 0x9070));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0x9170),
		REG32(DPHY0_BASE + 0x9170));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xb004),
		REG32(DPHY0_BASE + 0xb004));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xb008),
		REG32(DPHY0_BASE + 0xb008));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xb00c),
		REG32(DPHY0_BASE + 0xb00c));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xb010),
		REG32(DPHY0_BASE + 0xb010));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xb014),
		REG32(DPHY0_BASE + 0xb014));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xc070),
		REG32(DPHY0_BASE + 0xc070));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xc170),
		REG32(DPHY0_BASE + 0xc170));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xd070),
		REG32(DPHY0_BASE + 0xd070));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xd170),
		REG32(DPHY0_BASE + 0xd170));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xf004),
		REG32(DPHY0_BASE + 0xf004));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xf008),
		REG32(DPHY0_BASE + 0xf008));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xf00c),
		REG32(DPHY0_BASE + 0xf00c));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xf010),
		REG32(DPHY0_BASE + 0xf010));
	LogMsg(0, "ADDR[0x%08X]=0x%08X \n", (DPHY0_BASE + 0xf014),
		REG32(DPHY0_BASE + 0xf014));

	unsigned i = 0;
	for (i = 0; i <= 10; i++) {
		LogMsg(0, "PMU ADDR[0x%08X]=0x%08X \n",
			(0xd4282800 + 0x398 + i * 4),
			REG32(0xd4282800 + 0x398 + i * 4));
	}
}
#endif

void DDR_lowerpower_HWDFC_flow_config(unsigned DDRC_BASE, unsigned cs_num)
{
	unsigned cs_select = 0;

	if (cs_num == 0x1) {
		cs_select = 0x1;
	} else {
		cs_select = 0x3;
	}

	// fill MC6 tables
	// remove all RDG training from table for SOC, by weima
	// LJ_DEBUG, program table DFC_LC;
	// DFC_TB Halt Scheduler and Set DFC Mode= 1!;
	REG32(DDRC_BASE + 0x74) = 0x00030b03;
	REG32(DDRC_BASE + 0x78) = 0x00000044;
	REG32(DDRC_BASE + 0x70) = 0x00000000;
	// Write Reg Tb 0.0: Addr: 44, Data: 30a03, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
	// DFC_TB User_CMD_0 in CH0001CS0001, reg=11000008!;
	REG32(DDRC_BASE + 0x74) = 0x10000008 | (cs_select << 24);
	REG32(DDRC_BASE + 0x78) = 0x00000020;
	REG32(DDRC_BASE + 0x70) = 0x00000001;
	// Write Reg Tb 0.1: Addr: 20, Data: 11000008, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
	REG32(DDRC_BASE + 0x74) = 0x00000004;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x00000002;
	// Write Reg Tb 0.2: Addr: 8, Data: 4, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	REG32(DDRC_BASE + 0x74) = 0x00000004;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x00000003;
	// Write Reg Tb 0.3: Addr: 8, Data: 4, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	REG32(DDRC_BASE + 0x74) = 0x10000001 | (cs_select << 24);
	REG32(DDRC_BASE + 0x78) = 0x000013d0;
	REG32(DDRC_BASE + 0x70) = 0x00000004;
	// Write Reg Tb 0.4: Addr: 13d0, Data: 11000001, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000005;
	// Write Reg Tb 0.5: Addr: 13fc, Data: 8000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	REG32(DDRC_BASE + 0x74) = 0x00000000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000006;
	// Write Reg Tb 0.6: Addr: 13fc, Data: 0, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	// DFC_TB Halt Scheduler and Set DFC Mode= 1!;
	REG32(DDRC_BASE + 0x74) = 0x00030b03;
	REG32(DDRC_BASE + 0x78) = 0x00010044;
	REG32(DDRC_BASE + 0x70) = 0x00000007;
	// Write Reg Tb 0.7: Addr: 44, Data: 30a03, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 1, EOP: 0, RD=0;
	REG32(DDRC_BASE + 0x74) = 0x10000100 | (cs_select << 24);
	REG32(DDRC_BASE + 0x78) = 0x000013d0;
	REG32(DDRC_BASE + 0x70) = 0x00000008;
	// Write Reg Tb 0.8: Addr: 13d0, Data: 10000100, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000009;
	// Write Reg Tb 0.9: Addr: 13fc, Data: 8000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x0000000a;
	// Write Reg Tb 0.10: Addr: 13fc, Data: 8000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	// DFC_TB Halt Scheduler and Set DFC Mode= 0!;
	REG32(DDRC_BASE + 0x74) = 0x00030b02;
	REG32(DDRC_BASE + 0x78) = 0x00010044;
	REG32(DDRC_BASE + 0x70) = 0x0000000b;
	// Write Reg Tb 0.11: Addr: 44, Data: 30a02, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 1, EOP: 0, RD=0;
	// DFC_TB User_CMD_0 in CH0001CS0001, reg=11000004!;
	REG32(DDRC_BASE + 0x74) = 0x10000004 | (cs_select << 24);
	REG32(DDRC_BASE + 0x78) = 0x00000020;
	REG32(DDRC_BASE + 0x70) = 0x0000000c;
	// Write Reg Tb 0.12: Addr: 20, Data: 11000004, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
	// DFC_TB MRW to Reg1 in CH0001CS0001;
	REG32(DDRC_BASE + 0x74) = 0x10020001 | (cs_select << 24);
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x0000000d;
	// Write Reg Tb 0.13: Addr: 24, Data: 11020001, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
	// DFC_TB MRW to Reg2 in CH0001CS0001;
	REG32(DDRC_BASE + 0x74) = 0x10020002 | (cs_select << 24);
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x0000000e;
	// Write Reg Tb 0.14: Addr: 24, Data: 11020002, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;

	// CH0 PHY CS1 write table for training RDG;
	REG32(DDRC_BASE + 0x74) = 0x11100000;
	REG32(DDRC_BASE + 0x78) = 0x000013d0;
	REG32(DDRC_BASE + 0x70) = 0x0000000f;
	// Write Reg Tb 0.15: Addr: 13d0, Data: 11100000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
	REG32(DDRC_BASE + 0x74) = 0x00000006;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000010;
	// Write Reg Tb 0.16: Addr: 13fc, Data: 6, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	REG32(DDRC_BASE + 0x74) = 0x00000006;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000011;
	// Write Reg Tb 0.17: Addr: 13fc, Data: 6, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	if (cs_num == 2) {
		// CH0 PHY CS1 write table for training RDG;
		REG32(DDRC_BASE + 0x74) = 0x12100000;
		REG32(DDRC_BASE + 0x78) = 0x000013d0;
		REG32(DDRC_BASE + 0x70) = 0x00000012;
		// Write Reg Tb 0.15: Addr: 13d0, Data: 11100000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
		REG32(DDRC_BASE + 0x74) = 0x00000006;
		REG32(DDRC_BASE + 0x78) = 0x000033fc;
		REG32(DDRC_BASE + 0x70) = 0x00000013;
		// Write Reg Tb 0.16: Addr: 13fc, Data: 6, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
		REG32(DDRC_BASE + 0x74) = 0x00000006;
		REG32(DDRC_BASE + 0x78) = 0x000033fc;
		REG32(DDRC_BASE + 0x70) = 0x00000014;

		REG32(DDRC_BASE + 0x74) = 0x00030b00;
		REG32(DDRC_BASE + 0x78) = 0x00020044;
		REG32(DDRC_BASE + 0x70) = 0x00000015;
	} else {
		// Write Reg Tb 0.18: Addr: 24, Data: 11010000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
		// DFC_TB Resume Scheduler and Clear DFC Mode= 0!;
		REG32(DDRC_BASE + 0x74) = 0x00030b00;
		REG32(DDRC_BASE + 0x78) = 0x00020044;
		REG32(DDRC_BASE + 0x70) = 0x00000012;
		// Write Reg Tb 0.19: Addr: 44, Data: 30a00, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 1, RD=0;
	}
	// DFC_TB MRR to Reg0 in CH0001CS0001;
	// W32((DDRC_BASE + 0x74) , 0x11010000);
	// W32((DDRC_BASE + 0x78) , 0x00000024);
	// W32((DDRC_BASE + 0x70) , 0x0000000f);

	//====================================================================================================
	// LJ_DEBUG, program table LP;
	// DFC_TB Halt Scheduler and Set DFC Mode= 1!;
	REG32(DDRC_BASE + 0x74) = 0x00030b03;
	REG32(DDRC_BASE + 0x78) = 0x00000044;
	REG32(DDRC_BASE + 0x70) = 0x00000060;
	// Write Reg Tb 3.0: Addr: 44, Data: 30a03, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
	REG32(DDRC_BASE + 0x74) = 0x00000004;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x00000061;
	// Write Reg Tb 3.1: Addr: 8, Data: 4, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	REG32(DDRC_BASE + 0x74) = 0x00000004;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x00000062;
	// Write Reg Tb 3.2: Addr: 8, Data: 4, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	REG32(DDRC_BASE + 0x74) = 0x10000001 | (cs_select << 24);
	REG32(DDRC_BASE + 0x78) = 0x000013d0;
	REG32(DDRC_BASE + 0x70) = 0x00000063;
	// Write Reg Tb 3.3: Addr: 13d0, Data: 11000001, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000064;
	// Write Reg Tb 3.4: Addr: 13fc, Data: 8000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	REG32(DDRC_BASE + 0x74) = 0x00000000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000065;
	// Write Reg Tb 3.5: Addr: 13fc, Data: 0, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	// DFC_TB Halt Scheduler and Set DFC Mode= 1!;
	REG32(DDRC_BASE + 0x74) = 0x00030b03;
	REG32(DDRC_BASE + 0x78) = 0x00010044;
	REG32(DDRC_BASE + 0x70) = 0x00000066;
	// Write Reg Tb 3.6: Addr: 44, Data: 30a03, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 1, EOP: 0, RD=0;
	REG32(DDRC_BASE + 0x74) = 0x10000100 | (cs_select << 24);
	REG32(DDRC_BASE + 0x78) = 0x000013d0;
	REG32(DDRC_BASE + 0x70) = 0x00000067;
	// Write Reg Tb 3.7: Addr: 13d0, Data: 10000100, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000068;
	// Write Reg Tb 3.8: Addr: 13fc, Data: 8000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000069;
	// Write Reg Tb 3.9: Addr: 13fc, Data: 8000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	// DFC_TB Halt Scheduler and Set DFC Mode= 0!;
	REG32(DDRC_BASE + 0x74) = 0x00030b02;
	REG32(DDRC_BASE + 0x78) = 0x00010044;
	REG32(DDRC_BASE + 0x70) = 0x0000006a;
	// Write Reg Tb 3.10: Addr: 44, Data: 30a02, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 1, EOP: 0, RD=0;
	/*
	// CH0 PHY CS1 write table for training RDG;
	W32((DDRC_BASE + 0x74) , 0x11100000);
	W32((DDRC_BASE + 0x78) , 0x000013d0);
	W32((DDRC_BASE + 0x70) , 0x0000006b;
	// Write Reg Tb 3.11: Addr: 13d0, Data: 11100000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
	W32((DDRC_BASE + 0x74) , 0x00000006);
	W32((DDRC_BASE + 0x78) , 0x000033fc);
	W32((DDRC_BASE + 0x70) , 0x0000006c);
	// Write Reg Tb 3.12: Addr: 13fc, Data: 6, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	W32((DDRC_BASE + 0x74) , 0x00000006);
	W32((DDRC_BASE + 0x78) , 0x000033fc);
	W32((DDRC_BASE + 0x70) , 0x0000006d);
	// Write Reg Tb 3.13: Addr: 13fc, Data: 6, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1;
	*/
	// DFC_TB MRR to Reg0 in CH0001CS0001;
	// W32((DDRC_BASE + 0x74) , 0x11010000);
	// W32((DDRC_BASE + 0x78) , 0x00000024);
	// W32((DDRC_BASE + 0x70) , 0x0000006b);
	// Write Reg Tb 3.14: Addr: 24, Data: 11010000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0;
	// DFC_TB Resume Scheduler and Clear DFC Mode= 0!;
	REG32(DDRC_BASE + 0x74) = 0x00030b00;
	REG32(DDRC_BASE + 0x78) = 0x00020044;
	REG32(DDRC_BASE + 0x70) = 0x0000006b;
	// Write Reg Tb 3.15: Addr: 44, Data: 30a00, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 1, RD=0;
}

void freq_point_timing_init_dove(unsigned DDRC_BASE)
{
	unsigned DPHY0_BASE = DDRC_BASE + 0x040000;

#if 1
	// high for 2133Mbps
	// FSP_WR = 2b'11
	REG32(DDRC_BASE + MC_CH0_BASE + 0x104) = 0xf0800000; // DRAM Config 2 FSP WR FSP OP FSP=2b'11

	// timing releated
	REG32(DDRC_BASE + MC_CH0_BASE + 0x100) = 0x00000810; // DRAM Config 1 RL/WL
	// read_pre:0 static  read_post:0 0.5nCK   wr_pre:1 2nCK   wr_post:1  1.5nCK
	REG32(DDRC_BASE + MC_CH0_BASE + 0x10c) = 0x00000050; // DRAM Config 4    //liliang-0x54
	REG32(DDRC_BASE + MC_CH0_BASE + 0x110) = 0x80020000; // DRAM Config 5 CS0 //ODT:60  device_str:40
	REG32(DDRC_BASE + MC_CH0_BASE + 0x114) = 0x80020000; // DRAM Config 5 CS1 //ODT:60     device_str:40

	REG32(DDRC_BASE + MC_CH0_BASE + 0x18c) = 0x0036042a; // ZQC timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x190) = 0x00600180; // ZQC timing 1

	// write32(DDRC_BASE+MC_CH0_BASE+0x194,0xC040008B);              //Refresh timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x194) = 0xC06000E0; // Refresh timing

	// write32(DDRC_BASE+MC_CH0_BASE+0x1fc,0x000D0066);              //Refresh timing1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1fc) = 0x00130096; // Refresh timing1

	// write32(DDRC_BASE+MC_CH0_BASE+0x198,0x00960096);        //SelfRefresh timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x198) = 0x00EB00EB; // SelfRefresh timing 0

	REG32(DDRC_BASE + MC_CH0_BASE + 0x19c) = 0x00101010; // SelfRefresh timing 1

	// write32(DDRC_BASE+MC_CH0_BASE+0x1a0,0x06100805);              //Power down timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a0) = 0x06100808; // Power down timing 0

	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a4) = 0x00000001; // Power down timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a8) = 0x0000020f; // MRS timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1ac) = 0x3642162f; // ACT timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b0) = 0x17180816; // Pre-Charge timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b4) = 0x04000800; // CAS/RAS timing 0

	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b8) = 0x00000D00; // CAS/RAS timing 1

	REG32(DDRC_BASE + MC_CH0_BASE + 0x1bc) = 0x02050404; // Off-spec timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c0) = 0x00000004; // Off-spec timing 1

	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c4) = 0x00000006; // DRAM_read timing
	// write32(DDRC_BASE+MC_CH0_BASE+0x1c8,0x00000a0a);      //CA Train timing
	REG32(DDRC_BASE + 0x16c) = 0x00002010; // AM3_TH,14:8 am_th_high,6:0 am_th_low
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1d8) = 0x0000812d; // CH0_dram_training_timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x14c) = 0x000c4090; // odt_control_3
#if defined(NEW_FEATURE)
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e4) = 0x09000502; // MCK6 DFI phy ctrl register 1 (4to1)
#else
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e4) = 0x09000700; // MCK6 DFI phy ctrl register 1 (4to1)
#endif
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3ec) = 0x0000046c; // CH0_DFI_PHY_Control_3 trdlvl_rr
// high for 2133Mbps end
#endif

#if 0
// high for 1866Mbps
//FSP_WR = 2b'11
	REG32(DDRC_BASE + MC_CH0_BASE + 0x104)= 0xf0800400;	//DRAM Config 2 FSP WR FSP OP FSP=2b'11
	//timing releated
	REG32(DDRC_BASE + MC_CH0_BASE + 0x100)= 0x0000080e;	//DRAM Config 1 RL/WL
	REG32(DDRC_BASE + MC_CH0_BASE + 0x10c)= 0x00000050;	//DRAM Config 4

	REG32(DDRC_BASE + MC_CH0_BASE + 0x110)= 0x80020000;	//DRAM Config 5 CS0 //ODT:60    device_str:40
	REG32(DDRC_BASE + MC_CH0_BASE + 0x114)= 0x80020000;	//DRAM Config 5 CS1 //ODT:60    device_str:40
	//write32(DDRC_BASE+MC_CH0_BASE+0x110,0x80220000);     //DRAM Config 5 CS0 //ODT:120  device_str:40
	//REG32(DDRC_BASE+MC_CH0_BASE+0x114)= 0x80220000;   //DRAM Config 5 CS1 //ODT:120     device_str:40

	REG32(DDRC_BASE + MC_CH0_BASE + 0x18c)= 0x002f03a5;	//ZQC timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x190)= 0x00540150;	//ZQC timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x194)= 0xc040008b;	//Refresh timing

	REG32(DDRC_BASE + MC_CH0_BASE + 0x1fc)= 0x00130096;	//Refresh timing1

	REG32(DDRC_BASE + MC_CH0_BASE + 0x198)= 0x00ce00ce;	//SelfRefresh timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x19c)= 0x00100e0e;	//SelfRefresh timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a0)= 0x020e0707;	//Power down timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a4)= 0x00000001;	//Power down timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a8)= 0x0000020e;	//MRS timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1ac)= 0x2f3b1128;	//ACT timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b0)= 0x140e0711;	//Pre-Charge timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b4)= 0x04000700;	//CAS/RAS timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b8)= 0x00000a00;	//CAS/RAS timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1bc)= 0x02050404;	//Off-spec timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c0)= 0x00000004;	//Off-spec timing 1

	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c4)= 0x00000006;	//DRAM_read timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c8) = 0x85e17a0a;	//CA Train timing
	//write32(DDRC_BASE+MC_CH0_BASE+0x1d8,0x0000812d);              //CH0_dram_training_timing
	//write32(DDRC_BASE+MC_CH0_BASE+0x14c,0x000c4090);              //odt_control_3
#if defined(NEW_FEATURE)
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e4)= 0x07000502;
#else
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e4)= 0x07000700;	//MCK6 DFI phy ctrl register 1 (4to1)
#endif
	//write32(DDRC_BASE+MC_CH0_PHY_BASE+0x3ec,0x0000046c);     //CH0_DFI_PHY_Control_3 trdlvl_rr
	REG32(DDRC_BASE + 0x16c)= 0x00002010;	//AM3_TH,14:8 am_th_high,6:0 am_th_low
// high for 2133Mbps end
#endif

	// high for 1600Mbps
	// FSP_WR =10
	REG32(DDRC_BASE + MC_CH0_BASE + 0x104) = 0xa0800000; // DRAM Config 2
	REG32(DDRC_BASE + MC_CH0_BASE + 0x100) = 0x0000060c; // DRAM Config 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x10c) = 0x00000050; // DRAM Config 4
	// write32(DDRC_BASE+MC_CH0_BASE+0x110,0x80120000);    //DRAM Config 5 CS0 //ODT:60  device_str:40
	// write32(DDRC_BASE+MC_CH0_BASE+0x114,0x80120000);   //DRAM Config 5 CS1 //ODT:60     device_str:40
	REG32(DDRC_BASE + MC_CH0_BASE + 0x110) = 0x80020000; // DRAM Config 5 CS0 //ODT:120  device_str:40
	REG32(DDRC_BASE + MC_CH0_BASE + 0x114) = 0x80020000; // DRAM Config 5 CS1 //ODT:120  device_str:40
	// REG32(DDRC_BASE+MC_CH0_BASE+0x180) = 0x00000400;         //DDR init timing Control 0
	// REG32(DDRC_BASE+MC_CH0_BASE+0x184) = 0x0000006b;         //DDR init timing Control 1
	// REG32(DDRC_BASE+MC_CH0_BASE+0x188) = 0x09600080;         //DDR init timing Control 2
	REG32(DDRC_BASE + MC_CH0_BASE + 0x18c) = 0x00280320; // ZQC timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x190) = 0x00480120; // ZQC timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x194) = 0xC04800A8; // Refresh timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1fc) = 0x00130096; // Refresh timing1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x198) = 0x00b000b0; // SelfRefresh timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x19c) = 0x000c0c0c; // SelfRefresh timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a0) = 0x060c0606; // Power down timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a4) = 0x00000001; // Power down timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a8) = 0x0000020c; // MRS timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1ac) = 0x28321024; // ACT timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b0) = 0x11120610; // Pre-Charge timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b4) = 0x04000600; // CAS/RAS timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b8) = 0x00000a00; // CAS/RAS timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1bc) = 0x02040302; // Off-spec timing 0
	// REG32(DDRC_BASE+MC_CH0_BASE+0x1c0)= 0x00000003;   //Off-spec timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c0) = 0x00000004; // Off-spec timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c4) = 0x00000005; // DRAM_read timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c8) = 0x85e17a0a; // CA Train timing
#if defined(NEW_FEATURE)
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e4) = 0x06000302; // MCK6 DFI phy ctrl register 1 (4to1)
#else
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e4) = 0x06000500; // MCK6 DFI phy ctrl register 1 (4to1)
#endif
	REG32(DDRC_BASE + 0x16C) = 0x00002010; // AM3_TH, 14:8 am_th_high, 6:0 am_th_low
	// high for 1600Mbps end

	// high for 1066Mbps
	//  FSP_WR =01
	REG32(DDRC_BASE + MC_CH0_BASE + 0x104) = 0x50800000; // DRAM Config 2
	REG32(DDRC_BASE + MC_CH0_BASE + 0x100) = 0x00000408; // DRAM Config 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x10c) = 0x00000050; // DRAM Config 4
	// write32(DDRC_BASE+MC_CH0_BASE+0x110,0x80120000);    //DRAM Config 5 CS0 //ODT:60  device_str:40
	// write32(DDRC_BASE+MC_CH0_BASE+0x114,0x80120000);   //DRAM Config 5 CS1 //ODT:60     device_str:40
	REG32(DDRC_BASE + MC_CH0_BASE + 0x110) = 0x80020000; // DRAM Config 5 CS0 //ODT:120  device_str:40
	REG32(DDRC_BASE + MC_CH0_BASE + 0x114) = 0x80020000; // DRAM Config 5 CS1 //ODT:120  device_str:40
	// REG32(DDRC_BASE+MC_CH0_BASE+0x180) = 0x00000400;         //DDR init timing Control 0
	// REG32(DDRC_BASE+MC_CH0_BASE+0x184) = 0x0000006b;         //DDR init timing Control 1
	// REG32(DDRC_BASE+MC_CH0_BASE+0x188) = 0x09600080;         //DDR init timing Control 2
	REG32(DDRC_BASE + MC_CH0_BASE + 0x18c) = 0x001b0215; // ZQC timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x190) = 0x003000c0; // ZQC timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x194) = 0x00300070; // Refresh timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1fc) = 0x00130096; // Refresh timing1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x198) = 0x00760076; // SelfRefresh timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x19c) = 0x00080808; // SelfRefresh timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a0) = 0x06080404; // Power down timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a4) = 0x00000001; // Power down timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a8) = 0x0000020a; // MRS timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1ac) = 0x1b200a17; // ACT timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b0) = 0x0c0c040a; // Pre-Charge timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b4) = 0x04000400; // CAS/RAS timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b8) = 0x00000600; // CAS/RAS timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1bc) = 0x00040302; // Off-spec timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c0) = 0x00000003; // Off-spec timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c4) = 0x00000003; // DRAM_read timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c8) = 0x85E17A0A; // CA Train timing
#if defined(NEW_FEATURE)
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e4) = 0x04000102; // MCK6 DFI phy ctrl register 1 (4to1)
#else
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e4) = 0x04000300; // MCK6 DFI phy ctrl register 1 (4to1)
#endif
	REG32(DDRC_BASE + 0x16C) = 0x00002010; // AM3_TH, 14:8 am_th_high, 6:0 am_th_low
	// high for 1066Mbps end

	// high for 533Mbps
	//  FSP_WR =00
	REG32(DDRC_BASE + MC_CH0_BASE + 0x104) = 0x00800000; // DRAM Config 2
	REG32(DDRC_BASE + MC_CH0_BASE + 0x100) = 0x00000306; // DRAM Config 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x10c) = 0x00000050; // DRAM Config 4
	// write32(DDRC_BASE+MC_CH0_BASE+0x110,0x80120000);    //DRAM Config 5 CS0 //ODT:60  device_str:40
	// write32(DDRC_BASE+MC_CH0_BASE+0x114,0x80120000);   //DRAM Config 5 CS1 //ODT:60     device_str:40
	REG32(DDRC_BASE + MC_CH0_BASE + 0x110) = 0x80020000; // DRAM Config 5 CS0 //ODT:120  device_str:40
	REG32(DDRC_BASE + MC_CH0_BASE + 0x114) = 0x80020000; // DRAM Config 5 CS1 //ODT:120  device_str:40
	// REG32(DDRC_BASE+MC_CH0_BASE+0x180) = 0x00000400;         //DDR init timing Control 0
	// REG32(DDRC_BASE+MC_CH0_BASE+0x184) = 0x0000006b;         //DDR init timing Control 1
	// REG32(DDRC_BASE+MC_CH0_BASE+0x188) = 0x09600080;         //DDR init timing Control 2
	REG32(DDRC_BASE + MC_CH0_BASE + 0x18c) = 0x00140190; // ZQC timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x190) = 0x00240090; // ZQC timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x194) = 0x00240054; // Refresh timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1fc) = 0x00130096; // Refresh timing1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x198) = 0x00580058; // SelfRefresh timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x19c) = 0x00060606; // SelfRefresh timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a0) = 0x06060303; // Power down timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a4) = 0x00000001; // Power down timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1a8) = 0x0000020A; // MRS timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1ac) = 0x14180811; // ACT timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b0) = 0x09090408; // Pre-Charge timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b4) = 0x04000400; // CAS/RAS timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1b8) = 0x00000400; // CAS/RAS timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1bc) = 0x00040302; // Off-spec timing 0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c0) = 0x00000003; // Off-spec timing 1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c4) = 0x00000003; // DRAM_read timing
	REG32(DDRC_BASE + MC_CH0_BASE + 0x1c8) = 0x85E17A0A; // CA Train timing
#if defined(NEW_FEATURE)
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e4) = 0x02000002; // MCK6 DFI phy ctrl register 1 (4to1)
#else
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e4) = 0x02000200; // MCK6 DFI phy ctrl register 1 (4to1)
#endif
	REG32(DDRC_BASE + 0x16C) = 0x00002010; // AM3_TH, 14:8 am_th_high, 6:0 am_th_low
	// high for 533Mbps end

	// digital phy rd_plus1
	REG32(DPHY0_BASE + 0x0064) |= 0x1 << 12;
	// REG32(DPHY0_BASE+0x4064) |= 0x0 <<12;
	// REG32(DPHY0_BASE+0x8064) |= 0x0 <<12;
	// REG32(DPHY0_BASE+0xc064) |= 0x0 <<12;

	return;
}

void freq_point_sel(unsigned DDRC_BASE, unsigned int fp)
{
	uint32_t data;
	data = REG32(DDRC_BASE + MC_CH0_BASE + 0x104);
	data &= ~(0xf << 28);
	data |= (fp << 28) | (fp << 30);
	REG32(DDRC_BASE + MC_CH0_BASE + 0x104) = data;
	return;
}

void DDR_MC_init(unsigned DDRC_BASE, unsigned int fp, unsigned cs_num)
{

	/*Init MC misc register */
	REG32(DDRC_BASE + 0x44) = 0x00030B00; /*Data_Width:x32, Burst_Length: BL8 */

#ifdef PERBANK_REF
	REG32(DDRC_BASE + 0x44) = REG32((DDRC_BASE + 0x44) | 0xa000); // MC_Control_0  //enable pb-refresh mode and out of order refresh
#endif

	REG32(DDRC_BASE + 0x48) = 0x00000001; /*exclu_en: Enable exclusive access monitoring */
	REG32(DDRC_BASE + 0x4C) = 0x00000000;

	REG32(DDRC_BASE + 0x64) = 0x10070504;
	REG32(DDRC_BASE + 0x68) |= (0x1 << 6); // disable dclk auto stop

	REG32(DDRC_BASE + 0x50) = 0x00100aff; /*spool_2cycle_mode:1, starv_timer_init: 0x3F */
	REG32(DDRC_BASE + 0x54) = 0x00000480;
	REG32(DDRC_BASE + 0x58) = 0x10351035; // 0x3fd43fd4;
	REG32(DDRC_BASE + 0x5C) = 0x000494e4;
	REG32(DDRC_BASE + 0x148) = 0xc0a30000;
	REG32(DDRC_BASE + 0x180) = 0x00030200; /*rpp_starvation_en: 1 enable */

	// REG32(ROB_Control) |= (0x1<<7);

	/*Init memory address map */
	/*Memory Address Map Register Low CS0, CS0 Area length: 2GB, 16Gbit,start address: 0 */
	REG32(DDRC_BASE + MC_CH0_BASE) = 0xF0001;
	// REG32(DDRC_BASE+MC_CH0_BASE)= 0xE0001;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x4) = 0x0;
	/*Memory Address Map Register Low CS1, CS1 Area length: 2GB, 16Gbit,start address:0x100000000 */
	REG32(DDRC_BASE + MC_CH0_BASE + 0x8) = 0xF0001;
	// REG32(DDRC_BASE+MC_CH0_BASE+0x8)= 0x400E0001;
	REG32(DDRC_BASE + MC_CH0_BASE + 0xC) = 0x1;

	REG32(DDRC_BASE + 0x0080) = 0x00000000; // close TZ filter
	REG32(DDRC_BASE + 0x0a00) = 0x00000000; // change range0 nsaid mask
	REG32(DDRC_BASE + 0x0ac0) = 0x00000000; // change undefined nsaid mask
	REG32(DDRC_BASE + 0x0acc) = 0xffffffff; // open TZ intrrupt and resp

	/*Init Bank,row,column */
	/*MC Configuration      CS0:
	Number of Banks: 8
	Number of Bank groups: 1
	Number of Column address bits: 11
	Number of Row address bits:15
	Number of Stack Chips:1
	Bank address assignment boundary:8KB
	DDR device type: X32
	*/
	REG32(DDRC_BASE + MC_CH0_BASE + 0x20) = 0x06030542;

	/*MC Configuration      CS1:
	Number of Banks: 8
	Number of Bank groups: 1
	Number of Column address bits: 11
	Number of Row address bits:15
	Number of Stack Chips:1
	Bank address assignment boundary:8KB
	DDR device type: X32
	*/
	REG32(DDRC_BASE + MC_CH0_BASE + 0x24) = 0x06030542;

	/*Init MC feature */
	/*MC_Control_1:
	tw2r_dis: enable tW2R
	acs_en: Enable auto clock stop mode
	aps_ppd: Active Power-down
	*/
	REG32(DDRC_BASE + MC_CH0_BASE + 0xC0) = 0x6000;

	/*SDRAM_type: LPDDR3 */
	REG32(DDRC_BASE + MC_CH0_BASE + 0xC4) = 0x000000a0;

	/*Configure 2 frequency point timing */
	freq_point_timing_init_dove(DDRC_BASE);

	/*select a frequency point to run */
	freq_point_sel(DDRC_BASE, fp);

	/*Init DDR init timing */
	/*
	DDR init timing Control 0: init_count_nop
	DDR init timing Control 1: init_count
	DDR init timing Control 2: reset_count, cke_count
	*/
	switch (fp) { // Power Ramp and Initialization Sequence in JESD
	case 0:
		REG32(DDRC_BASE + MC_CH0_BASE + 0x180) = 0x340D0;
		REG32(DDRC_BASE + MC_CH0_BASE + 0x184) = 0x0006B;
		REG32(DDRC_BASE + MC_CH0_BASE + 0x188) = 0x029A4;
		// REG32(DDRC_BASE+MC_CH0_BASE+0x180)= 0x13880;
		// REG32(DDRC_BASE+MC_CH0_BASE+0x184)= 0x28;
		// REG32(DDRC_BASE+MC_CH0_BASE+0x188)= 0xFA0;

		REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e0) &= ~(0x3 << 2);
		REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e0) = REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e0) | (0 << 2); /*tell phy boot from frequency point 0 */
		break;
	case 1:
		REG32(DDRC_BASE + MC_CH0_BASE + 0x180) = 0x340D0;
		REG32(DDRC_BASE + MC_CH0_BASE + 0x184) = 0x0006B;
		REG32(DDRC_BASE + MC_CH0_BASE + 0x188) = 0x029A4;
		REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e0) &= ~(0x3 << 2);
		REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e0) = REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e0) | (1 << 2); /*tell phy boot from frequency point 1 */
		break;
	case 2:
		REG32(DDRC_BASE + MC_CH0_BASE + 0x180) = 0x340D0;
		REG32(DDRC_BASE + MC_CH0_BASE + 0x184) = 0x0006B;
		REG32(DDRC_BASE + MC_CH0_BASE + 0x188) = 0x029A4;
		REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e0) &= ~(0x3 << 2);
		REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e0) = REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e0) | (2 << 2); /*tell phy boot from frequency point 1 */
		break;
	case 3:
		REG32(DDRC_BASE + MC_CH0_BASE + 0x180) = 0x340D0;
		REG32(DDRC_BASE + MC_CH0_BASE + 0x184) = 0x0006B;
		REG32(DDRC_BASE + MC_CH0_BASE + 0x188) = 0x029A4;
		REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e0) &= ~(0x3 << 2);
		REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e0) = REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3e0) | (3 << 2); /*tell phy boot from frequency point 1 */
		break;
	default:
		break;
	}
	return;
}

static void top_DDR_amble_config(unsigned DPHY0_BASE)
{
	unsigned data = 0;

	/*
	en_rd_odt=1: odt on in read mode
	write preamble =1
	write postamble=1
	read preamble=0
	read postamble=0
	*/
	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0x4);
	data &= 0xFFFF0FFF; /*clear write and read pre/post amble to 0 */
	data |= (1 << 11) | (1 << 14) | (1 << 15); /*enable DQ/DQS read ODT, set wr-preamble = 1  wr_post =1 */
	// data &= ~(0x10);
	/*Init subPHY-A */
	REG32(DPHY0_BASE + COMMON_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;

	data = REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x4);
	data &= 0xFFFF0FFF; /*clear write and read pre/post amble to 0 */
	data |= (1 << 11) | (1 << 14) | (1 << 15); /*enable DQ/DQS read ODT, set wr-preamble = 1  wr_post =1 */
	// data &= ~(0x10);
	/*Init subPHY-B */
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;

	return;
}

void top_DDR_ca_drivestr_adjust(unsigned DPHY0_BASE, unsigned combination)
{
	unsigned data = 0;
	uint8_t ca_reg2 = 0xd8;

	switch (combination) {
	case 0: // 60ohm
		ca_reg2 = 0x90;
		break;
	case 1: // 48ohm
		ca_reg2 = 0xb4;
		break;
	case 2: // 40ohm
		ca_reg2 = 0xd8;
		break;
	case 3: // 80ohm
		ca_reg2 = 0x6c;
		break;
	default:
		break;
	}

	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0x8);
	data &= 0xFFFF00FF;
	data |= (ca_reg2 << 8);

	REG32(DPHY0_BASE + COMMON_OFFSET + 0x8) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x8) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x8) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x8) = data;

	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x8) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x8) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x8) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x8) = data;

	return;
}

static void top_DDR_wr_ds_odt_vref(unsigned DPHY0_BASE, unsigned combination)
{
	unsigned data = 0, data1 = 0;
	uint8_t d_reg2 = 0;
	uint8_t dq_odt_cfg = 0;
	uint8_t vref_dq = 0x0;
	uint8_t vref_ca = 0x0;
	uint32_t DDRC_BASE = DPHY0_BASE - 0x40000;

	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0xc);
	data1 = REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0xc);

	switch (combination) {
	case 0: // 40+odt off
		d_reg2 = 0xd8;
		dq_odt_cfg = 0;
		vref_dq = 0x0; // ODT:OFF
		break;
	case 1: // 40+odt 120
		d_reg2 = 0xd8;
		dq_odt_cfg = 0x2;
		vref_dq = 0x5;
		break;
	case 2: // 40+60 odt
		d_reg2 = 0xd8;
		dq_odt_cfg = 0x1;
		vref_dq = 0xa;
		break;
	case 3: // 40+240 odt
		d_reg2 = 0xd8;
		dq_odt_cfg = 0x3;
		vref_dq = 0x2;
		break;
	case 4: // 60+ODT off
		d_reg2 = 0x90;
		dq_odt_cfg = 0x0;
		vref_dq = 0x0;
		break;
	case 5: // 60+ODT 120
		d_reg2 = 0x90;
		dq_odt_cfg = 0x2;
		vref_dq = 0x8;
		break;
	case 6: // 60+ODT 240
		d_reg2 = 0x90;
		dq_odt_cfg = 0x3;
		vref_dq = 0x3;
		break;
	case 7: // 48+ODT off
		d_reg2 = 0xB4;
		dq_odt_cfg = 0x3;
		vref_dq = 0x0;
		break;
	default:
		break;
	}

	data &= 0xFFFF00FF;
	data |= (d_reg2 << 8);
	REG32(DPHY0_BASE + COMMON_OFFSET + 0xc) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc) = data;

	data1 &= 0xFFFF00FF;
	data1 |= (d_reg2 << 8);
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0xc) = data1;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc) = data1;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc) = data1;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc) = data1;

	// vref_dq_ca
	REG32(0xd4282800 + 0x3a4) &= ~(0xff << 8);
	REG32(0xd4282800 + 0x3a4) |= (vref_dq << 12); // vref_dq:0.7vddq  0xa:0.84V
	REG32(0xd4282800 + 0x3a4) |= (vref_ca << 8); // vref_ca 0x0:0.6V //ca have no odt

	// fp3
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = 0xf0800400; // DRAM_Config_2,
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0110) |= (dq_odt_cfg << 20);
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0114) |= (dq_odt_cfg << 20);

	// fp2
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = 0xa0800400; // DRAM_Config_2,
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0110) |= (dq_odt_cfg << 20);
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0114) |= (dq_odt_cfg << 20);

	// fp1
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = 0x50800400; // DRAM_Config_2,
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0110) |= (dq_odt_cfg << 20);
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0114) |= (dq_odt_cfg << 20);
	// fp0
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = 0x00800400; // DRAM_Config_2,
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0110) |= (dq_odt_cfg << 20);
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0114) |= (dq_odt_cfg << 20);

	return;
}

static void top_DDR_rx_ds_odt_vref(unsigned DPHY0_BASE, unsigned combination)
{
	unsigned data = 0;
	uint8_t d_reg3 = 0;
	uint8_t rx_ref_d1 = 0x0, rx_ref_d2 = 0x0; // high 4 bit for DQS, low for bit for DQ
	uint8_t dram_ds = 0x0;
	unsigned ddrc_base = DPHY0_BASE - 0x40000;

	switch (combination) {
	case 0: // 40+odt off
		dram_ds = 0x2;
		d_reg3 = 0x80; // odt off
		rx_ref_d1 = 0x0;
		rx_ref_d2 = 0x0;
		break;
	case 1: // 40+120 odt
		dram_ds = 0x2;
		d_reg3 = 0x92;
		rx_ref_d1 = 0x55; // 0.75v
		rx_ref_d2 = 0x55;
		break;
	case 2: // 40+60 odt
		dram_ds = 0x2;
		d_reg3 = 0xa4;
		rx_ref_d1 = 0xaa; // 0.84
		rx_ref_d2 = 0xaa;
		break;
	case 3: // 40+240ohm odt
		dram_ds = 0x2;
		d_reg3 = 0x89;
		rx_ref_d1 = 0x11; // 0.68
		rx_ref_d2 = 0x11;
		break;
	case 4: // 60+odt off
		dram_ds = 0x4;
		d_reg3 = 0x80;
		rx_ref_d1 = 0x0; // 0.6
		rx_ref_d2 = 0x0;
		break;
	case 5: // 60+120
		dram_ds = 0x4;
		d_reg3 = 0x92;
		rx_ref_d1 = 0x88; // 0.8
		rx_ref_d2 = 0x88; // 0.8
		break;
	case 6: // 60+240
		dram_ds = 0x4;
		d_reg3 = 0x89;
		rx_ref_d1 = 0x44; // 0.72
		rx_ref_d2 = 0x44; // 0.72
		break;
	case 7: // 48+240
		dram_ds = 0x3;
		d_reg3 = 0x89;
		rx_ref_d1 = 0x22; // 0.7
		rx_ref_d2 = 0x22; // 0.7
		break;
	case 8: // 40+DQS ODT ON 80ohm, DQ ODT off
		dram_ds = 0x2;
		d_reg3 = 0x98;
		rx_ref_d1 = 0x80; // 0.8 dor dqs, 0.6 for dq
		rx_ref_d2 = 0x80; // 0.8 for dqs, 0.6 for dq
		break;
	case 9: // 48+odt off
		dram_ds = 0x3;
		d_reg3 = 0x80; // odt off
		rx_ref_d1 = 0x0;
		rx_ref_d2 = 0x0;
		break;
	default:
		break;
	}
	/*soc odt */
	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0xc);
	data &= 0xFF00FFFF;
	data |= (d_reg3 << 16);
	REG32(DPHY0_BASE + COMMON_OFFSET + 0xc) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc) = data;

	data = REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0xc);
	data &= 0xFF00FFFF;
	data |= (d_reg3 << 16);
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0xc) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc) = data;
	/*soc vref */
	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0x4);
	data &= 0x0000FFFF;
	data |= (rx_ref_d1 << 16) | (rx_ref_d2 << 24);
	REG32(DPHY0_BASE + COMMON_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;

	data = REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x4);
	data &= 0x0000FFFF;
	data |= (rx_ref_d1 << 16) | (rx_ref_d2 << 24);
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;

	/*device drive strength */
	REG32(ddrc_base + MC_CH0_BASE + 0x0104) = 0xf0800400;
	data = REG32(ddrc_base + MC_CH0_BASE + 0x0110);
	data &= 0xFFF0FFFF;
	data |= (dram_ds << 16); // device drive strength config
	REG32(ddrc_base + MC_CH0_BASE + 0x0110) = data;
	REG32(ddrc_base + MC_CH0_BASE + 0x0114) = data;

	REG32(ddrc_base + MC_CH0_BASE + 0x0104) = 0xa0800400;
	data = REG32(ddrc_base + MC_CH0_BASE + 0x0110);
	data &= 0xFFF0FFFF;
	data |= (dram_ds << 16); // device drive strength config
	REG32(ddrc_base + MC_CH0_BASE + 0x0110) = data;
	REG32(ddrc_base + MC_CH0_BASE + 0x0114) = data;

	REG32(ddrc_base + MC_CH0_BASE + 0x0104) = 0x50800400;
	data = REG32(ddrc_base + MC_CH0_BASE + 0x0110);
	data &= 0xFFF0FFFF;
	data |= (dram_ds << 16); // device drive strength config
	REG32(ddrc_base + MC_CH0_BASE + 0x0110) = data;
	REG32(ddrc_base + MC_CH0_BASE + 0x0114) = data;

	REG32(ddrc_base + MC_CH0_BASE + 0x0104) = 0x00800400;
	data = REG32(ddrc_base + MC_CH0_BASE + 0x0110);
	data &= 0xFFF0FFFF;
	data |= (dram_ds << 16); // device drive strength config
	REG32(ddrc_base + MC_CH0_BASE + 0x0110) = data;
	REG32(ddrc_base + MC_CH0_BASE + 0x0114) = data;

	return;
}

void DDR_phy_init_dove(unsigned DDRC_BASE, unsigned wds_odt, unsigned rds_odt,
	unsigned cads_odt)
{
	unsigned DPHY0_BASE = DDRC_BASE + 0x040000;
	unsigned device_type = 0;
	unsigned i = 0;
	unsigned data = 0;

	/*3.Set aon_reg2 to 0xf (PMUAP.ddr_ckphy_ctrl3.bit[15:8]) */
	REG32(0xd4282800 + 0x3A4) &= 0xFFFF00FF;

	REG32(0xd4282800 + 0x3A4) |= (0xF << 8);

	/*4 read device type */
	for (i = 0; i < 4; i++) {

		device_type = REG32(0xd4282800 + 0x3B8);
		LogMsg(0, "Address[0x%08x]=0x%08x \n", (0xd4282800 + 0x3B8),
			REG32(0xd4282800 + 0x3B8));
		device_type = ((device_type & 0x03000000) >> 24);
		LogMsg(0, "%d times read device type = 0x%x \n", i,
			device_type);
	}

	/*5, set device type */
	REG32(0xd4282800 + 0x398) |= (0x1 << 10);

	REG32(DPHY0_BASE + COMMON_OFFSET) = 0x0;
	REG32(DPHY0_BASE + COMMON_OFFSET + subPHY_B_OFFSET) = 0x0;

	REG32(DPHY0_BASE + COMMON_OFFSET) = 0x1;
	REG32(DPHY0_BASE + COMMON_OFFSET + subPHY_B_OFFSET) = 0x1;

#if defined(NEW_FEATURE) // wren_e4_on=1
	REG32(DPHY0_BASE + 0x0064) = 0x0000534A;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET + 0x0064) = 0x00000434A;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 2 + 0x0064) = 0x00000434A;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 3 + 0x0064) = 0x00000434A;

	REG32(DPHY0_BASE + 0x0068) = 0x0000534A;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET + 0x1068) = 0x00000434A;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 2 + 0x1068) = 0x00000434A;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 3 + 0x1068) = 0x00000434A;

#else
	REG32(DPHY0_BASE + 0x0064) = 0x0000134A;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET + 0x0064) = 0x0000034A;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 2 + 0x0064) = 0x0000034A;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 3 + 0x0064) = 0x0000034A;

	REG32(DPHY0_BASE + 0x0068) = 0x0000134A;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET + 0x1068) = 0x0000034A;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 2 + 0x1068) = 0x0000034A;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 3 + 0x1068) = 0x0000034A;
#endif

	top_DDR_amble_config(DPHY0_BASE);

	LogMsg(0, "ddr wds_odt %d rds_odt %d cads_odt %d\n", wds_odt,
		rds_odt, cads_odt);
	top_DDR_wr_ds_odt_vref(DPHY0_BASE, wds_odt); // transfer

	top_DDR_rx_ds_odt_vref(DPHY0_BASE, rds_odt); // receiver,rxodt=80ohm

	top_DDR_ca_drivestr_adjust(DPHY0_BASE, cads_odt);

	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0x10);
	// data&=0xF7FFFFFF;
	// data |= (0x44<<8);
	data |= 0x10000000; // enable duty calibration

	REG32(DPHY0_BASE + COMMON_OFFSET + 0x10) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x10) = data;

	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x10) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x10) = data;

	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x10) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x10) = data;

	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x10) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x10) = data;

	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n",
		(DPHY0_BASE + COMMON_OFFSET + 0x10),
		REG32(DPHY0_BASE + COMMON_OFFSET + 0x10));

#if defined(NEW_FEATURE)
	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0x14);
	data &= 0xFF9FFFEF;
	data |= (0x3 << 21);
	REG32(DPHY0_BASE + COMMON_OFFSET + 0x14) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x14) = data;

	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x14) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x14) = data;

	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x14) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x14) = data;

	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x14) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x14) = data;
#endif

	/*
			//turn off CKPHY A CK_T/T
			data=REG32(DPHY0_BASE+COMMON_OFFSET+0x8);
			data&=0xFBFBFFFF;//ca_reg4 bit2=0, PHYA  CA2=0
			REG32(DPHY0_BASE+COMMON_OFFSET+0x8)=data;
			REG32(DPHY0_BASE+COMMON_OFFSET+FREQ_POINT_OFFSET+0x8)=data;
			REG32(DPHY0_BASE+COMMON_OFFSET+FREQ_POINT_OFFSET*2+0x8)=data;
			REG32(DPHY0_BASE+COMMON_OFFSET+FREQ_POINT_OFFSET*3+0x8)=data;

			//turn off CKPHY B CS CKE
			data=REG32(DPHY0_BASE+COMMON_OFFSET+subPHY_B_OFFSET+0x8);
			data&=0xFC3EFFFF;  //PHY B, CA0 off
			REG32(DPHY0_BASE+COMMON_OFFSET+subPHY_B_OFFSET+0x8)=data;
			REG32(DPHY0_BASE+COMMON_OFFSET+subPHY_B_OFFSET+FREQ_POINT_OFFSET+0x8)=data;
			REG32(DPHY0_BASE+COMMON_OFFSET+subPHY_B_OFFSET+FREQ_POINT_OFFSET*2+0x8)=data;
			REG32(DPHY0_BASE+COMMON_OFFSET+subPHY_B_OFFSET+FREQ_POINT_OFFSET*3+0x8)=data;
	*/

	/*set reg for r-cali */
	REG32(DPHY0_BASE + COMMON_OFFSET + 0x30) = 0x1077;
	// LogMsg(0,"ADDR[0x%08x]=0x%08x !!!! \n",(DPHY0_BASE+COMMON_OFFSET+0x18),REG32(DPHY0_BASE+COMMON_OFFSET+0x18));

	/*set phy pipe line */
	REG32(DPHY0_BASE + OTHER_CONTROL_OFFSET + 0x24) = 0x0;

	REG32(DPHY0_BASE + OTHER_CONTROL_OFFSET + 0x1c) |= (0x1 << 2);

	// LogMsg(0,"ADDR[0x%08x]=0x%08x !!!! \n",(DPHY0_BASE+OTHER_CONTROL_OFFSET+0x24),REG32(DPHY0_BASE+OTHER_CONTROL_OFFSET+0x24));

	/*Init other control
	enable phy hwdfc_en */
	REG32(DPHY0_BASE + OTHER_CONTROL_OFFSET) |= 0x1;
	// LogMsg(0,"ADDR[0x%08x]=0x%08x !!!! \n",(DPHY0_BASE+OTHER_CONTROL_OFFSET),REG32(DPHY0_BASE+OTHER_CONTROL_OFFSET));
	/*Make rx dqs =0xF */
	for (i = 0; i < 4; i++) {
		REG32(DPHY0_BASE + FREQ_POINT_OFFSET * i + 0x60) = 0x88;
		REG32(DPHY0_BASE + FREQ_POINT_OFFSET * i + 0x160) = 0x88;
		REG32(DPHY0_BASE + FREQ_POINT_OFFSET * i + subPHY_B_OFFSET + 0x60) = 0x88;
		REG32(DPHY0_BASE + FREQ_POINT_OFFSET * i + subPHY_B_OFFSET + 0x160) = 0x88;

		REG32(DPHY0_BASE + FREQ_POINT_OFFSET * i + 0x60 + 0x1000) = 0x88;
		REG32(DPHY0_BASE + FREQ_POINT_OFFSET * i + 0x160 + 0x1000) = 0x88;
		REG32(DPHY0_BASE + FREQ_POINT_OFFSET * i + subPHY_B_OFFSET + 0x60 + 0x1000) = 0x88;
		REG32(DPHY0_BASE + FREQ_POINT_OFFSET * i + subPHY_B_OFFSET + 0x160 + 0x1000) = 0x88;
	}

	REG32(DPHY0_BASE + 0x90) = 0x17528304; // dq1_pad[0]
	REG32(DPHY0_BASE + 0x1090) = 0x31084675; // dq2_pad[0]
	REG32(DPHY0_BASE + 0x2090) = 0x20026;
	REG32(DPHY0_BASE + 0x2094) = 0x130024;

	REG32(DPHY0_BASE + (1 << 9) + 0x90) = 0x50268437; // dq1_pad[1]
	REG32(DPHY0_BASE + (1 << 9) + 0x1090) = 0x45723108; // dq2_pad[1]
	REG32(DPHY0_BASE + (1 << 9) + 0x2090) = 0x130061;
	REG32(DPHY0_BASE + (1 << 9) + 0x2094) = 0x876950;
}

void LPDDR3_MC_Phy_Device_Init(unsigned DDRC_BASE, unsigned int fp,
	unsigned cs_num, unsigned wds_odt,
	unsigned rds_odt, unsigned cads_odt)
{
	uint32_t read_data;
	unsigned cs_select = 0;
	unsigned cnt = 0;
	unsigned status_chk = 0x0;

	if (cs_num == 0x1) {
		cs_select = 0x1;
		status_chk = 0x1;

	} else {
		cs_select = 0x3;
		status_chk = 0x11;
	}

	/*DDRC port AXI QQS setting */
	REG32(0xd4282c00 + 0x118) = 0x33221133;

	LogMsg(0, "Config DDR MC .....\n");
	DDR_MC_init(DDRC_BASE, fp, cs_num);

	LogMsg(0, "Config DDR phy .....\n");
	DDR_phy_init_dove(DDRC_BASE, wds_odt, rds_odt, cads_odt);

	// init ddrc and ddrphy
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3d0) = 0x10000001 | (cs_select << 24);

	LogMsg(0, "CHK @[0x%08X]'bit31 to be 1\n",
		(DDRC_BASE + MC_CH0_PHY_BASE + 0x3fc));
	read_data = REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3fc);
	while (((read_data & 0x80000000) != 0x80000000) && (cnt < 10000)) {
		read_data = REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3fc);
		cnt++;
	}
	if (cnt < 10000) {
		LogMsg(0, "PHY INIT done \n");
	} else {
		LogMsg(0, "PHY INIT timeout \n");
	}

	cnt = 0; // clear counter

	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3d0) = 0x10000100 | (cs_select << 24); // MCK6 DFI phy user cmd to clear the state

	LogMsg(0, "Init DRAM .....\n");
	// start init
	REG32(DDRC_BASE + 0x20) = (0x10000001 | (cs_select << 24)); // USER_COMMAND_0   power sequence for Dram

	/*wait init down */
	LogMsg(0, "CHK @[0x%08X]'bit0 to be 1\n", (DDRC_BASE + 0x8));

	read_data = REG32(DDRC_BASE + 0x8);
	while (((read_data & status_chk) != status_chk) && (cnt < 100000)) {
		read_data = REG32(DDRC_BASE + 0x8);
		cnt++;
	}

	if (cnt < 100000) {
		LogMsg(0, "DRAM INIT done \n");
	} else {
		LogMsg(0, "DRAM INIT timeout \n");
	}

	LogMsg(0, "ZQ calibration .....\n");
	// CS0 ZQ calibration
	REG32(DDRC_BASE + 0x20) = 0x11001000; /*ZQ calibration long */ // make Dram to calibration;
	REG32(DDRC_BASE + 0x20) = 0x11001000; // ZQ calibration Latch command
	if (cs_num == 2) {
		// CS1 ZQ calibration
		REG32(DDRC_BASE + 0x20) = 0x12002000; // ZQ calibration Start command
		REG32(DDRC_BASE + 0x20) = 0x12001000; // ZQ calibration Latch command
	}

	LogMsg(0, "Init DRAM MR Reg.....\n");
	/*Init MR register */
	REG32(DDRC_BASE + 0x24) = 0x10020001 | (cs_select << 24); // Init MR1, burst length,  burst type, nWR
	REG32(DDRC_BASE + 0x24) = 0x10020002 | (cs_select << 24); // Init MR2, WL, RL
	REG32(DDRC_BASE + 0x24) = 0x10020003 | (cs_select << 24); // Init MR3, dram output drive strength
	REG32(DDRC_BASE + 0x24) = 0x1002000B | (cs_select << 24); // Init MR11

	LogMsg(0, "Init DRAM MR Reg done.....\n");
	// falcon_phy_reg_dump();
}

uint32_t getBitAtPosition(uint32_t number, uint32_t position)
{
	return (number >> position) & 1;
}

#define MODE_REG_TEST
#if defined(MODE_REG_TEST)
static uint8_t Mode_register_read(unsigned ddrc_base, unsigned MR, unsigned CH,
	unsigned CS)
{
	uint32_t read_data;
	uint8_t UI1 = 0;
	// uint8_t UI2=0;
	// uint8_t UI3=0;
	// uint8_t UI4=0;

	REG32(ddrc_base + 0x24) = (0x10010000 + ((CS + 1) << 24) + (CH << 18) + MR);
	/*wait MR read data ready */
	read_data = REG32(ddrc_base + 0x370);
	while (!(read_data & 0x80000000)) {
		read_data = REG32(ddrc_base + 0x370);
	}

	/*UI Read */
	UI1 = REG32(ddrc_base + 0x370) & 0xFF;
	// UI2=REG32(ddrc_base+0x230)&0xFF;
	// UI3=REG32(ddrc_base+0x234)&0xFF;
	// UI4=REG32(ddrc_base+0x238)&0xFF;

	// LogMsg(0, "CH%d CS%d MR%d Read UI2=0x%x\n",CH,CS,MR,UI2);
	// LogMsg(0, "CH%d CS%d MR%d Read UI3=0x%x\n",CH,CS,MR,UI3);
	// LogMsg(0, "CH%d CS%d MR%d Read UI4=0x%x\n",CH,CS,MR,UI4);
	LogMsg(0, "CH%d CS%d MR%d Read UI1=0x%x\n", CH, CS, MR, UI1);

	return UI1;
}
#endif

static void mck6_sw_fc_top(unsigned freqNo)
{
	unsigned read_data;

	switch (freqNo) {
	case 0: // 1066Mbps
		LogMsg(0, "sw frequency change to 1066Mbps !!!! \n");
		// CKPHY_FC_CTRL
		REG32(0xd4282800 + 0x3b4) = 0x00003B08; // sel pll2, div 3
		// PMU_MC_HW_SLP_TYPE
		REG32(0xd4282800 + 0x0b0) = 0x40600400; // disable table
		// PMU_CC_AP
		REG32(0xd4282800 + 0x004) = 0x4000000; // dclk fc trigger
		LogMsg(0, "wait sw frequency change to 1066Mbps!!!! \n");
		// pooling trigger bit self clean
		read_data = REG32(0xd4282800 + 0x004);
		while ((read_data & 0x4000000) != 0x0) {
			read_data = REG32(0xd4282800 + 0x004);
		}
		LogMsg(0, "sw frequency change to 1066Mbps done!!!! \n");
		break;

	default:
		LogMsg(0, "not support frequency change !!!! \n");
		break;
	}
}

void set_PLL(void)
{
	//pll1 2666mbps
	unsigned pll1_reg0 = 0x55;
	unsigned pll1_reg1 = 0x55;
	unsigned pll1_reg2 = 0x3c;
	unsigned pll1_reg3 = 0x20;
	unsigned pll1_reg4 = 0x38;
	unsigned pll1_reg5 = 0x65;
	unsigned pll1_reg6 = 0xdd;
	unsigned pll1_reg7 = 0x50;
	REG32(0xd4282800 + 0x39C) = (pll1_reg3 << 24) | (pll1_reg2 << 16) | (pll1_reg1 << 8) | (pll1_reg0);
	REG32(0xd4282800 + 0x3A0) = (pll1_reg7 << 24) | (pll1_reg6 << 16) | (pll1_reg5 << 8) | (pll1_reg4);

	// pll2 3200MHZ
	unsigned pll2_reg0 = 0x55;
	unsigned pll2_reg1 = 0x55;
	unsigned pll2_reg2 = 0x3d;
	unsigned pll2_reg3 = 0x20;
	unsigned pll2_reg4 = 0x43;
	unsigned pll2_reg5 = 0x67;
	unsigned pll2_reg6 = 0xdd;
	unsigned pll2_reg7 = 0x50;
	REG32(0xd4282800 + 0x3A8) = (pll2_reg3 << 24) | (pll2_reg2 << 16) | (pll2_reg1 << 8) | (pll2_reg0);
	REG32(0xd4282800 + 0x3AC) = (pll2_reg7 << 24) | (pll2_reg6 << 16) | (pll2_reg5 << 8) | (pll2_reg4);
}

static void enable_PLL(void)
{
	unsigned read_data = 0;
	REG32(0xd4282800 + 0x3b4) &= 0xFFFFFCFF;
	REG32(0xd4282800 + 0x3b4) |= (0x1 << 11) | (0x1 << 8) | (0x1 << 9);
	LogMsg(0, "CKPHY_FC_CTRL[0x%08x]=0x%08x \n", (0xd4282800 + 0x3b4),
		REG32(0xd4282800 + 0x3b4));
	read_data = REG32(0xd4282800 + 0x3b4);
	while ((read_data & 0x30000) != 0x30000) {
		read_data = REG32(0xd4282800 + 0x3b4);
	}
	LogMsg(0, "CKPHY_FC_CTRL[0x%08x]=0x%08x \n", (0xd4282800 + 0x3b4),
		REG32(0xd4282800 + 0x3b4));
	LogMsg(0, "DDR PLL ready \n");
}

static void top_Common_config(unsigned fp)
{
	/*1.Set PLL1.PLL_REG1 to 0x3B(PMUAP.ddr_ckphy_ctrl1.bit[15:8]) */

	set_PLL();

	enable_PLL();

	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (0xd4282800 + 0x39c),
		REG32(0xd4282800 + 0x39c));
	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (0xd4282800 + 0x3a0),
		REG32(0xd4282800 + 0x3a0));
	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (0xd4282800 + 0x3a8),
		REG32(0xd4282800 + 0x3a8));
	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (0xd4282800 + 0x3ac),
		REG32(0xd4282800 + 0x3ac));

	/*Config boot frequency to 1066Mbps */
	mck6_sw_fc_top(fp);

	REG32(0xd42828e8) &= 0xFFFFFFFC; // release MCK6 hclk reset and enable MCK6 hclk
	REG32(0xd42828e8) |= 0x3;

	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (0xd4282800 + 0xe8),
		REG32(0xd4282800 + 0xe8));
	LogMsg(0, "MCK6 reset and release .....\n");

	return;
}

void lpddr3_dfc(unsigned freqNo)
{
	unsigned int read_data, index, pll_control;

	// PMU_AP_IMR
	REG32(0xd4282800 + 0x098) |= 0x10; // enable dclk fc done interrupt
	// DFC_TABLE for top mc0
	REG32(0xc0000000 + 0x148) = 0xc0a30000;

	LogMsg(0, "lpddr3_dfc.....\n");

	switch (freqNo) {
	case 0:
		LogMsg(0, "change to 1066 \n");
		pll_control = 0x00003308;
		index = 0x0;
		break;
	case 1:
		LogMsg(0, "change to 1333 \n");
		pll_control = 0x00003350;
		index = 0x1;
		break;
	case 2:
	default:
		LogMsg(0, "change to 1600 \n");
		pll_control = 0x00003304;
		index = 0x2;
		break;
	}

	REG32(0xd4282800 + 0x3b4) = pll_control;
	REG32(0xd4282800 + 0x0b0) = (1 << TOP_DDRPHY0_EN_OFFSET) | (1 << TOP_DCLK_BYPASS_RST_OFFSET)
		| (1 << TOP_FREQ_PLL_CHG_MODE_OFFSET) | (index << TOP_MC_REQ_TABLE_NUM_OFFSET);

	LogMsg(0, "Request frequency change \n");
	// PMU_CC_AP
	// REG320xd4282800 + 0x004= (1<<TOP_AP_ALLOW_SPD_CHG)|(TOP_DDR_FREQ_CHG_REQ; //dclk fc trigger
	REG32(0xd4282800 + 0x004) = (0x1 << TOP_DDR_FREQ_CHG_REQ); // dclk fc trigger
	LogMsg(0, "Wait frequency change done \n");
	// pooling PMU_AP_ISR
	read_data = REG32(0xd4282800 + 0x004);
	while ((read_data & 0x400000) != 0x0) {
		read_data = REG32(0xd4282800 + 0x004);
	}
	LogMsg(0, "frequency change done!!!! \n");
	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (0xd4282800 + 0x3b4), REG32(0xd4282800 + 0x3b4)); //(DPHY0_BASE+0x10000

	return;
}

/*
for lpddr3, must check the MR8 bit6~7, the width is X16 or X32.

if the width is x16, the density per CS should double
*/
static void Adjust_density(unsigned Ddrc_Base, uint8_t vendor,
	uint8_t density_cs0, uint8_t density_cs1,
	uint8_t cs_num)
{
	uint8_t density_map = 0;
	uint8_t io_width = 0;
	uint8_t dev_type = 0x3;
	uint32_t cs0_addr, cs1_addr_start = 0;

#if defined(PIN_SWAP)
	dev_type = 0x1;
#endif

	density_map = ((density_cs0 & 0x3C) >> 2);
	io_width = ((density_cs0 & 0xC0) >> 6);

	LogMsg(0,
		"cs_num=%d  density=0x%x 0x%x vendor=0x%x density_map=0x%x, io_width=0x%x\n",
		cs_num, density_cs0, density_cs1, vendor, density_map, io_width);

	if (vendor == DDR_MID_SAMSUNG) {
		LogMsg(0, "vendor is Samsung \n");
	} else if (vendor == DDR_MID_CMXT) {
		LogMsg(0, "vendor is CMXT \n");
	} else if (vendor == DDR_MID_MICRON) {
		LogMsg(0, "vendor is Micron \n");
	} else if (vendor == DDR_MID_SK_HYNIX) {
		LogMsg(0, "vendor is Hynix \n");
	} else if (vendor == DDR_MID_NANYA) {
		LogMsg(0, "vendor is Nanya \n");
	} else if (vendor == DDR_MID_WINBOND) {
		LogMsg(0, "vendor is Winbond \n");
	} else {
		LogMsg(0, "vendor is Wrong \n");
	}

	switch (density_map) {
	case 0x6:
		if (io_width == 0x0) // die is x32 512MB per cs
		{
			REG32(Ddrc_Base + MC_CH0_BASE) = 0x000D0001;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x4) = 0x0;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x20) = 0x05000432;
		} else // die is x16 1GB per cs
		{
			REG32(Ddrc_Base + MC_CH0_BASE) = 0x000E0001;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x4) = 0x0;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x20) = 0x06000442;
		}
		break;
	case 0xE:
		if (io_width == 0x0) // die is  x32, 0.75GB per cs
		{
			REG32(Ddrc_Base + MC_CH0_BASE) = 0x00010001;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x4) = 0x0;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x20) = 0x05000532;
		} else // die is  x16, 1.5GB per cs
		{
			REG32(Ddrc_Base + MC_CH0_BASE) = 0x00020001;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x4) = 0x0;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x20) = 0x06000542;
		}
		break;
	case 0x7:
		if (io_width == 0x0) // die is  x32, 1GB per CS
		{
			REG32(Ddrc_Base + MC_CH0_BASE) = 0x000E0001;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x4) = 0x0;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x20) = 0x05000532;
		} else // die is x16, 2GB per CS
		{
			REG32(Ddrc_Base + MC_CH0_BASE) = 0x000F0001;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x4) = 0x0;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x20) = 0x06000542;
		}
		break;
	case 0xd: // No this type device, not check yet!!!!
		if (io_width == 0x0) // die is  x32, 1.5GB per CS
		{
			REG32(Ddrc_Base + MC_CH0_BASE) = 0x00020001;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x4) = 0x0;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x20) = 0x06000542;
		} else // die is x16, 2GB per CS
		{
			REG32(Ddrc_Base + MC_CH0_BASE) = 0x00030001;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x4) = 0x0;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x20) = 0x07000552;
		}
		break;
	case 0x8: // No this type device, not check yet!!!
		if (io_width == 0x0) // die is  x32, 2GB per CS
		{
			REG32(Ddrc_Base + MC_CH0_BASE) = 0x000f0001;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x4) = 0x0;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x20) = 0x06000542;
		} else // die is x16, 4GB per CS
		{
			REG32(Ddrc_Base + MC_CH0_BASE) = 0x00100001;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x4) = 0x0;
			REG32(Ddrc_Base + MC_CH0_BASE + 0x20) = 0x07000552;
		}
		break;
	case 0x9:
		break;
	default:
		LogMsg(0, "Density is Wrong \n");
		break;
	}
	if (cs_num == 2) {
		density_map = ((density_cs1 & 0x3C) >> 2);
		io_width = ((density_cs1 & 0xC0) >> 6);
		cs0_addr = (REG32(Ddrc_Base + MC_CH0_BASE) & 0x001f0000) >> 16;
		switch (cs0_addr) {
		case 0x1:
			cs1_addr_start = 0x3;
			break;
		case 0x2:
			cs1_addr_start = 0x6;
			break;
		case 0x3:
			cs1_addr_start = 0xc;
			break;
		case 0xd:
			cs1_addr_start = 0x2;
			break;
		case 0xe:
			cs1_addr_start = 0x4;
			break;
		case 0xf:
			cs1_addr_start = 0x8;
			break;
		default:
			cs1_addr_start = 0x0;
			LogMsg(1, "unsupport cs1 density maybe over range ?\n");
			break;
		}
		switch (density_map) {
		case 0x6:
			if (io_width == 0x0) // x16 smallest unit,0.5GB byte per CS
			{
				REG32(Ddrc_Base + MC_CH0_BASE + 0x8) = (cs1_addr_start << 28) | 0x000D0001;
				REG32(Ddrc_Base + MC_CH0_BASE + 0xc) = 0x0;
				REG32(Ddrc_Base + MC_CH0_BASE + 0x24) = 0x05000432;
			} else if (io_width == 0x1) {
				REG32(Ddrc_Base + MC_CH0_BASE + 0x8) = (cs1_addr_start << 28) | 0x000E0001;
				REG32(Ddrc_Base + MC_CH0_BASE + 0xc) = 0x0;
				REG32(Ddrc_Base + MC_CH0_BASE + 0x24) = 0x06000442;
			}
			break;
		case 0xE:
			if (io_width == 0x0) // x16 smallest unit,0.75GB byte per CS
			{
				REG32(Ddrc_Base + MC_CH0_BASE + 0x8) = (cs1_addr_start << 28) | 0x00010001;
				REG32(Ddrc_Base + MC_CH0_BASE + 0xc) = 0x0;
				REG32(Ddrc_Base + MC_CH0_BASE + 0x24) = 0x05000532;
			} else if (io_width == 0x1) {
				REG32(Ddrc_Base + MC_CH0_BASE + 0x8) = (cs1_addr_start << 28) | 0x00020001;
				REG32(Ddrc_Base + MC_CH0_BASE + 0xc) = 0x0;
				REG32(Ddrc_Base + MC_CH0_BASE + 0x24) = 0x06000542;
			}
			break;
		case 0x7:
			if (io_width == 0x0) // x16 smallest unit,1GB byte per CS
			{
				REG32(Ddrc_Base + MC_CH0_BASE + 0x8) = (cs1_addr_start << 28) | 0x000E0001;
				REG32(Ddrc_Base + MC_CH0_BASE + 0xc) = 0x0;
				REG32(Ddrc_Base + MC_CH0_BASE + 0x24) = 0x05030532;
			} else if (io_width == 0x1) {
				REG32(Ddrc_Base + MC_CH0_BASE + 0x8) = (cs1_addr_start << 28) | 0x000F0001;
				REG32(Ddrc_Base + MC_CH0_BASE + 0xc) = 0x0;
				REG32(Ddrc_Base + MC_CH0_BASE + 0x24) = 0x06000542;
			}
			break;
		case 0xd:
			if (io_width == 0x0) // x16 smallest unit,1.5GB byte per CS, for example CXMT 3GB LPDDR4x,MR8=0xC
			{
				REG32(Ddrc_Base + MC_CH0_BASE + 0x8) = (cs1_addr_start << 28) | 0x00020001;
				REG32(Ddrc_Base + MC_CH0_BASE + 0xc) = 0x0;
				REG32(Ddrc_Base + MC_CH0_BASE + 0x24) = 0x06000542;
			} else if (io_width == 0x1) {
				REG32(Ddrc_Base + MC_CH0_BASE + 0x8) = (cs1_addr_start << 28) | 0x00030001;
				REG32(Ddrc_Base + MC_CH0_BASE + 0xc) = 0x0;
				REG32(Ddrc_Base + MC_CH0_BASE + 0x24) = 0x07000552;
			}
			break;
		case 0x8:
			if (io_width == 0x0) // x16 smallest unit,2G byte per CS, for example K4UBE3D4AB-MGCL
			{
				REG32(Ddrc_Base + MC_CH0_BASE + 0x8) = (cs1_addr_start << 28) | 0x000F0001;
				REG32(Ddrc_Base + MC_CH0_BASE + 0xc) = 0x0;
				REG32(Ddrc_Base + MC_CH0_BASE + 0x24) = 0x06000542;
			} else // x8 smallest unit,4G byte per CS, for example K4UCE3Q4AA-MGCL,MR8=0x52
			{
				REG32(Ddrc_Base + MC_CH0_BASE + 0x8) = (cs1_addr_start << 28) | 0x00100001;
				REG32(Ddrc_Base + MC_CH0_BASE + 0xc) = 0x1;
				REG32(Ddrc_Base + MC_CH0_BASE + 0x24) = 0x07000552;
			}
			break;
		default:
			LogMsg(0, "Density is Wrong \n");
			break;
		}
	} else {
		REG32(Ddrc_Base + MC_CH0_BASE + 0x8) = 0x0;
		REG32(Ddrc_Base + MC_CH0_BASE + 0xC) = 0x0;
		REG32(Ddrc_Base + MC_CH0_BASE + 0x24) = 0x0;
	}

	REG32(Ddrc_Base + MC_CH0_BASE + 0x20) |= (dev_type << 16);
	REG32(Ddrc_Base + MC_CH0_BASE + 0x24) |= (dev_type << 16);

	return;
}

static void init_table_mc_tim(uint32_t ddrc_base, uint32_t* idx)
{
	uint32_t i;
	uint32_t read_data;
	// uint32_t MC_CH0_BASE=0x200;
	uint32_t mc_ch0_phy_base = 0x1000;
	volatile unsigned addrs[] = {
		MC_CH0_BASE + 0x0100, // DRAM Config 1 RL/WL
		MC_CH0_BASE + 0x010c, // DRAM Config 4
		MC_CH0_BASE + 0x0110, // DRAM Config 5 cs0
		MC_CH0_BASE + 0x0114, // DRAM Config 5 cs1
		MC_CH0_BASE + 0x018c, // ZQC timing 0
		MC_CH0_BASE + 0x0190, // ZQC timing 1
		MC_CH0_BASE + 0x0194, // Refresh timing
		MC_CH0_BASE + 0x01fc,
		MC_CH0_BASE + 0x0198, // SelfRefresh timing 0
		MC_CH0_BASE + 0x019c, // SelfRefresh timing 1
		MC_CH0_BASE + 0x01a0, // Power down timing 0
		MC_CH0_BASE + 0x01a4, // Power down timing 1
		MC_CH0_BASE + 0x01a8, // MRS timing
		MC_CH0_BASE + 0x01ac, // ACT timing
		MC_CH0_BASE + 0x01b0, // Pre-Charge timing
		MC_CH0_BASE + 0x01b4, // CAS/RAS timing 0
		MC_CH0_BASE + 0x01b8, // CAS/RAS timing 1
		MC_CH0_BASE + 0x01bc, // Off-spec timing 0     WDQS enable
		MC_CH0_BASE + 0x01c0, // Off-spec timing 1
		MC_CH0_BASE + 0x01c4, // DRAM_read timing
		MC_CH0_BASE + 0x0200, // WDQS timing
		MC_CH0_BASE + 0x01d8, // CH0_dram_training_timing
		MC_CH0_BASE + 0x014c, // odt_control_3
		mc_ch0_phy_base + 0x03e4, // MCK6 DFI phy ctrl register 1
		mc_ch0_phy_base + 0x03ec // CH0_DFI_PHY_Control_3 trdlvl_rr
	};
	uint32_t tim_size = sizeof(addrs) >> 2;

	for (i = 0; i < tim_size; i++) {
		read_data = REG32(ddrc_base + addrs[i]);
		REG32(ddrc_base + 0x0074) = read_data;
		REG32(ddrc_base + 0x0078) = addrs[i];
		REG32(ddrc_base + 0x0070) = (*idx)++;
	}
}

static void init_table_mc_a0(uint32_t ddrc_base)
{
	uint32_t idx = 0x200;
	uint32_t i = 0;
	// uint32_t MC_CH0_BASE=0x200;
	volatile unsigned mc_cfg2_addr = MC_CH0_BASE + 0x0104;
	uint32_t temp_data, mc_cfg2_org, mc_cfg2_fp, mc_ctl0_org;
	volatile unsigned addrs[] = {
		0x0048,
		0x0050,
		0x0054,
		0x0058,
		0x0060,
		0x0064,
		0x0068,
		0x0148,
		0x014c,
		0x0180,
		MC_CH0_BASE + 0x0000,
		MC_CH0_BASE + 0x0004,
		MC_CH0_BASE + 0x0008,
		MC_CH0_BASE + 0x000c,
		MC_CH0_BASE + 0x0020,
		MC_CH0_BASE + 0x0024,
		MC_CH0_BASE + 0x00c4,
		MC_CH0_BASE + 0x00c0,
		MC_CH0_BASE + 0x00c8,
		MC_CH0_BASE + 0x00cc,
		MC_CH0_BASE + 0x0108,
		MC_CH0_BASE + 0x0180,
		MC_CH0_BASE + 0x0184,
		MC_CH0_BASE + 0x0188,
		0x80,
		0xa00,
		0xac0,
		0xacc,
	};

	mc_ctl0_org = REG32(ddrc_base + 0x44);
	temp_data = mc_ctl0_org | (BIT(2) | BIT(12));
	REG32(ddrc_base + 0x74) = temp_data;
	REG32(ddrc_base + 0x78) = 0x00000044 | (0x1 << 16);
	REG32(ddrc_base + 0x70) = idx++;

	uint32_t cfg_size = sizeof(addrs) >> 2;
	for (i = 0; i < cfg_size; i++) {
		temp_data = REG32(ddrc_base + addrs[i]);
		REG32(ddrc_base + 0x74) = temp_data;
		REG32(ddrc_base + 0x78) = addrs[i] & 0xffff;
		REG32(ddrc_base + 0x70) = idx++;
	}

	mc_cfg2_org = REG32(ddrc_base + mc_cfg2_addr);
	temp_data = mc_cfg2_org;
	temp_data &= ~(0xf << 28);

	mc_cfg2_fp = temp_data;
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_fp; // fp0
	REG32(ddrc_base + 0x0074) = mc_cfg2_fp;
	REG32(ddrc_base + 0x0078) = mc_cfg2_addr;
	REG32(ddrc_base + 0x0070) = idx++;
	init_table_mc_tim(ddrc_base, &idx);
	// fp1
	mc_cfg2_fp = temp_data | (0x5 << 28);
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_fp;
	mc_cfg2_fp &= ~(0x3 << 28); // clr fsp_op
	REG32(ddrc_base + 0x0074) = mc_cfg2_fp;
	REG32(ddrc_base + 0x0078) = mc_cfg2_addr;
	REG32(ddrc_base + 0x0070) = idx++;
	init_table_mc_tim(ddrc_base, &idx);

	// FP2
	mc_cfg2_fp = temp_data | (0xa << 28);
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_fp;
	mc_cfg2_fp &= ~(0x3 << 28); // clr fsp_op

	REG32(ddrc_base + 0x0074) = mc_cfg2_fp;
	REG32(ddrc_base + 0x0078) = mc_cfg2_addr;
	REG32(ddrc_base + 0x0070) = idx++;
	init_table_mc_tim(ddrc_base, &idx);

	// FP3
	mc_cfg2_fp = temp_data | (0xf << 28);
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_fp;
	mc_cfg2_fp &= ~(0x3 << 28); // clr fsp_op
	REG32(ddrc_base + 0x0074) = mc_cfg2_fp;
	REG32(ddrc_base + 0x0078) = mc_cfg2_addr;
	REG32(ddrc_base + 0x0070) = idx++;
	init_table_mc_tim(ddrc_base, &idx);

	// LogMsg(0,"idx to 0x%x\n", idx);

#if 0
	//2. trigger phy table
	//LogMsg(1, "//MC_INIT_TABLE: 2. trigger phy table! insert table code");
	//LogMsg(1, "//MC_INIT_TABLE: NOTE insert table code!");
								/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x00020200;
								//dphy table addr
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x000013e0;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;

								/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x13000010;
								//dphy table trigger
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x000013d0;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;

								/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x00010000;
								//dphy table done
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x000033fc;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x00010000;
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x000033fc;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;
#endif

	// 3. get mc_fsp_op/wr dev_fsp_op/wr
	// LogMsg(1, "//MC_INIT_TABLE: 3. get mc_fsp_op/wr dev_fsp_op/wr!");
	/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x13000008;
	// invert_mc_fsp_wr, get from pmu csysfreq
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x00000020;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x13000004;
	// invert_mc_fsp_op, get from pmu csysfreq
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x00000020;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;

#if 0
								/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x13020000;
								//rld_ddr_fsp, dev_fsp_op/wr get from pmu
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x00000028;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;
#endif

	// 4. dfi handshake
	// LogMsg(1, "//MC_INIT_TABLE: 4. dfi handshake!");
	/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x13000001;
	// dfi_init_start handhske
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x000013d0;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x00008000;
	// get dfi_init_complete
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x000033fc;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x00008000;
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x000033fc;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x10000100;
	// clear dfi_init_start
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x000013d0;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x00008000;
	// get dfi_init_complete
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x000033fc;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x00008000;
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x000033fc;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;
#if 0
	//LogMsg(1, "//MC_INIT_TABLE: 5. MRW!");
								/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x1302000d;
								//MRW13 for DM
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x00000024;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;

								/*TABLE_C: */ REG32(ddrc_base + 0x0074) = 0x13020003;
								//MRW3 for DBI
	/*TABLE_C: */ REG32(ddrc_base + 0x0078) = 0x00000024;
	/*TABLE_C: */ REG32(ddrc_base + 0x0070) = idx++;
#endif

	// /*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x11010004; //MRR MR4, just for test refrate
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x00000024;
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	// /*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x11050004; //MRR MR4, just for test refrate
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x00000024;
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	// /*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x12010004; //MRR MR4, just for test refrate
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x00000024;
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	// /*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x12050004; //MRR MR4, just for test refrate
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x00000024;
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;
	//
	// LogMsg(1, "//MC_INIT_TABLE: NOTE insert table code end!");

	// LogMsg(0, "REG32(ddrc_base + 0x0074)= 0x%x\n", mc_ctl0_org;
	// LogMsg(0, "REG32(ddrc_base + 0x0078)= 0x%x\n", 0x44 | (0x1<<17);    //EOP
	// LogMsg(0, "REG329ddrc_base + 0x0070)= 0x%x\n", idx;
	REG32(ddrc_base + 0x0074) = mc_ctl0_org; // release block AXI and halt scheduler
	REG32(ddrc_base + 0x0078) = 0x44 | (0x1 << 17); // EOP
	REG32(ddrc_base + 0x0070) = idx++;

	// LogMsg(0, "//MC_INIT_TABLE: table done!");
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_org;
}

__maybe_unused static int printf_no_output(const char *fmt, ...)
{
	return 0;
}

static void top_training_fp_all(uint32_t ddr_base, uint32_t cs_num, uint32_t boot_pp, uint32_t pre_init)
{
	u64 to_traning_param[10];
	int (*func)(const char*, ...) = printf;
	void (*training)(void* param);

#if !(LOGLEVEL > 0)
	func = printf_no_output;
#endif

	to_traning_param[0] = ddr_base;
	to_traning_param[1] = cs_num;
	to_traning_param[2] = boot_pp;
	to_traning_param[3] = (u64)func;
	to_traning_param[4] = pre_init;

	training = (void (*)(void* param))DDR_TRAINING_DATA_BASE;
	training(to_traning_param);
}

uint32_t lpddr3_silicon_init(uint32_t ddrc_base, const char *ddr_type, uint32_t data_rate)
{
	unsigned fp = BOOT_PP;
	unsigned int pre_init = 1;
	struct top_ddr_info lp3_info;

	lp3_info.cs_num = 0x2; // pre inic cs_num=2
	lp3_info.vendor = 0x1; // samsung
	lp3_info.density_cs0 = 0x16; // samsung 3G byte
	lp3_info.wds_odt = 1; // 40+60ohm
	lp3_info.rds_odt = 2; // 40+60ohm
	lp3_info.ca_ds_odt = 2;

	LogMsg(0, "wr ds %d, read ds %d, ca ds %d \n", lp3_info.wds_odt,
		lp3_info.rds_odt, lp3_info.ca_ds_odt);

	top_Common_config(BOOT_PP);

	/*Init controller, phy and device */

	LPDDR3_MC_Phy_Device_Init(ddrc_base, BOOT_PP,
		lp3_info.cs_num,
		lp3_info.wds_odt,
		lp3_info.rds_odt,
		lp3_info.ca_ds_odt);

	top_training_fp_all(ddrc_base, lp3_info.cs_num, BOOT_PP, pre_init);

	lp3_info.vendor = Mode_register_read(ddrc_base, 5, 0, 0);
	lp3_info.density_cs0 = Mode_register_read(ddrc_base, 8, 0, 0);

	if ((REG32(ddrc_base + 0x40000 + CS1_OFFSET + 0x70) & 0xff00) == 0x7f00) // cs1 byte0 read gate training code
		lp3_info.cs_num = 0x1; // detect cs num=1
	else
		lp3_info.cs_num = 0x2; // detect cs num=2

	if (lp3_info.cs_num == 2) {
		lp3_info.density_cs1 = Mode_register_read(ddrc_base, 8, 0, 1);
	}

	// update DDR3 rank number
	ddr_cs_num = lp3_info.cs_num;

	REG32(0xd4282800 + 0xb0) &= ~(0x1 << 21);
	REG32(0xd4282800 + 0xb0) |= (0x1 << 21);
	LogMsg(0, "reset mc done \n");
	pre_init = 0;

	if (lp3_info.vendor == 0) {
		LogMsg(0, "vendor is wrong ,read again\n");
		lp3_info.wds_odt = 0;
	}
	LPDDR3_MC_Phy_Device_Init(ddrc_base, BOOT_PP,
		lp3_info.cs_num,
		lp3_info.wds_odt,
		lp3_info.rds_odt,
		lp3_info.ca_ds_odt);

	Adjust_density(ddrc_base,
		lp3_info.vendor,
		lp3_info.density_cs0,
		lp3_info.density_cs1, lp3_info.cs_num);

	/*init frequency change table */
	LogMsg(0, "Set LPM table \n");

	top_training_fp_all(ddrc_base, lp3_info.cs_num, fp, pre_init);

	DDR_lowerpower_HWDFC_flow_config(0xF0000000, lp3_info.cs_num);

	init_table_mc_a0(0xF0000000);

	fp = 1;
	lpddr3_dfc(fp);
	top_training_fp_all(ddrc_base, lp3_info.cs_num, fp, pre_init);

	fp = 2;
	lpddr3_dfc(fp);
	top_training_fp_all(ddrc_base, lp3_info.cs_num, fp, pre_init);

	/* change dram frequency */
	switch(data_rate) {
	case 1066:
		lpddr3_dfc(0);
		break;

	case 1333:
		lpddr3_dfc(1);
		break;

	default:
		data_rate = 1600;
		lpddr3_dfc(2);
		break;
	}

	unsigned DPHY0_BASE = ddrc_base + 0x040000;
	unsigned data = 0;
	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0x4);
	data &= ~(0x10);
	/*Init subPHY-A */
	REG32(DPHY0_BASE + COMMON_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;

	data = REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x4);
	data &= ~(0x10);
	/*Init subPHY-B */
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;

	return data_rate;
}
