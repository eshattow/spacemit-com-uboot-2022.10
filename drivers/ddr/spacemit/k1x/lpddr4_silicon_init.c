// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Spacemit
 */

#ifdef CONFIG_K1_X_BOARD_ASIC

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
#include "ddr_init_asic.h"

#define BOOT_PP		0
#define PMUA_REG_BASE	 0xd4282800
#define PMUA_MCK_CTRL	(PMUA_REG_BASE + 0xe8)
#define PMUA_MC_HW_SLP_TYPE	(PMUA_REG_BASE + 0xb0)
#define REG32(x)	(*((volatile uint32_t *)((uintptr_t)(x))))

#define NEW_FEATURE
#define LOGLEVEL 0
#if (LOGLEVEL > 0)
#define LogMsg(level, format, args...) \
	do { \
		if (level < LOGLEVEL) \
			printf(format, ##args); \
	} while (0)
#else
#define LogMsg(level, format, args...)
#endif

extern u32 ddr_get_density(void);
extern uint32_t get_manufacture_id(void);
extern uint32_t get_ddr_rev_id(void);

#if 1
void top_Phy_reg_dump(unsigned DDRC_BASE,unsigned int fp)
{

	__maybe_unused unsigned DPHY0_BASE=DDRC_BASE+0x040000;
	unsigned i=0;


	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x70),REG32(DPHY0_BASE+0x70));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x74),REG32(DPHY0_BASE+0x74));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x78),REG32(DPHY0_BASE+0x78));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x170),REG32(DPHY0_BASE+0x170));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x1070),REG32(DPHY0_BASE+0x1070));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x1074),REG32(DPHY0_BASE+0x1074));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x1078),REG32(DPHY0_BASE+0x1078));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x1170),REG32(DPHY0_BASE+0x1170));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x3000),REG32(DPHY0_BASE+0x3000));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x3004),REG32(DPHY0_BASE+0x3004));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x3008),REG32(DPHY0_BASE+0x3008));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x300c),REG32(DPHY0_BASE+0x300c));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x3010),REG32(DPHY0_BASE+0x3010));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x3014),REG32(DPHY0_BASE+0x3014));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x3034),REG32(DPHY0_BASE+0x3034));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x4070),REG32(DPHY0_BASE+0x4070));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x4170),REG32(DPHY0_BASE+0x4170));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x5070),REG32(DPHY0_BASE+0x5070));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x5170),REG32(DPHY0_BASE+0x5170));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x7004),REG32(DPHY0_BASE+0x7004));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x7008),REG32(DPHY0_BASE+0x7008));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x700c),REG32(DPHY0_BASE+0x700c));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x7010),REG32(DPHY0_BASE+0x7010));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x7014),REG32(DPHY0_BASE+0x7014));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x8070),REG32(DPHY0_BASE+0x8070));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x8170),REG32(DPHY0_BASE+0x8170));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x9070),REG32(DPHY0_BASE+0x9070));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0x9170),REG32(DPHY0_BASE+0x9170));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xb004),REG32(DPHY0_BASE+0xb004));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xb008),REG32(DPHY0_BASE+0xb008));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xb00c),REG32(DPHY0_BASE+0xb00c));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xb010),REG32(DPHY0_BASE+0xb010));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xb014),REG32(DPHY0_BASE+0xb014));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xc070),REG32(DPHY0_BASE+0xc070));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xc170),REG32(DPHY0_BASE+0xc170));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xd070),REG32(DPHY0_BASE+0xd070));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xd170),REG32(DPHY0_BASE+0xd170));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xf004),REG32(DPHY0_BASE+0xf004));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xf008),REG32(DPHY0_BASE+0xf008));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xf00c),REG32(DPHY0_BASE+0xf00c));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xf010),REG32(DPHY0_BASE+0xf010));
	LogMsg(1,"ADDR[0x%08X]=0x%08X \n",(DPHY0_BASE+0xf014),REG32(DPHY0_BASE+0xf014));

	for(i=0;i<=10;i++)
	{
		LogMsg(1,"PMU ADDR[0x%08X]=0x%08X \n",(0xd4282800+0x398+i*4),REG32(0xd4282800+0x398+i*4));
	}

}
#endif


void enable_PLL(void)
{
	unsigned read_data = 0;
	REG32(0xd4282800 + 0x3b4) &= 0xFFFFFCFF;
	//REG32(0xd4282800+0x3b4)=0x78C20;
	REG32(0xd4282800 + 0x3b4) |= (0x1 << 11) | (0x1 << 8) | (0x1 << 9);
	//REG32(0xd4282800+0x3b4)=0x48F20;
	LogMsg(0, "CKPHY_FC_CTRL[0x%08x]=0x%08x \n", (0xd4282800 + 0x3b4), REG32(0xd4282800 + 0x3b4));
	read_data = REG32(0xd4282800 + 0x3b4);
	while ((read_data & 0x30000) != 0x30000) {
		read_data = REG32(0xd4282800 + 0x3b4);
		LogMsg(0, "CKPHY_FC_CTRL[0x%08x]=0x%08x \n", (0xd4282800 + 0x3b4), REG32(0xd4282800 + 0x3b4));

	}
	LogMsg(0, "CKPHY_FC_CTRL[0x%08x]=0x%08x \n", (0xd4282800 + 0x3b4), REG32(0xd4282800 + 0x3b4));
	LogMsg(0, "DDR PLL ready \n");

	return;
}

void ddr_dfc(unsigned freqNo)
{
	unsigned read_data;

	//PMU_AP_IMR
	REG32(0xd4282800 + 0x098) |= 0x10; //enable dclk fc done interrupt
					   //DFC_TABLE for top mc0
	REG32(0xc0000000 + 0x148) = 0x80ac0000;
	switch (freqNo)
	{
		case 0:
			LogMsg(0, "change to 1200 \n");
			REG32(0xd4282800 + 0x3b4) = 0x00003B50;
			REG32(0xd4282800 + 0x0b0) =
				(1 << TOP_DDRPHY0_EN_OFFSET) |
				(1 << TOP_DCLK_BYPASS_RST_OFFSET) |
				(1 << TOP_FREQ_PLL_CHG_MODE_OFFSET) |
				(0x0 << TOP_MC_REQ_TABLE_NUM_OFFSET);
			break;
		case 1:
			LogMsg(0, "change to 1600 \n");
			REG32(0xd4282800 + 0x3b4) = 0x00003B04;
			REG32(0xd4282800 + 0x0b0) =
				(1 << TOP_DDRPHY0_EN_OFFSET) |
				(1 << TOP_DCLK_BYPASS_RST_OFFSET) |
				(1 << TOP_FREQ_PLL_CHG_MODE_OFFSET) |
				(0x1 << TOP_MC_REQ_TABLE_NUM_OFFSET);
			break;
		case 2:
			LogMsg(0, "change to 2400 \n");
			REG32(0xd4282800 + 0x3b4) = 0x00003b40;
			LogMsg(0, "!!!!!ADDR[0x%08x]=0x%08x !!!! \n", (0xd4282800 + 0x3b4), REG32(0xd4282800 + 0x3b4));
			REG32(0xd4282800 + 0x0b0) =
				(1 << TOP_DDRPHY0_EN_OFFSET) |
				(1 << TOP_DCLK_BYPASS_RST_OFFSET) |
				(1 << TOP_FREQ_PLL_CHG_MODE_OFFSET) |
				(0x2 << TOP_MC_REQ_TABLE_NUM_OFFSET);
			break;
		case 3:
			LogMsg(0, "change to 3200 \n");
			REG32(0xd4282800 + 0x3b4) = 0x00003b00;
			REG32(0xd4282800 + 0x0b0) =
				(1 << TOP_DDRPHY0_EN_OFFSET) |
				(1 << TOP_DCLK_BYPASS_RST_OFFSET) |
				(1 << TOP_FREQ_PLL_CHG_MODE_OFFSET) |
				(0x3 << TOP_MC_REQ_TABLE_NUM_OFFSET);
			break;
		case 4:
			LogMsg(0, "change to ext clock\n");
			REG32(0xd4282800 + 0x3b4) = 0x00003b02;
			REG32(0xd4282800 + 0x0b0) =
				(1 << TOP_DDRPHY0_EN_OFFSET) |
				(1 << TOP_DCLK_BYPASS_RST_OFFSET) |
				(1 << TOP_DCLK_BYPASS_CLK_EN_OFFSET) |
				(1 << TOP_DCLK_BYPASS_DIV_OFFSET);
			break;
		default:
			LogMsg(0, "no this case\n");
			break;

	}

	//LogMsg(0,"Request frequency change \n");
	//PMU_CC_AP
	//REG32(0xd4282800 + 0x004) = (1<<TOP_AP_ALLOW_SPD_CHG)|(TOP_DDR_FREQ_CHG_REQ); //dclk fc trigger
	REG32(0xd4282800 + 0x004) = (0x1 << TOP_DDR_FREQ_CHG_REQ); //dclk fc trigger
								   //LogMsg(0,"Wait frequency change done \n");
								   //pooling PMU_AP_ISR
	read_data = REG32(0xd4282800 + 0x004);
	while ((read_data & 0x400000) != 0x0) {
		read_data = REG32(0xd4282800 + 0x004);
	}
	LogMsg(0, "frequency change done!!!! \n");
	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (0xd4282800 + 0x3b4), REG32(0xd4282800 + 0x3b4));	//(DPHY0_BASE+0x10000

	return;
}


void mck6_sw_fc_top(unsigned freqNo)
{
	unsigned read_data;

	switch (freqNo)
	{
		case 1://1600mbps
			LogMsg(0, "sw frequency change to 1600!!!! \n");
			//CKPHY_FC_CTRL
			REG32(0xd4282800 + 0x3b4) = 0x00003B04; //sel pll2, div 2
								//PMU_MC_HW_SLP_TYPE
			REG32(0xd4282800 + 0x0b0) = 0x40600600; //disable table
								//PMU_CC_AP
			REG32(0xd4282800 + 0x004) = 0x04000000; //dclk fc trigger ??
#if defined(CONFIG_SILENT)
#else
			LogMsg(1, "ADDR[0x%08x]=0x%08x !!!! \n", (0xd4282800 + 0x3b4), REG32(0xd4282800 + 0x3b4));
			LogMsg(1, "ADDR[0x%08x]=0x%08x !!!! \n", (0xd4282800 + 0x0b0), REG32(0xd4282800 + 0x0b0));
			LogMsg(1, "ADDR[0x%08x]=0x%08x !!!! \n", (0xd4282800 + 0x004), REG32(0xd4282800 + 0x004));
			LogMsg(0, "sw frequency change to 1600 start!!!! \n");
#endif
			//pooling trigger bit self clean
			read_data = REG32(0xd4282800 + 0x004);
			while ((read_data & 0x4000000) != 0x0) {
				read_data = REG32(0xd4282800 + 0x004);
			}
			LogMsg(0, "sw frequency change to 1600 done!!!! \n");

			break;
		case 0://1200mbps
			LogMsg(0, "sw frequency change to 1200!!!! \n");
			//CKPHY_FC_CTRL
			REG32(0xd4282800 + 0x3b4) = 0x00003B50; //sel pll1, div 2
								//PMU_MC_HW_SLP_TYPE
			REG32(0xd4282800 + 0x0b0) = 0x40600400; //disable table
								//PMU_CC_AP
			REG32(0xd4282800 + 0x004) = 0x04000000; //dclk fc trigger
			LogMsg(0, "wait sw frequency change to 1200!!!! \n");
			//pooling trigger bit self clean
			read_data = REG32(0xd4282800 + 0x004);
			while ((read_data & 0x4000000) != 0x0) {
				read_data = REG32(0xd4282800 + 0x004);
			}
			LogMsg(0, "sw frequency change to 1200 done!!!! \n");
			break;
		case 3://3200mbps
		       //CKPHY_FC_CTRL
			REG32(0xd4282800 + 0x3b4) = 0x00003B00; //sel pll2, div 1
								//PMU_MC_HW_SLP_TYPE
			REG32(0xd4282800 + 0x0b0) = 0x40600400; //disable table
								//PMU_CC_AP
			REG32(0xd4282800 + 0x004) = 0x04000000; //dclk fc trigger

			//pooling trigger bit self clean
			read_data = REG32(0xd4282800 + 0x004);
			while ((read_data & 0x4000000) != 0x0) {
				read_data = REG32(0xd4282800 + 0x004);
			}
			break;
		case 2://2400mbps
		       //CKPHY_FC_CTRL
			REG32(0xd4282800 + 0x3b4) = 0x00003B40; //sel pll1, div 1
								//PMU_MC_HW_SLP_TYPE
			REG32(0xd4282800 + 0x0b0) = 0x40600400; //disable table
								//PMU_CC_AP
			REG32(0xd4282800 + 0x004) = 0x04000000; //dclk fc trigger

			//pooling trigger bit self clean
			read_data = REG32(0xd4282800 + 0x004);
			while ((read_data & 0x4000000) != 0x0) {
				read_data = REG32(0xd4282800 + 0x004);
			}
			break;
		case 4:
			LogMsg(0, "sw frequency change to ext clk!!!! \n");
			//CKPHY_FC_CTRL
			REG32(0xd4282800 + 0x3b4) = 0x00003B02; //sel pll1, div 2
								//PMU_MC_HW_SLP_TYPE
			REG32(0xd4282800 + 0x0b0) = 0x40600400; //disable table
								//PMU_CC_AP
			REG32(0xd4282800 + 0x004) = 0x04000000; //dclk fc trigger
			LogMsg(0, "wait sw frequency change to ext clk!!!! \n");
			//pooling trigger bit self clean
			read_data = REG32(0xd4282800 + 0x004);
			while ((read_data & 0x4000000) != 0x0) {
				read_data = REG32(0xd4282800 + 0x004);
			}
			LogMsg(0, "sw frequency change to ext clk done!!!! \n");
			break;

		default:
			LogMsg(0, "not support frequency change !!!! \n");
			break;

	}
}

/*fp3 3200
  fp2 2400
  fp1 1600
  fp0 1200
 */
void fp_timing_init(unsigned DDRC_BASE)
{
	unsigned int read_data=0;
	/***** LPDDR4x timing config relate register  ************/
	/*CH0_DRAM_Config_1:  CAS latency  ,   RL3 Option Support ,  CWL, wl_select
CH0_DRAM_Config_2:	 DRAM burst type, write_level_en,  DM  pu_cal,  FSP_OP,  FSP_WR
CH0_DRAM_Config_4:  Read Preamble, Read Postamble, Write Preamble, Write Postamble, Write DBI, Read DBI, SOC_ODT, Vref_Training_Value_DQ,Vref_Training_Range_DQ, Vref_Training_Value_CA,Vref_Training_Range_CA
CH0_ZQC_Timing_0:  			tZQINIT,  tZQCR
CH0_ZQC_Timing_1:  			tZQCL, tZQCS
CH0_Refresh_timing: 			tREFI, tRFC,
CH0_SelfRefresh_timing_0: 	tXSRD, tXSNR
CH0_SelfRefresh_timing_1: 	tCKSRX, tCKSRE, tSR
CH0_PowerDown_timing_0: 	tXARDS, tXP, tCKESR, tCPDED
CH0_PowerDown_timing_1: 	tPDEN
CH0_MRS_timing: 			tMRD, tMOD
CH0_ACT_timing: 			tRAS, tRCD, tRC, tFAW
CH0_PreCharge_Timing: 		tRP, tRTP, tWR, tRPA
CH0_CAS_RAS_timing_0: 		tWTR, tCCD
CH0_CAS_RAS_timing_1:		tRRD
CH0_Off_spec_timing_0:		tCCD_ccs_ext_dly,tCCD_ccs_wr_ext_dly,trwd_ext_dly,twl_early
CH0_Off_spec_timing_1:		read_gap_extend, tccd_ccs_ext_dly_min, tccd_ccs_wr_ext_dly_min
CH0_dram_read_timing:		tDQSCK
CH0_dram_ca_train_timing:	tCACKEL, tCAEXT,tCAENT,tVref_long
	 */
	/************************************************/

	//DDR 3200
	REG32(DDRC_BASE+MC_CH0_BASE+0x0104)= 0xF0800400; //DRAM_Config_2
	// REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x00000E1C;
	// #if defined(SAMSUNG_8GB)
	// REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x00000E24;	// relevent to dbi
	// #else
	REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x00000E20;	// relevent to dbi
	// #endif
	/*on silicon: DRAM_config_4
	  SOC ODT:100 60ohm,
Vref_Training_Range_CA:0
Vref_Training_Value_CA:011001
Vref_Training_Range_DQ:0
Vref_Training_Value_DQ:011001
wr_post:	0.5*tCK
wr_pre: 2*tCK
rd_post: 1.5*tCK
rd_pre: static
	 */
	REG32(DDRC_BASE+MC_CH0_BASE+0x010c)= 0x19194314;	//en r/w dbi(data bus inversion)
	/*
	   on silicon:
	   Device CA ODT : 100  60ohm, bit28~bit30
	   Device DQ ODT:  100  60ohm, bit20~bit22
	   Device pull-down drive strength: 100 60ohm bit16~bit19
	 */
	REG32(DDRC_BASE+MC_CH0_BASE+0x0110)= 0x20440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0114)= 0x20440000;

	//    REG32(DDRC_BASE+MC_CH0_BASE+0x018c) = 0x00500000;   //ZQC timing 0	 no effection
	REG32(DDRC_BASE+MC_CH0_BASE+0x018c) = 0x00000030;   //ZQC timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x0190) = 0x06400030;   //ZQC timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x0194) = 0x80e001c0;   //Refresh timing 0
	// REG32(DDRC_BASE+MC_CH0_BASE+0x0194) = 0x81300260;   //Refresh timing 0
	//    REG32(DDRC_BASE+MC_CH0_BASE+0x01fc) = 0x000d0065;   //Refresh timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01fc) = 0x000C005E;   //Refresh timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x0198) = 0x01CC01CC;   //SelfRefresh timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x019c) = 0x00181818;   //SelfRefresh timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a0) = 0x08180C0C;   //Power down timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a4) = 0x00000003;   //Power down timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a8) = 0x00000217;   //MRS timing
	REG32(DDRC_BASE+MC_CH0_BASE+0x01ac) = 0x30651D44;   //ACT timing
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b0) = 0x1120080F;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b4) = 0x08001000;   //CAS/RAS timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b8) = 0x00000C00;   //CAS/RAS timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01bc) = 0x02020404;   //Off-spec timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c0) = 0x10000004;   //Off-spec timing 1
	//REG32(DDRC_BASE+MC_CH0_BASE+0x01c4) = 0x00000004; //DRAM_read timing  ###check
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c4) = 0x00000006;   //DRAM_read timing  ###pending
	//REG32(DDRC_BASE+MC_CH0_BASE+0x01c8) = 0x00000A0A; //CA Train timing
	REG32(DDRC_BASE+MC_CH0_BASE+0x01d8) = 0x00010190;   //CH0_dram_training_timing [16:10]tOSCO 40ns [9:0]tFC 250ns
	REG32(DDRC_BASE+MC_CH0_BASE+0x014c) = 0x000c4090;   // odt_control_3
#if defined(NEW_FEATURE)
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03e4) = 0x15000A02;   //MCK6 DFI phy ctrl register 1  write leveling
#else
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03e4) = 0x15000C00;   //MCK6 DFI phy ctrl register 1  write leveling
#endif
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03ec) = 0x0000046c;   //CH0_DFI_PHY_Control_3 trdlvl_rr  read training



#if defined(CONFIG_SILENT)
#else
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x104),REG32(DDRC_BASE+MC_CH0_BASE+0x104) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x100),REG32(DDRC_BASE+MC_CH0_BASE+0x100) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x10c),REG32(DDRC_BASE+MC_CH0_BASE+0x10c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x110),REG32(DDRC_BASE+MC_CH0_BASE+0x110) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x114),REG32(DDRC_BASE+MC_CH0_BASE+0x114) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x18c),REG32(DDRC_BASE+MC_CH0_BASE+0x18c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x190),REG32(DDRC_BASE+MC_CH0_BASE+0x190) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x194),REG32(DDRC_BASE+MC_CH0_BASE+0x194) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1fc),REG32(DDRC_BASE+MC_CH0_BASE+0x1fc) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x198),REG32(DDRC_BASE+MC_CH0_BASE+0x198) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x19c),REG32(DDRC_BASE+MC_CH0_BASE+0x19c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1a0),REG32(DDRC_BASE+MC_CH0_BASE+0x1a0) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1a4),REG32(DDRC_BASE+MC_CH0_BASE+0x1a4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1a8),REG32(DDRC_BASE+MC_CH0_BASE+0x1a8) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1ac),REG32(DDRC_BASE+MC_CH0_BASE+0x1ac) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1b0),REG32(DDRC_BASE+MC_CH0_BASE+0x1b0) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1b4),REG32(DDRC_BASE+MC_CH0_BASE+0x1b4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1b8),REG32(DDRC_BASE+MC_CH0_BASE+0x1b8) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1bc),REG32(DDRC_BASE+MC_CH0_BASE+0x1bc) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1c0),REG32(DDRC_BASE+MC_CH0_BASE+0x1c0) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1c4),REG32(DDRC_BASE+MC_CH0_BASE+0x1c4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1d8),REG32(DDRC_BASE+MC_CH0_BASE+0x1d8) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x14c),REG32(DDRC_BASE+MC_CH0_BASE+0x14c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+0x13e4),REG32(DDRC_BASE+MC_CH0_BASE+0x13e4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+0x13ec),REG32(DDRC_BASE+MC_CH0_BASE+0x13ec) );
#endif

	//DDR 2667
	REG32(DDRC_BASE+MC_CH0_BASE+0x0104)= 0xA0800400;
	// REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x00000C18;
	// #if defined(SAMSUNG_8GB)
	// REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x00000C1E;	// relevent to dbi
	// #else
	REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x00000C18;
	// #endif
	/*on silicon: DRAM_config_4
	  SOC ODT:100 60ohm,
Vref_Training_Range_CA:0
Vref_Training_Value_CA:011001
Vref_Training_Range_DQ:0
Vref_Training_Value_DQ:011001
wr_post:	0.5*tCK
wr_pre: 2*tCK
rd_post: 1.5*tCK
rd_pre: static
	 */
	REG32(DDRC_BASE+MC_CH0_BASE+0x010c)= 0x9d194314;

	/*
	   on silicon:
	   Device CA ODT : 100  60ohm, bit28~bit30
	   Device DQ ODT:  100  60ohm, bit20~bit22
	   Device pull-down drive strength: 100 60ohm bit16~bit19
	 */
	REG32(DDRC_BASE+MC_CH0_BASE+0x0110)= 0x00440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0114)= 0x00440000;

	REG32(DDRC_BASE+MC_CH0_BASE+0x018c) = 0x00430000;   //ZQC timing 0, zq calibration reset time, 50ns/0.75=67 cycle
	REG32(DDRC_BASE+MC_CH0_BASE+0x0190) = 0x05350028;   //ZQC timing 1, zq latch 30ns/0.75=40 cycle. 1us/0.75ns=1333 cycle
	REG32(DDRC_BASE+MC_CH0_BASE+0x0194) = 0x80A80151;   //Refresh timing 0, tRFC=280ns, tRFCpb=140ns
	//    REG32(DDRC_BASE+MC_CH0_BASE+0x0194) = 0x80FD01FB;	//Refresh timing 0, tRFC=380ns, tRFCpb=190ns
	//    REG32(DDRC_BASE+MC_CH0_BASE+0x01fc) = 0x000d0065;   //Refresh timing 1, tREFI=3.9us	,fclk=26mhz , tREFIpb=0.488us
	REG32(DDRC_BASE+MC_CH0_BASE+0x01fc) = 0x000C005E;   //Refresh timing 1, tREFI=3.9us	,fclk=24mhz , tREFIpb=0.488us
	REG32(DDRC_BASE+MC_CH0_BASE+0x0198) = 0x017F017F;   //SelfRefresh timing 0	, tRFC+7.5ns
	REG32(DDRC_BASE+MC_CH0_BASE+0x019c) = 0x00141414;   //SelfRefresh timing 1	,15ns/0.75ns=20
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a0) = 0x07140A0A;   //Power down timing 0, tXP=7.5ns, tCPDED=5ns
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a4) = 0x00000003;   //Power down timing 1, tCMDCKE=3 clk
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a8) = 0x00000213;   //MRS timing, tMRD=14ns
	REG32(DDRC_BASE+MC_CH0_BASE+0x01ac) = 0x36541838;   //ACT timing, tRAS, RCD, RC, FAW
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b0) = 0x1c180a18;

	REG32(DDRC_BASE+MC_CH0_BASE+0x01b4) = 0x08000E00;   //CAS/RAS timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b8) = 0x00000E00;   //CAS/RAS timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01bc) = 0x02020404;   //Off-spec timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c0) = 0x10000004;   //Off-spec timing 1
	//REG32(DDRC_BASE+MC_CH0_BASE+0x01c4) = 0x00000004; //DRAM_read timing  ###check
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c4) = 0x00000004;   //DRAM_read timing  ###pending
	//REG32(DDRC_BASE+MC_CH0_BASE+0x01c8) = 0x00000A0A; //CA Train timing
	REG32(DDRC_BASE+MC_CH0_BASE+0x01d8) = 0x0000D94E;   //CH0_dram_training_timing [16:10]tOSCO 40ns [9:0]tFC 250ns
	REG32(DDRC_BASE+MC_CH0_BASE+0x014c) = 0x0007204a;   // odt_control_3
#if defined(NEW_FEATURE)
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03e4) = 0x13000802;   //MCK6 DFI phy ctrl register 1  write leveling
#else
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03e4) = 0x13000A00;   //MCK6 DFI phy ctrl register 1  write leveling
#endif
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03ec) = 0x00000450;   //CH0_DFI_PHY_Control_3 trdlvl_rr  read training



#if defined(CONFIG_SILENT)
#else
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x104),REG32(DDRC_BASE+MC_CH0_BASE+0x104) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x100),REG32(DDRC_BASE+MC_CH0_BASE+0x100) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x10c),REG32(DDRC_BASE+MC_CH0_BASE+0x10c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x110),REG32(DDRC_BASE+MC_CH0_BASE+0x110) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x114),REG32(DDRC_BASE+MC_CH0_BASE+0x114) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x18c),REG32(DDRC_BASE+MC_CH0_BASE+0x18c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x190),REG32(DDRC_BASE+MC_CH0_BASE+0x190) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x194),REG32(DDRC_BASE+MC_CH0_BASE+0x194) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1fc),REG32(DDRC_BASE+MC_CH0_BASE+0x1fc) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x198),REG32(DDRC_BASE+MC_CH0_BASE+0x198) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x19c),REG32(DDRC_BASE+MC_CH0_BASE+0x19c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1a0),REG32(DDRC_BASE+MC_CH0_BASE+0x1a0) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1a4),REG32(DDRC_BASE+MC_CH0_BASE+0x1a4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1a8),REG32(DDRC_BASE+MC_CH0_BASE+0x1a8) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1ac),REG32(DDRC_BASE+MC_CH0_BASE+0x1ac) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1b0),REG32(DDRC_BASE+MC_CH0_BASE+0x1b0) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1b4),REG32(DDRC_BASE+MC_CH0_BASE+0x1b4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1b8),REG32(DDRC_BASE+MC_CH0_BASE+0x1b8) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1bc),REG32(DDRC_BASE+MC_CH0_BASE+0x1bc) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1c0),REG32(DDRC_BASE+MC_CH0_BASE+0x1c0) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1c4),REG32(DDRC_BASE+MC_CH0_BASE+0x1c4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1d8),REG32(DDRC_BASE+MC_CH0_BASE+0x1d8) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x14c),REG32(DDRC_BASE+MC_CH0_BASE+0x14c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+0x13e4),REG32(DDRC_BASE+MC_CH0_BASE+0x13e4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+0x13ec),REG32(DDRC_BASE+MC_CH0_BASE+0x13ec) );
#endif

	//DDR_1600
	REG32(DDRC_BASE+MC_CH0_BASE+0x0104)= 0x50800400;
	// #if defined(SAMSUNG_8GB)
	// REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x00001208;	// relevent to dbi
	// #else
	REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x0000080e;//0x0000080e;
	//   #endif
	/*on silicon: DRAM_config_4
	  SOC ODT:100 60ohm,
Vref_Training_Range_CA:0
Vref_Training_Value_CA:011001
Vref_Training_Range_DQ:0
Vref_Training_Value_DQ:011001
wr_post:  0.5*tCK
wr_pre: 2*tCK
rd_post: 1.5*tCK
rd_pre: static
	 */
	REG32(DDRC_BASE+MC_CH0_BASE+0x010c)= 0x9d194314;

	/*
	   on silicon:
	   Device CA ODT : 100  60ohm, bit28~bit30
	   Device DQ ODT:  100  60ohm, bit20~bit22
	   Device pull-down drive strength: 100 60ohm bit16~bit19
	 */
	REG32(DDRC_BASE+MC_CH0_BASE+0x0110)= 0x00440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0114)= 0x00440000;

	REG32(DDRC_BASE+MC_CH0_BASE+0x018c) = 0x00280018;   //ZQC timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x0190) = 0x03200018;   //ZQC timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x0194) = 0x807000e0;   //Refresh timing 0
	//    REG32(DDRC_BASE+MC_CH0_BASE+0x0194) = 0x80980130;   //Refresh timing 0
	//    REG32(DDRC_BASE+MC_CH0_BASE+0x01fc) = 0x000d0065;   //Refresh timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01fc) = 0x000C005E;   //Refresh timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x0198) = 0x00e600e6;   //SelfRefresh timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x019c) = 0x000c0c0c;   //SelfRefresh timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a0) = 0x050c0606;   //Power down timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a4) = 0x00000003;   //Power down timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a8) = 0x0000020c;   //MRS timing
	REG32(DDRC_BASE+MC_CH0_BASE+0x01ac) = 0x18330f22;   //ACT timing
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b0) = 0x110f080f;

	REG32(DDRC_BASE+MC_CH0_BASE+0x01b4) = 0x08000800;   //CAS/RAS timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b8) = 0x00000600;   //CAS/RAS timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01bc) = 0x02020404;   //Off-spec timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c0) = 0x00000003;   //Off-spec timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c4) = 0x00000003;   //DRAM_read timing
	//REG32(DDRC_BASE+MC_CH0_BASE+0x01c8) = 0x00000A0A; //CA Train timing
	REG32(DDRC_BASE+MC_CH0_BASE+0x01d8) = 0x00008190;   //CH0_dram_training_timing
	REG32(DDRC_BASE+MC_CH0_BASE+0x014c) = 0x00030848;   // odt_control_3
#if defined(NEW_FEATURE)
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03e4) = 0x0a000402;   //MCK6 DFI phy ctrl register 1
#else
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03e4) = 0x0a000600;   //MCK6 DFI phy ctrl register 1
#endif
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03ec) = 0x00000480;   //CH0_DFI_PHY

#if defined(CONFIG_SILENT)
#else
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x104),REG32(DDRC_BASE+MC_CH0_BASE+0x104) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x100),REG32(DDRC_BASE+MC_CH0_BASE+0x100) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x10c),REG32(DDRC_BASE+MC_CH0_BASE+0x10c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x110),REG32(DDRC_BASE+MC_CH0_BASE+0x110) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x114),REG32(DDRC_BASE+MC_CH0_BASE+0x114) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x18c),REG32(DDRC_BASE+MC_CH0_BASE+0x18c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x190),REG32(DDRC_BASE+MC_CH0_BASE+0x190) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x194),REG32(DDRC_BASE+MC_CH0_BASE+0x194) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1fc),REG32(DDRC_BASE+MC_CH0_BASE+0x1fc) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x198),REG32(DDRC_BASE+MC_CH0_BASE+0x198) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x19c),REG32(DDRC_BASE+MC_CH0_BASE+0x19c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1a0),REG32(DDRC_BASE+MC_CH0_BASE+0x1a0) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1a4),REG32(DDRC_BASE+MC_CH0_BASE+0x1a4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1a8),REG32(DDRC_BASE+MC_CH0_BASE+0x1a8) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1ac),REG32(DDRC_BASE+MC_CH0_BASE+0x1ac) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1b0),REG32(DDRC_BASE+MC_CH0_BASE+0x1b0) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1b4),REG32(DDRC_BASE+MC_CH0_BASE+0x1b4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1b8),REG32(DDRC_BASE+MC_CH0_BASE+0x1b8) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1bc),REG32(DDRC_BASE+MC_CH0_BASE+0x1bc) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1c0),REG32(DDRC_BASE+MC_CH0_BASE+0x1c0) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1c4),REG32(DDRC_BASE+MC_CH0_BASE+0x1c4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1d8),REG32(DDRC_BASE+MC_CH0_BASE+0x1d8) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x14c),REG32(DDRC_BASE+MC_CH0_BASE+0x14c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+0x13e4),REG32(DDRC_BASE+MC_CH0_BASE+0x13e4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+0x13ec),REG32(DDRC_BASE+MC_CH0_BASE+0x13ec) );
#endif



	//DDR_1200
	REG32(DDRC_BASE+MC_CH0_BASE+0x0104)= 0x00800400;
	// #if defined(SAMSUNG_8GB)
	// REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x00001208;	// relevent to dbi
	// #else
	REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x0000080e;//0x0000080e;
	// #endif
	/*on silicon: DRAM_config_4
	  SOC ODT:100 60ohm,
Vref_Training_Range_CA:0
Vref_Training_Value_CA:011001
Vref_Training_Range_DQ:0
Vref_Training_Value_DQ:011001
wr_post:	0.5*tCK
wr_pre: 2*tCK
rd_post: 1.5*tCK
rd_pre: static
	 */
	REG32(DDRC_BASE+MC_CH0_BASE+0x010c)= 0x9d194314;
	/*
	   on silicon:
	   Device CA ODT : 100  60ohm, bit28~bit30
	   Device DQ ODT:  100  60ohm, bit20~bit22
	   Device pull-down drive strength: 100 60ohm bit16~bit19
	 */
	REG32(DDRC_BASE+MC_CH0_BASE+0x0110)= 0x00440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0114)= 0x00440000;

	REG32(DDRC_BASE+MC_CH0_BASE+0x018c) = 0x00280018;   //ZQC timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x0190) = 0x03200018;   //ZQC timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x0194) = 0x805400A8;   //Refresh timing 0
	//    REG32(DDRC_BASE+MC_CH0_BASE+0x0194) = 0x807200E4;   //Refresh timing 0
	//    REG32(DDRC_BASE+MC_CH0_BASE+0x01fc) = 0x000d0065;   //Refresh timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01fc) = 0x000C005E;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0198) = 0x00e600e6;   //SelfRefresh timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x019c) = 0x000c0c0c;   //SelfRefresh timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a0) = 0x050c0606;   //Power down timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a4) = 0x00000003;   //Power down timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a8) = 0x0000020c;   //MRS timing
	REG32(DDRC_BASE+MC_CH0_BASE+0x01ac) = 0x18330f22;   //ACT timing
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b0) = 0x110f080f;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b4) = 0x08000800;   //CAS/RAS timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b8) = 0x00000600;   //CAS/RAS timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01bc) = 0x02020404;   //Off-spec timing 0
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c0) = 0x00000002;   //Off-spec timing 1
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c4) = 0x00000003;   //DRAM_read timing
	//REG32(DDRC_BASE+MC_CH0_BASE+0x01c8) = 0x00000A0A; //CA Train timing
	REG32(DDRC_BASE+MC_CH0_BASE+0x01d8) = 0x00008190;   //CH0_dram_training_timing
	REG32(DDRC_BASE+MC_CH0_BASE+0x014c) = 0x00030848;   // odt_control_3
#if defined(NEW_FEATURE)
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03e4) = 0x0a000402;   //MCK6 DFI phy ctrl register 1
#else
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03e4) = 0x0a000600;   //MCK6 DFI phy ctrl register 1
#endif
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03ec) = 0x00000480;   //CH0_DFI_PHY_Control_3 trdlvl_rr
#if defined(CONFIG_SILENT)
#else
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x104),REG32(DDRC_BASE+MC_CH0_BASE+0x104) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x100),REG32(DDRC_BASE+MC_CH0_BASE+0x100) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x10c),REG32(DDRC_BASE+MC_CH0_BASE+0x10c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x110),REG32(DDRC_BASE+MC_CH0_BASE+0x110) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x114),REG32(DDRC_BASE+MC_CH0_BASE+0x114) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x18c),REG32(DDRC_BASE+MC_CH0_BASE+0x18c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x190),REG32(DDRC_BASE+MC_CH0_BASE+0x190) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x194),REG32(DDRC_BASE+MC_CH0_BASE+0x194) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1fc),REG32(DDRC_BASE+MC_CH0_BASE+0x1fc) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x198),REG32(DDRC_BASE+MC_CH0_BASE+0x198) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x19c),REG32(DDRC_BASE+MC_CH0_BASE+0x19c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1a0),REG32(DDRC_BASE+MC_CH0_BASE+0x1a0) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1a4),REG32(DDRC_BASE+MC_CH0_BASE+0x1a4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1a8),REG32(DDRC_BASE+MC_CH0_BASE+0x1a8) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1ac),REG32(DDRC_BASE+MC_CH0_BASE+0x1ac) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1b0),REG32(DDRC_BASE+MC_CH0_BASE+0x1b0) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1b4),REG32(DDRC_BASE+MC_CH0_BASE+0x1b4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1b8),REG32(DDRC_BASE+MC_CH0_BASE+0x1b8) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1bc),REG32(DDRC_BASE+MC_CH0_BASE+0x1bc) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1c0),REG32(DDRC_BASE+MC_CH0_BASE+0x1c0) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1c4),REG32(DDRC_BASE+MC_CH0_BASE+0x1c4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x1d8),REG32(DDRC_BASE+MC_CH0_BASE+0x1d8) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x14c),REG32(DDRC_BASE+MC_CH0_BASE+0x14c) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+0x13e4),REG32(DDRC_BASE+MC_CH0_BASE+0x13e4) );
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+0x13ec),REG32(DDRC_BASE+MC_CH0_BASE+0x13ec) );
#endif


	/*set DQS internal timer runtime=0x10*/
	read_data=REG32(DDRC_BASE+MC_CH0_BASE+0x0108);//DRAM_config_3
	read_data &= 0xF00FFFFF;
	read_data |= (0x10<<20);
	REG32(DDRC_BASE+MC_CH0_BASE+0x0108)=read_data;
	LogMsg(2,"ADDR[0x%08x]=0x%08x !!!! \n",(DDRC_BASE+MC_CH0_BASE+0x108),REG32(DDRC_BASE+MC_CH0_BASE+0x108) );

	return;
}
void fp_sel(unsigned DDRC_BASE,unsigned int fp)
{
	uint32_t data;
	data = REG32(DDRC_BASE + MC_CH0_BASE + 0x0104);
	data &= ~(0xf << 28);
	data |= (fp << 28) | (fp << 30);
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = data;
	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (DDRC_BASE + MC_CH0_BASE + 0x104), REG32(DDRC_BASE + MC_CH0_BASE + 0x104));

	return;
}




void init_table_mc_tim (uint32_t ddrc_base, uint32_t *idx) {
        uint32_t i;
        uint32_t read_data;
        //uint32_t MC_CH0_BASE=0x200;
        uint32_t mc_ch0_phy_base = 0x1000;
        volatile unsigned addrs[ ] = {
                MC_CH0_BASE+0x0100,	//DRAM Config 1 RL/WL
                MC_CH0_BASE+0x010c,	//DRAM Config 4
                MC_CH0_BASE+0x0110,	//DRAM Config 5 cs0
                MC_CH0_BASE+0x0114,	//DRAM Config 5 cs1
                MC_CH0_BASE+0x018c,	//ZQC timing 0
                MC_CH0_BASE+0x0190,	//ZQC timing 1
                MC_CH0_BASE+0x0194,	//Refresh timing
                MC_CH0_BASE+0x0198,	//SelfRefresh timing 0
                MC_CH0_BASE+0x019c,	//SelfRefresh timing 1
                MC_CH0_BASE+0x01a0,	//Power down timing 0
                MC_CH0_BASE+0x01a4,	//Power down timing 1
                MC_CH0_BASE+0x01a8,	//MRS timing
                MC_CH0_BASE+0x01ac,	//ACT timing
                MC_CH0_BASE+0x01b0,	//Pre-Charge timing
                MC_CH0_BASE+0x01b4,	//CAS/RAS timing 0
                MC_CH0_BASE+0x01b8,	//CAS/RAS timing 1
                MC_CH0_BASE+0x01bc,	//Off-spec timing 0	WDQS enable
                MC_CH0_BASE+0x01c0,	//Off-spec timing 1
                MC_CH0_BASE+0x01c4,	//DRAM_read timing
                MC_CH0_BASE+0x0200,	//WDQS timing
                MC_CH0_BASE+0x01d8,   //CH0_dram_training_timing
                MC_CH0_BASE+0x014c,	// odt_control_3
                mc_ch0_phy_base+0x03e4,	//MCK6 DFI phy ctrl register 1
                mc_ch0_phy_base+0x03ec	//CH0_DFI_PHY_Control_3 trdlvl_rr
        };
        uint32_t tim_size = sizeof(addrs)>>2;

        for(i=0;i<tim_size;i++) {
                read_data = REG32(ddrc_base + addrs[i]);
                REG32(ddrc_base + 0x0074) = read_data;
                REG32(ddrc_base + 0x0078) = addrs[i];
                REG32(ddrc_base + 0x0070) = (*idx)++;
        }
}


void init_table_mc_a0(uint32_t ddrc_base)
{
	uint32_t idx = 0x200;
	uint32_t i = 0;
	//uint32_t MC_CH0_BASE=0x200;
	volatile unsigned mc_cfg2_addr = MC_CH0_BASE + 0x0104;
	uint32_t temp_data, mc_cfg2_org, mc_cfg2_fp, mc_ctl0_org;
	volatile unsigned addrs[] = {
		0x0048,
		0x0054,
		0x0058,
		0x0060,
		0x0064,
		0x0148,
		0x014c,
		MC_CH0_BASE + 0x0000,
		MC_CH0_BASE + 0x0004,
		MC_CH0_BASE + 0x0008,
		MC_CH0_BASE + 0x000c,
		MC_CH0_BASE + 0x0020,
		MC_CH0_BASE + 0x0024,
		MC_CH0_BASE + 0x00c4,
		MC_CH0_BASE + 0x00c0,
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
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_fp;//fp0
	REG32(ddrc_base + 0x0074) = mc_cfg2_fp;
	REG32(ddrc_base + 0x0078) = mc_cfg2_addr;
	REG32(ddrc_base + 0x0070) = idx++;
	init_table_mc_tim(ddrc_base, &idx);
	//fp1
	mc_cfg2_fp = temp_data | (0x5 << 28);
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_fp;
	mc_cfg2_fp &= ~(0x3 << 28);	//clr fsp_op
	REG32(ddrc_base + 0x0074) = mc_cfg2_fp;
	REG32(ddrc_base + 0x0078) = mc_cfg2_addr;
	REG32(ddrc_base + 0x0070) = idx++;
	init_table_mc_tim(ddrc_base, &idx);

	//FP2
	mc_cfg2_fp = temp_data | (0xa << 28);
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_fp;
	mc_cfg2_fp &= ~(0x3 << 28);	//clr fsp_op

	REG32(ddrc_base + 0x0074) = mc_cfg2_fp;
	REG32(ddrc_base + 0x0078) = mc_cfg2_addr;
	REG32(ddrc_base + 0x0070) = idx++;
	init_table_mc_tim(ddrc_base, &idx);

	//FP3
	mc_cfg2_fp = temp_data | (0xf << 28);
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_fp;
	mc_cfg2_fp &= ~(0x3 << 28);	//clr fsp_op
	REG32(ddrc_base + 0x0074) = mc_cfg2_fp;
	REG32(ddrc_base + 0x0078) = mc_cfg2_addr;
	REG32(ddrc_base + 0x0070) = idx++;
	init_table_mc_tim(ddrc_base, &idx);

	LogMsg(0, "idx to 0x%x\n", idx);


	//2. trigger phy table
	LogMsg(1, "//MC_INIT_TABLE: 2. trigger phy table! insert table code");
	LogMsg(1, "//MC_INIT_TABLE: NOTE insert table code!");
	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x00020200;	//dphy table addr
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x000013e0;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x13000010;	//dphy table trigger
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x000013d0;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x00010000;	//dphy table done
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x000033fc;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x00010000;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x000033fc;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	//3. get mc_fsp_op/wr dev_fsp_op/wr
	LogMsg(1, "//MC_INIT_TABLE: 3. get mc_fsp_op/wr dev_fsp_op/wr!");
	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x13000008;	//invert_mc_fsp_wr, get from pmu csysfreq
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x00000020;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x13000004;	//invert_mc_fsp_op, get from pmu csysfreq
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x00000020;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x13020000;	//rld_ddr_fsp, dev_fsp_op/wr get from pmu
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x00000028;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	//4. dfi handshake
	LogMsg(1, "//MC_INIT_TABLE: 4. dfi handshake!");
	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x13000001;	//dfi_init_start handhske
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x000013d0;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x00008000;	//get dfi_init_complete
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x000033fc;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x00008000;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x000033fc;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x10000100;	//clear dfi_init_start
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x000013d0;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x00008000;	//get dfi_init_complete
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x000033fc;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x00008000;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x000033fc;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	LogMsg(1, "//MC_INIT_TABLE: 5. MRW!");
	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x1302000d;	//MRW13 for DM
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x00000024;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	/*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x13020003;	//MRW3 for DBI
	/*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x00000024;
	/*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	// /*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x11010004;	//MRR MR4, just for test refrate
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x00000024;
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	// /*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x11050004;	//MRR MR4, just for test refrate
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x00000024;
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	// /*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x12010004;	//MRR MR4, just for test refrate
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x00000024;
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;

	// /*TABLE_C:*/ REG32(ddrc_base + 0x0074) = 0x12050004;	//MRR MR4, just for test refrate
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0078) = 0x00000024;
	// /*TABLE_C:*/ REG32(ddrc_base + 0x0070) = idx++;
	//
	LogMsg(1, "//MC_INIT_TABLE: NOTE insert table code end!");

	LogMsg(0, "REG32(ddrc_base + 0x0074) = 0x%x\n", mc_ctl0_org);
	LogMsg(0, "REG32(ddrc_base + 0x0078) = 0x%x\n", 0x44 | (0x1 << 17));	//EOP
	LogMsg(0, "REG32(ddrc_base + 0x0070) = 0x%x\n", idx);
	REG32(ddrc_base + 0x0074) = mc_ctl0_org;			//release block AXI and halt scheduler
	REG32(ddrc_base + 0x0078) = 0x44 | (0x1 << 17);	//EOP
	REG32(ddrc_base + 0x0070) = idx++;

	LogMsg(0, "//MC_INIT_TABLE: table done!");

	/*TABLE_C:*/ REG32(ddrc_base + 0x40000 + 0x10104) = 0x00001100;
	/*TABLE_C:*/ REG32(ddrc_base + 0x40000 + 0x10108) = 0x00010000;
	/*TABLE_C:*/ REG32(ddrc_base + 0x40000 + 0x10100) = 0x00000020;

	/*TABLE_C:*/ REG32(ddrc_base + 0x40000 + 0x10104) = 0x000000ff;
	/*TABLE_C:*/ REG32(ddrc_base + 0x40000 + 0x10108) = 0x0001001c;
	/*TABLE_C:*/ REG32(ddrc_base + 0x40000 + 0x10100) = 0x00000021;

	/*TABLE_C:*/ REG32(ddrc_base + 0x40000 + 0x10104) = 0x00000000;
	/*TABLE_C:*/ REG32(ddrc_base + 0x40000 + 0x10108) = 0x0005001c;
	/*TABLE_C:*/ REG32(ddrc_base + 0x40000 + 0x10100) = 0x00000022;

	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_org;
}


void ddr_dfc_table_init(unsigned int DDRC_BASE)
{
	// start of DFC table 0
	//Halt scdlr
	//REG32(DDRC_BASE + 0x74) = 0x00040403;
#if defined(X16_MODE)
	REG32(DDRC_BASE + 0x74) = 0x00040203;
#else
	REG32(DDRC_BASE + 0x74) = 0x00040303;
#endif

	REG32(DDRC_BASE + 0x78) = 0x00000044;
	REG32(DDRC_BASE + 0x70) = 0x00000000;
	//user cmd0 change MC fsp_wr
	REG32(DDRC_BASE + 0x74) = 0x13000008;
	REG32(DDRC_BASE + 0x78) = 0x00000020;
	REG32(DDRC_BASE + 0x70) = 0x00000001;
	//user cmd2 invert DDR FSP_WR
	REG32(DDRC_BASE + 0x74) = 0x13010000;
	REG32(DDRC_BASE + 0x78) = 0x00000028;
	REG32(DDRC_BASE + 0x70) = 0x00000002;
	//user cmd1 MRW13 invert DDR FSP_WR
	REG32(DDRC_BASE + 0x74) = 0x1302000d;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000003;
	//user cmd 1 MRW1
	REG32(DDRC_BASE + 0x74) = 0x13020001;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000004;
	//user cmd 1 MRW2
	REG32(DDRC_BASE + 0x74) = 0x13020002;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000005;
	//user cmd 1 MRW3
	REG32(DDRC_BASE + 0x74) = 0x13020003;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000006;
	//user cmd 1 MRW11
	REG32(DDRC_BASE + 0x74) = 0x1302000b;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000007;
	//user cmd 1 MRW12
	REG32(DDRC_BASE + 0x74) = 0x1302000c;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000008;
	//user cmd 1 MRW14
	REG32(DDRC_BASE + 0x74) = 0x1302000e;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000009;
	//user cmd 1 MRW22
	REG32(DDRC_BASE + 0x74) = 0x13020016;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x0000000a;
	//user cmd 2 invert DDR FSP_OP
	REG32(DDRC_BASE + 0x74) = 0x13008000;
	REG32(DDRC_BASE + 0x78) = 0x00000028;
	REG32(DDRC_BASE + 0x70) = 0x0000000b;
	//user cmd 1 MRW13 invert DDR FSP_OP
	REG32(DDRC_BASE + 0x74) = 0x1302000d;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x0000000c;
	//user cmd 0 send APD cmd
	REG32(DDRC_BASE + 0x74) = 0x13000010;
	REG32(DDRC_BASE + 0x78) = 0x00000020;
	REG32(DDRC_BASE + 0x70) = 0x0000000d;
	//check PD enter status(read mask)
	REG32(DDRC_BASE + 0x74) = 0x00000002;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x0000000e;
	//check PD enter status(read data)
	REG32(DDRC_BASE + 0x74) = 0x00000002;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x0000000f;
	//drive high dfi_init_start
	REG32(DDRC_BASE + 0x74) = 0x13000001;
	REG32(DDRC_BASE + 0x78) = 0x000013d0;
	REG32(DDRC_BASE + 0x70) = 0x00000010;
	//check dfi_init_complete low(read mask)
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000011;
	//check dfi_init_complete low(read data)
	REG32(DDRC_BASE + 0x74) = 0x00000000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000012;
	//dummy write for wait pmu
	//REG32(DDRC_BASE + 0x74) = 0x00040303;
#if defined(X16_MODE)
	REG32(DDRC_BASE + 0x74) = 0x00040203;
#else
	REG32(DDRC_BASE + 0x74) = 0x00040303;
#endif

	REG32(DDRC_BASE + 0x78) = 0x00010044;
	REG32(DDRC_BASE + 0x70) = 0x00000013;
	//drive low dfi_init_start
	REG32(DDRC_BASE + 0x74) = 0x10000100;
	REG32(DDRC_BASE + 0x78) = 0x000013d0;
	REG32(DDRC_BASE + 0x70) = 0x00000014;
	//check dfi_init_complete high(read mask)
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000015;
	//check dfi_init_complete high(read data)
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000016;
	//user cmd 0 change MC fsp_op
	REG32(DDRC_BASE + 0x74) = 0x13000004;
	REG32(DDRC_BASE + 0x78) = 0x00000020;
	REG32(DDRC_BASE + 0x70) = 0x00000017;
	//user cmd 1 MRW13 disable VRCG
	REG32(DDRC_BASE + 0x74) = 0x1302000d;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000018;
	//check PD exit status(read mask)
	REG32(DDRC_BASE + 0x74) = 0x00000002;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x00000019;
	//check PD exit status(read data)
	REG32(DDRC_BASE + 0x74) = 0x00000000;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x0000001a;

	/*REG32(DDRC_BASE + 0x74) = 0x11100000;//read gate training cs0
	  REG32(DDRC_BASE + 0x78) = 0x000013d0;
	  REG32(DDRC_BASE + 0x70) = 0x0000001b;

	  REG32(DDRC_BASE + 0x74) = 0x00000006;
	  REG32(DDRC_BASE + 0x78) = 0x000033fc;
	  REG32(DDRC_BASE + 0x70) = 0x0000001c;

	  REG32(DDRC_BASE + 0x74) = 0x00000006;
	  REG32(DDRC_BASE + 0x78) = 0x000033fc;
	  REG32(DDRC_BASE + 0x70) = 0x0000001d;

	  REG32(DDRC_BASE + 0x74) = 0x12100000;//read gate training cs1
	  REG32(DDRC_BASE + 0x78) = 0x000013d0;
	  REG32(DDRC_BASE + 0x70) = 0x0000001e;

	  REG32(DDRC_BASE + 0x74) = 0x00000006;
	  REG32(DDRC_BASE + 0x78) = 0x000033fc;
	  REG32(DDRC_BASE + 0x70) = 0x0000001f;

	  REG32(DDRC_BASE + 0x74) = 0x00000006;
	  REG32(DDRC_BASE + 0x78) = 0x000033fc;
	  REG32(DDRC_BASE + 0x70) = 0x00000020;*/

	/*REG32(DDRC_BASE + 0x74) = 0x11200000;//read leveling cs0
	  REG32(DDRC_BASE + 0x78) = 0x000013d0;
	  REG32(DDRC_BASE + 0x70) = 0x00000021;

	  REG32(DDRC_BASE + 0x74) = 0x00000006;
	  REG32(DDRC_BASE + 0x78) = 0x000033fc;
	  REG32(DDRC_BASE + 0x70) = 0x00000022;

	  REG32(DDRC_BASE + 0x74) = 0x00000006;
	  REG32(DDRC_BASE + 0x78) = 0x000033fc;
	  REG32(DDRC_BASE + 0x70) = 0x00000023;

	  REG32(DDRC_BASE + 0x74) = 0x12200000;//read leveling cs1
	  REG32(DDRC_BASE + 0x78) = 0x000013d0;
	  REG32(DDRC_BASE + 0x70) = 0x00000024;

	  REG32(DDRC_BASE + 0x74) = 0x00000006;
	  REG32(DDRC_BASE + 0x78) = 0x000033fc;
	  REG32(DDRC_BASE + 0x70) = 0x00000025;

	  REG32(DDRC_BASE + 0x74) = 0x00000006;
	  REG32(DDRC_BASE + 0x78) = 0x000033fc;
	  REG32(DDRC_BASE + 0x70) = 0x00000026;*/

	/*REG32(DDRC_BASE + 0x74) = 0x11080000;//write dq leveling
	  REG32(DDRC_BASE + 0x78) = 0x000013d0;
	  REG32(DDRC_BASE + 0x70) = 0x00000021;

	  REG32(DDRC_BASE + 0x74) = 0x00000060;
	  REG32(DDRC_BASE + 0x78) = 0x000033fc;
	  REG32(DDRC_BASE + 0x70) = 0x00000022;

	  REG32(DDRC_BASE + 0x74) = 0x00000060;
	  REG32(DDRC_BASE + 0x78) = 0x000033fc;
	  REG32(DDRC_BASE + 0x70) = 0x00000023;*/



	//resume scdlr(end of table)
	//REG32(DDRC_BASE + 0x74) = 0x00040380;
#if defined(X16_MODE)
	REG32(DDRC_BASE + 0x74) = 0x00040280;
#else
	REG32(DDRC_BASE + 0x74) = 0x00040380;
#endif

	REG32(DDRC_BASE + 0x78) = 0x00020044;
	REG32(DDRC_BASE + 0x70) = 0x0000001b;
	// end of DFC table 0


	//resume scdlr(end of table)
	//REG32(DDRC_BASE + 0x74) = 0x00040380;
#if defined(X16_MODE)
	REG32(DDRC_BASE + 0x74) = 0x00040280;
#else
	REG32(DDRC_BASE + 0x74) = 0x00040380;
#endif

	REG32(DDRC_BASE + 0x78) = 0x00020044;
	REG32(DDRC_BASE + 0x70) = 0x0000012e;
	// end of DFC table 3

	// start of DFC table 4
	// program table LP
	// Halt Scheduler and Set DFC Mode= 1!
	//REG32(DDRC_BASE + 0x74) = 0x00040b43;
#if defined(X16_MODE)
	REG32(DDRC_BASE + 0x74) = 0x00040A43;
#else
	REG32(DDRC_BASE + 0x74) = 0x00040b43;
#endif

	REG32(DDRC_BASE + 0x78) = 0x00000044;
	REG32(DDRC_BASE + 0x70) = 0x00000180;
	// Write Reg Tb 12.0: Addr: 44, Data: 40b03, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0
	// cke_seq [mcUvmRegUC0Seq] DFC_TB User_CMD_0 in CH0001CS0011, reg=13000010!
	REG32(DDRC_BASE + 0x74) = 0x13000010;
	REG32(DDRC_BASE + 0x78) = 0x00000020;
	REG32(DDRC_BASE + 0x70) = 0x00000181;
	// Write Reg Tb 12.1: Addr: 20, Data: 13000010, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0

	// check dram status
	REG32(DDRC_BASE + 0x74) = 0x00000002;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x00000182;
	// Write Reg Tb 12.2: Addr: 8, Data: 2, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1

	REG32(DDRC_BASE + 0x74) = 0x00000002;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x00000183;
	// Write Reg Tb 12.3: Addr: 8, Data: 2, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1

	// dfi_init_start
	REG32(DDRC_BASE + 0x74) = 0x13000001;
	REG32(DDRC_BASE + 0x78) = 0x000013d0;
	REG32(DDRC_BASE + 0x70) = 0x00000184;
	// Write Reg Tb 12.4: Addr: 13d0, Data: 13000001, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0

	// check dfi_init_complete
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000185;
	// Write Reg Tb 12.5: Addr: 13fc, Data: 8000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1

	REG32(DDRC_BASE + 0x74) = 0x00000000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000186;
	// Write Reg Tb 12.6: Addr: 13fc, Data: 0, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1
	//
	// DFC_TB Halt Scheduler and Set DFC Mode= 1, halt
	//REG32(DDRC_BASE + 0x74) = 0x00040b03;
#if defined(X16_MODE)
	REG32(DDRC_BASE + 0x74) = 0x00040A43;
#else
	REG32(DDRC_BASE + 0x74) = 0x00040b43;
#endif
	REG32(DDRC_BASE + 0x78) = 0x00010044;
	REG32(DDRC_BASE + 0x70) = 0x00000187;
	// Write Reg Tb 12.7: Addr: 44, Data: 40b03, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 1, EOP: 0, RD=0

	// deassert dfi_init_start
	REG32(DDRC_BASE + 0x74) = 0x10000100;
	REG32(DDRC_BASE + 0x78) = 0x000013d0;
	REG32(DDRC_BASE + 0x70) = 0x00000188;
	// Write Reg Tb 12.8: Addr: 13d0, Data: 10000100, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0

	// check dfi_init_complete
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000189;
	// Write Reg Tb 12.9: Addr: 13fc, Data: 8000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1

	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x0000018a;
	// Write Reg Tb 12.10: Addr: 13fc, Data: 8000, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1

	// DFC_TB MRW to Reg13 in CH0001CS0011
	REG32(DDRC_BASE + 0x74) = 0x1302000d;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x0000018b;
	// Write Reg Tb 12.11: Addr: 24, Data: 1302000d, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=0

	// check dram status
	REG32(DDRC_BASE + 0x74) = 0x00000002;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x0000018c;
	// Write Reg Tb 12.12: Addr: 8, Data: 2, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1

	REG32(DDRC_BASE + 0x74) = 0x00000000;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x0000018d;
	// Write Reg Tb 12.13: Addr: 8, Data: 0, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 0, RD=1
	//
	// TRAIN_ALL is enable, no training in DFC and LP
	// DFC_TB Resume Scheduler and Clear DFC Mode= 0!
	//REG32(DDRC_BASE + 0x74) = 0x00040b00;
#if defined(X16_MODE)
	REG32(DDRC_BASE + 0x74) = 0x00040A00;
#else
	REG32(DDRC_BASE + 0x74) = 0x00040b00;
#endif

	REG32(DDRC_BASE + 0x78) = 0x00020044;
	REG32(DDRC_BASE + 0x70) = 0x0000018e;
	// Write Reg Tb 12.14: Addr: 44, Data: 40b00, REG_WRITE_DISABLE: 0, REQ_PHY: 0, REQ_PMU: 0, EOP: 1, RD=0
	// end of DFC table 4



}

void top_DDR_MC_init(unsigned DDRC_BASE,unsigned int fp)
{

	/*Init MC misc register */
	REG32(DDRC_BASE + 0x44) = 0x00040300;			/*Data_Width:x32, Burst_Length: BL16*/
	REG32(DDRC_BASE + 0x48) = 0x00000001;			/*exclu_en: Enable exclusive access monitoring*/
	REG32(DDRC_BASE + 0x64) = 0x100d0803;			//RDP_Control,3200,2400,1600,1200
	REG32(DDRC_BASE + 0x50) = 0x000000ff;		/*spool_2cycle_mode:1, starv_timer_init: 0x3F*/
	REG32(DDRC_BASE + 0x58) = 0x3fd53fd5;		//0x3fd43fd4; enable auto drain
	REG32(DDRC_BASE + 0x180) = 0x00010200;	/*rpp_starvation_en: 1 enable*/


	/*Micron*/
	/*Memory Address Map Register Low CS0, CS0 Area length: 4GB, 32Gbit,start address: 0*/
	REG32(DDRC_BASE + MC_CH0_BASE) = 0x100001;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x4) = 0x0;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x8) = 0x100001;
	REG32(DDRC_BASE + MC_CH0_BASE + 0xc) = 0x1;


	REG32(DDRC_BASE + 0x0080) = 0x00000000;						// close TZ filter
	REG32(DDRC_BASE + 0x0a00) = 0x00000000;						// change range0 nsaid mask
	REG32(DDRC_BASE + 0x0ac0) = 0x00000000;						// change undefined nsaid mask
	REG32(DDRC_BASE + 0x0acc) = 0xffffffff;						// open TZ intrrupt and resp

	/*Init Bank,row,column*/
	/*MC Configuration  CS0:
	  Number of Banks: 8
	  Number of Bank groups: 1
	  Number of Column address bits: 10
	  Number of Row address bits:17
	  Number of Stack Chips:1
	  Bank address assignment boundary:4KB
	  DDR device type: X32
	  */
	REG32(DDRC_BASE + MC_CH0_BASE + 0x20) = 0x05030732;//8 bank, 17 row, 10 column
	REG32(DDRC_BASE + MC_CH0_BASE + 0x24) = 0x05030732;//8 bank, 17 row, 10 column


	/*Init MC feature*/
	/*MC_Control_1:
tw2r_dis: enable tW2R
acs_en: Disable auto clock stop mode
aps_ppd: Active Power-down
*/
	REG32(DDRC_BASE + MC_CH0_BASE + 0xc0) = 0x14008000;//0x00000000;
	/*MC_Control_2:
S4_type: LPDDR2 S4 type DRAM
SDRAM_type: LPDDR4
*/
	REG32(DDRC_BASE + MC_CH0_BASE + 0xc4) = 0x000000b8;
	/*MC_Control_3:
	  PU CAL bit17: 1  VDDQ*0.6
	  VRCG bit16: 1
phy_in_ff_bypass: 	bypass register slice flop
phy_out_ff_bypass: bypass register slice flop
*/
	REG32(DDRC_BASE + MC_CH0_BASE + 0xc8) = 0x0000FFFF;  //PU CAL=0, VOH=0.6*vddq

	/*set self-refresh time*/
	REG32(DDRC_BASE + MC_CH0_BASE + 0xcc) = 0x200;//512 mc clock cycle

	/*Configure 4 frequency point timing*/
	fp_timing_init(DDRC_BASE);
	/*select a frequency point to run*/
	fp_sel(DDRC_BASE, fp);

	/*Init DDR init timing*/
	/*
	   DDR init timing Control 0: init_count_nop
	   DDR init timing Control 1: init_count
	   DDR init timing Control 2: reset_count, cke_count
	   */
	switch (fp)
	{
		case 3:
			REG32(DDRC_BASE + MC_CH0_BASE + 0x0180) = 0x30D400;
			REG32(DDRC_BASE + MC_CH0_BASE + 0x0184) = 0x4E200;
			REG32(DDRC_BASE + MC_CH0_BASE + 0x0188) = 0xC800000;
			REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3E0) |= 3 << 2;
			break;
		case 2:
			REG32(DDRC_BASE + MC_CH0_BASE + 0x0180) = 0x30D400;
			REG32(DDRC_BASE + MC_CH0_BASE + 0x0184) = 0x4E200;
			REG32(DDRC_BASE + MC_CH0_BASE + 0x0188) = 0xC800000;
			REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3E0) |= 2 << 2;
			break;
		case 1:
			REG32(DDRC_BASE + MC_CH0_BASE + 0x0180) = 0x30D400;
			REG32(DDRC_BASE + MC_CH0_BASE + 0x0184) = 0x4E200;
			REG32(DDRC_BASE + MC_CH0_BASE + 0x0188) = 0xC800000;
			REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3E0) |= 1 << 2;
			break;
		case 0:
			REG32(DDRC_BASE + MC_CH0_BASE + 0x0180) = 0x30D400;
			REG32(DDRC_BASE + MC_CH0_BASE + 0x0184) = 0x4E200;
			REG32(DDRC_BASE + MC_CH0_BASE + 0x0188) = 0xC800000;
			REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3E0) |= 0 << 2;

			break;
		default:
			break;
	}

#if defined(CONFIG_SILENT)
#else
	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (DDRC_BASE + MC_CH0_BASE + 0x180), REG32(DDRC_BASE + MC_CH0_BASE + 0x180));
	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (DDRC_BASE + MC_CH0_BASE + 0x184), REG32(DDRC_BASE + MC_CH0_BASE + 0x184));
	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (DDRC_BASE + MC_CH0_BASE + 0x188), REG32(DDRC_BASE + MC_CH0_BASE + 0x188));
	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (DDRC_BASE + 0x13e0), REG32(DDRC_BASE + 0x13e0));
#endif

	return;
}
void top_DDR_wr_ds_odt_vref(unsigned DPHY0_BASE,unsigned combination)
{
	unsigned data = 0;
	unsigned d_reg2 = 0;

	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0xc);
	switch (combination)
	{
		/*SOC drive=40ohm, device ODT=60ohm, Vref=0.3*vddq*/
		case 2:
			d_reg2 = 0xd8;
			data &= 0xFFFF00FF;
			data |= (d_reg2 << 8);
			REG32(DPHY0_BASE + COMMON_OFFSET + 0xc) = data;
			REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc) = data;
			REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc) = data;
			REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc) = data;

			REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0xc) = data;
			REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc) = data;
			REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc) = data;
			REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc) = data;

			break;
		default://ODT off
			LogMsg(0, "not support.....\n");
			break;
	}
}
void top_DDR_rx_ds_odt_vref(unsigned DPHY0_BASE,unsigned combination)
{
	unsigned data = 0;
	unsigned d_reg3 = 0;
	unsigned rx_ref_d1 = 0x0, rx_ref_d2 = 0x0;


	switch (combination)
	{

		/*SOC ODT=60ohm, device drive=40ohm, Vref=0.3*vddq */
		case 2:
			data = REG32(DPHY0_BASE + COMMON_OFFSET + 0xc);
			data &= 0xFF00FFFF;
			d_reg3 = 0xE4;
			data |= (d_reg3 << 16);
			REG32(DPHY0_BASE + COMMON_OFFSET + 0xc) = data;
			REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc) = data;
			REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc) = data;
			REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc) = data;

			REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0xc) = data;
			REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc) = data;
			REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc) = data;
			REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc) = data;

			data = REG32(DPHY0_BASE + COMMON_OFFSET + 0x4);
			data &= 0x0000FFFF;
			rx_ref_d1 = 0x55;//high 4 bit for DQS, low for bit for DQ,both vref=0.3*vddq
			rx_ref_d2 = 0x55;//high 4 bit for DQS, low for bit for DQ
			data |= (rx_ref_d1 << 16) | (rx_ref_d2 << 24);

			REG32(DPHY0_BASE + COMMON_OFFSET + 0x4) = data;
			REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
			REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
			REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;

			REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x4) = data;
			REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
			REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
			REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;
			break;
		default:
			LogMsg(0, "not support.....\n");
			break;
	}
}

void top_DDR_amble_config(unsigned DPHY0_BASE)
{
	unsigned data = 0;
	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0x4);
	data &= 0xFFFF0FFF; 				/*clear write and read pre/post amble to 0*/
	/*
	   en_rd_odt=1: odt on in read mode
	   write preamble =1
	   write postamble=0
	   read preamble=0
	   read postamble=1
	   */
	data |= (1 << 11) | (1 << 13) | (1 << 15);			/*enable DQ/DQS read ODT, set wr-preamble = 1*/
	/*Init subPHY-A*/
	REG32(DPHY0_BASE + COMMON_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;
	/*Init subPHY-B*/
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;

	return;
}

void top_DDR_phy_init(unsigned DDRC_BASE,unsigned fp)
{
	unsigned DPHY0_BASE = DDRC_BASE + 0x040000;

	unsigned data = 0;

	unsigned device_type = 0;
	unsigned i = 0;

	/*3.	Set aon_reg2 to 0xf (PMUAP.ddr_ckphy_ctrl3.bit[15:8])*/
	REG32(0xd4282800 + 0x3A4) &= 0xFFFF00FF;
	REG32(0xd4282800 + 0x3A4) |= (0xF << 8);

	/*4 read device type*/
	for (i = 0; i < 4; i++) {

		device_type = REG32(0xd4282800 + 0x3B8);
		LogMsg(0, "Address[0x%08x]=0x%08x \n", (0xd4282800 + 0x3B8), REG32(0xd4282800 + 0x3B8));
		device_type = ((device_type & 0x03000000) >> 24);
		LogMsg(0, "%d times read device type = 0x%x \n", i, device_type);
	}



	/*5, set device type*/
	REG32(0xd4282800 + 0x398) |= (0x3 << 10);
	LogMsg(0, "Address[0x%08x]=0x%08x \n", (0xd4282800 + 0x398), REG32(0xd4282800 + 0x398));


	REG32(DPHY0_BASE + COMMON_OFFSET) = 0x0;
	REG32(DPHY0_BASE + COMMON_OFFSET + subPHY_B_OFFSET) = 0x0;

	REG32(DPHY0_BASE + COMMON_OFFSET) = 0x1;
	REG32(DPHY0_BASE + COMMON_OFFSET + subPHY_B_OFFSET) = 0x1;

#if defined(NEW_FEATURE)
	/*start phy configuration*/
	REG32(DPHY0_BASE + 0x0064) = 0x4349;//data;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET + 0x0064) = 0x4349;//data;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 2 + 0x064) = 0x4349;//data;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 3 + 0x064) = 0x4349;//data;
#else
	/*start phy configuration*/
	REG32(DPHY0_BASE + 0x0064) = 0x349;//data;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET + 0x0064) = 0x349;//data;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 2 + 0x064) = 0x349;//data;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 3 + 0x064) = 0x349;//data;
#endif


	top_DDR_amble_config(DPHY0_BASE);

	top_DDR_wr_ds_odt_vref(DPHY0_BASE, 2);

	top_DDR_rx_ds_odt_vref(DPHY0_BASE, 2);

	//top_DDR_lpbk_config(DPHY0_BASE);





	LogMsg(0, "PHY config address[0x%08x]=0x%08x \n", (DPHY0_BASE + COMMON_OFFSET + 0x4), REG32(DPHY0_BASE + COMMON_OFFSET + 0x4));

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


	/*data=REG32(DPHY0_BASE+COMMON_OFFSET+0x10);
	  data&=0xFFFF00FF;
	  data |= (0x44<<8);
	  REG32(DPHY0_BASE+COMMON_OFFSET+0x10)=data;
	  REG32(DPHY0_BASE+subPHY_B_OFFSET+COMMON_OFFSET+0x10)=data;

	  REG32(DPHY0_BASE+COMMON_OFFSET+FREQ_POINT_OFFSET+0x10)=data;
	  REG32(DPHY0_BASE+subPHY_B_OFFSET+COMMON_OFFSET+FREQ_POINT_OFFSET+0x10)=data;

	  REG32(DPHY0_BASE+COMMON_OFFSET+FREQ_POINT_OFFSET*2+0x10)=data;
	  REG32(DPHY0_BASE+subPHY_B_OFFSET+COMMON_OFFSET+FREQ_POINT_OFFSET*2+0x10)=data;

	  REG32(DPHY0_BASE+COMMON_OFFSET+FREQ_POINT_OFFSET*3+0x10)=data;
	  REG32(DPHY0_BASE+subPHY_B_OFFSET+COMMON_OFFSET+FREQ_POINT_OFFSET*3+0x10)=data;*/

	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0x10);
	//data&=0xF7FFFFFF;
	//data |= (0x44<<8);
	data |= 0x10000000;//enable duty calibration

	REG32(DPHY0_BASE + COMMON_OFFSET + 0x10) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x10) = data;

	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x10) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x10) = data;

	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x10) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x10) = data;

	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x10) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x10) = data;




	LogMsg(0, "a_cr0[0x%08x]=0x%08x \n", (DPHY0_BASE + COMMON_OFFSET), REG32(DPHY0_BASE + COMMON_OFFSET));
	LogMsg(0, "a_cr3[0x%08x]=0x%08x \n", (DPHY0_BASE + COMMON_OFFSET + 0xc), REG32(DPHY0_BASE + COMMON_OFFSET + 0xc));
	LogMsg(0, "a_cr5[0x%08x]=0x%08x \n", (DPHY0_BASE + COMMON_OFFSET + 0x14), REG32(DPHY0_BASE + COMMON_OFFSET + 0x14));




	/*set reg for r-cali*/
	REG32(DPHY0_BASE + COMMON_OFFSET + 0x30) = 0x1077;
	//LogMsg(0,"ADDR[0x%08x]=0x%08x !!!! \n",(DPHY0_BASE+COMMON_OFFSET+0x18),REG32(DPHY0_BASE+COMMON_OFFSET+0x18) );

	/*set phy pipe line*/
	REG32(DPHY0_BASE + OTHER_CONTROL_OFFSET + 0x24) = 0x0;
	//LogMsg(0,"ADDR[0x%08x]=0x%08x !!!! \n",(DPHY0_BASE+OTHER_CONTROL_OFFSET+0x24),REG32(DPHY0_BASE+OTHER_CONTROL_OFFSET+0x24) );

	/*Init other control
	  enable phy hwdfc_en*/
	REG32(DPHY0_BASE + OTHER_CONTROL_OFFSET) |= 0x1;
	//LogMsg(0,"ADDR[0x%08x]=0x%08x !!!! \n",(DPHY0_BASE+OTHER_CONTROL_OFFSET),REG32(DPHY0_BASE+OTHER_CONTROL_OFFSET) );

#if defined(CONFIG_SILENT)
#else
	top_Phy_reg_dump(DDRC_BASE, fp);
	LogMsg(0, "[0x%08x]=0x%08x \n", (0xc000030c), REG32(0xc000030c));
	LogMsg(0, "[0x%08x]=0x%08x \n", (0xc0000310), REG32(0xc0000310));
	LogMsg(0, "[0x%08x]=0x%08x \n", (0xc0000314), REG32(0xc0000314));
#endif

	LogMsg(0, "pinmux [0x%08x]=0x%08x \n", (DPHY0_BASE + 0x10018), REG32(DPHY0_BASE + 0x10018));
	LogMsg(0, "rdo [0x%08x]=0x%08x \n", (DPHY0_BASE + 0x3034), REG32(DPHY0_BASE + 0x3034));
	LogMsg(0, "debug_ctrl_0 [0x%08x]=0x%08x \n", (DPHY0_BASE + 0x10030), REG32(DPHY0_BASE + 0x10030));

	return;

}

void top_Common_config(void)
{
	/*1.	Set PLL1.PLL_REG1 to 0x3B(PMUAP.ddr_ckphy_ctrl1.bit[15:8])*/
	REG32(0xd4282800 + 0x39c) &= 0xFFFF00FF;
	REG32(0xd4282800 + 0x39c) |= (0x3B << 8);

	enable_PLL();

	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (0xd4282800 + 0x3a8), REG32(0xd4282800 + 0x3a8));
	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (0xd4282800 + 0x3ac), REG32(0xd4282800 + 0x3ac));

	/*Config boot frequency to 1200Mbps*/
	mck6_sw_fc_top(BOOT_PP);

	REG32(0xd42828e8) &= 0xFFFFFFFC;	  //release MCK6 reset and enable MCK6 hclk
	REG32(0xd42828e8) |= 0x3;

	return;
}


void top_DDR_MC_Phy_Device_Init(unsigned int DDRC_BASE,unsigned int cs_val,unsigned int fp)
{
	unsigned DFI_PHY_USER_COMMAND_0 = DDRC_BASE + 0x13D0;
	__maybe_unused unsigned DPHY0_BASE = DDRC_BASE + 0x40000;
	unsigned read_data = 0;
	unsigned cs_num;

	if (cs_val == 1)
		cs_num = 0x1;
	else
		cs_num = 0x3;

	top_DDR_MC_init(DDRC_BASE, fp);

	top_DDR_phy_init(DDRC_BASE, fp);

	REG32(DFI_PHY_USER_COMMAND_0) = 0x13000001;	//MCK6 DFI phy user cmd, set init_start
	read_data = REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3fc);		// get init_complete
	LogMsg(0, "wait PHY INIT \n");
	while ((read_data & 0x80000000) != 0x80000000) {
		read_data = REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3fc);
	}
	LogMsg(0, "PHY INIT done \n");

	LogMsg(0, "pinmux [0x%08x]=0x%08x \n", (DPHY0_BASE + 0x10018), REG32(DPHY0_BASE + 0x10018));
	LogMsg(0, "rdo [0x%08x]=0x%08x \n", (DPHY0_BASE + 0x3034), REG32(DPHY0_BASE + 0x3034));
	LogMsg(0, "debug_ctrl_0 [0x%08x]=0x%08x \n", (DPHY0_BASE + 0x10030), REG32(DPHY0_BASE + 0x10030));

	REG32(DFI_PHY_USER_COMMAND_0) = 0x13000100;	//MCK6 DFI phy user cmd, clear init_start



	/*start init DRAM*/
	REG32(DDRC_BASE + 0x20) = (0x10000001 | (cs_num << 24));//USER_COMMAND_0

	read_data = REG32(DDRC_BASE + 0x8);//DRAM_STATUS

	LogMsg(0, "wait DRAM INIT \n");
	while ((read_data & 0x00000011) != 0x00011) {
		read_data = REG32(DDRC_BASE + 0x8);
	}
	LogMsg(0, "DRAM INIT done \n");



	/*Init MR register*/
	REG32(DDRC_BASE + 0x24) = (0x10020001 | (cs_num << 24)); //Init MR1, read-preamble, write-preamble, read-postamble, nWR
	REG32(DDRC_BASE + 0x24) = (0x10020002 | (cs_num << 24)); //Init MR2, WL, RL
	REG32(DDRC_BASE + 0x24) = (0x1002000d | (cs_num << 24)); //Init MR13, fsp_op, fsp_wr, vrcg
	REG32(DDRC_BASE + 0x24) = (0x10020003 | (cs_num << 24)); //Init MR3, pull-down drive strength, pull-calibration point, write-post amble.
	REG32(DDRC_BASE + 0x24) = (0x10020016 | (cs_num << 24)); //Init MR22, update SOC ODT
								 //LogMsg(0,"ZQ calibration .....\n");
	/*ZQ calibration start and latch*/

	REG32(DDRC_BASE + 0x20) = 0x11002000;//ZQ calibration Start command ,USER_COMMAND_0
	REG32(DDRC_BASE + 0x20) = 0x11001000;//ZQ calibration Latch command
	if (cs_val != 1) {
		REG32(DDRC_BASE + 0x20) = 0x12002000;//ZQ calibration Start command ,USER_COMMAND_0
		REG32(DDRC_BASE + 0x20) = 0x12001000;//ZQ calibration Latch command
	}

	REG32(DDRC_BASE + 0x24) = (0x1002000C | (cs_num << 24)); //Init MR12, update Vref(ca), Vref(ca) range
	REG32(DDRC_BASE + 0x24) = (0x1002000E | (cs_num << 24)); //Init MR14, update Vref(dq), Vref(dq) range
	REG32(DDRC_BASE + 0x24) = (0x1002000B | (cs_num << 24)); //Init MR11, update CA ODT/DQ ODT
	REG32(DDRC_BASE + 0x24) = (0x10020017 | (cs_num << 24)); //Init MR23, update DQS internal timer runtime
	LogMsg(0, "DRAM Mode register Init done.....\n");

	//top_Phy_reg_dump(DDRC_BASE,0);

	return;
}

void adjust_timing(u32 DDRC_BASE)
{
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = 0xF0800400; //DRAM_Config_2
	REG32(DDRC_BASE+MC_CH0_BASE+0x0110)= 0x40440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0114)= 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x01b0) = 0x221D0C1D;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x01fc) = 0x000C005E;   //Refresh timing 1, tREFI=3.9us	,fclk=24mhz , tREFIpb=0.488us


	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = 0xA0800400;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0100) = 0x00000C1C;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0110)= 0x40440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0114)= 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x01fc) = 0x000C005E;   //Refresh timing 1, tREFI=3.9us	,fclk=24mhz , tREFIpb=0.488us

#if defined(NEW_FEATURE)
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x03e4) = 0x13000802;   //MCK6 DFI phy ctrl register 1  write leveling
#else
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x03e4) = 0x13000A00;   //MCK6 DFI phy ctrl register 1  write leveling
#endif


	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = 0x50800400;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0110) = 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0114) = 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x01fc) = 0x000C005E;   //Refresh timing 1, tREFI=3.9us	,fclk=24mhz , tREFIpb=0.488us


	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = 0x00800400;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0110) = 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0114) = 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x01fc) = 0x000C005E;

}

void adjust_mapping(u32 DDRC_BASE)
{
	REG32(DDRC_BASE + MC_CH0_BASE) = 0xf0001;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x4) = 0x0;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x8) = 0x800f0001;
	REG32(DDRC_BASE + MC_CH0_BASE + 0xc) = 0x0;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x20) = 0x05030632;//8 bank, 17 row, 10 column
	REG32(DDRC_BASE + MC_CH0_BASE + 0x24) = 0x05030632;//8 bank, 17 row, 10 column

}

__maybe_unused static int printf_no_output(const char *fmt, ...)
{
        return 0;
}

static void top_training_fp_all(u32 ddr_base, u32 cs_num, u32 boot_pp)
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
	training = (void (*)(void * param))lpddr4_training_img;
	training(to_traning_param);
}

void lpddr4_silicon_init(u32 ddr_base, u32 data_rate)
{
	unsigned fp=0;
	unsigned cs_num=2;

	top_Common_config();

	top_DDR_MC_Phy_Device_Init(ddr_base,cs_num,0);

	if (ddr_get_density() == 4096) {
		adjust_mapping(ddr_base);
	}
	LogMsg(0,"ddr density: %u \n", ddr_get_density());

	LogMsg(0,"init table start \n");
	ddr_dfc_table_init(0xF0000000);
	LogMsg(0,"init table done \n");
	init_table_mc_a0(0xF0000000);

	top_training_fp_all(ddr_base,cs_num,0);

	fp=1;
	ddr_dfc(fp);
	top_training_fp_all(ddr_base,cs_num,fp);

	fp=2;
	ddr_dfc(fp);
	top_training_fp_all(ddr_base,cs_num,fp);

	/* change dram frequency */
	switch(data_rate) {
	case 1600:
		ddr_dfc(1);
		break;

	case 2400:
		ddr_dfc(2);
		break;

	case 1200:
	default:
		data_rate = 1200;
		ddr_dfc(1);
		break;
	}

	printf("change ddr data rate to %u ..."	\
		"		[succeed]\n", data_rate);
	return;
}

#endif

