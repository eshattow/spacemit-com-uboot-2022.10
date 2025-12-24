// SPDX-License-Identifier: GPL-2.0+
/*
 * Spacemit PCIe phy driver
 *
 * Copyright (c) 2025, spacemit Corporation.
 *
 */
#include <stdio.h>
#include <linux/delay.h>

//=======================================================================================
#define PU_COMBO_MGMT_BASE 0x81400000
#define PHY0_REG_BASE  (PU_COMBO_MGMT_BASE+0x0900000)
#define PHY1_REG_BASE  (PU_COMBO_MGMT_BASE+0x0A00000)
#define PHY2_REG_BASE  (PU_COMBO_MGMT_BASE+0x0B00000)
#define PHY3_REG_BASE  (PU_COMBO_MGMT_BASE+0x0C00000)
#define PHY4_REG_BASE  (PU_COMBO_MGMT_BASE+0x0D00000)
#define PHY5_REG_BASE  (PU_COMBO_MGMT_BASE+0x0E00000)
#define PHY0_SRAM_BASE (PU_COMBO_MGMT_BASE+0x0F00000)
#define PHY1_SRAM_BASE (PU_COMBO_MGMT_BASE+0x1000000)
#define PHY2_SRAM_BASE (PU_COMBO_MGMT_BASE+0x1100000)
#define PHY3_SRAM_BASE (PU_COMBO_MGMT_BASE+0x1200000)
#define PHY4_SRAM_BASE (PU_COMBO_MGMT_BASE+0x1300000)
#define PHY5_SRAM_BASE (PU_COMBO_MGMT_BASE+0x1400000)
#define PCIE_MGMT_BASE (PU_COMBO_MGMT_BASE+0x1A00000)

#define IOMMU_REG_BASE 0xC0F00000
#define SQU_BASE    0xc0800000

#define PCIE_REF_CLK_OUTPUT
#define STATIC_BF
#define PCIE_PORTA
#define PORTA_X4

#define PMUA_REG_BASE            0xD4282800
#ifdef PCIE_PORTA

#define PCIE_DBI_SLV_BASE        0x80000000
#define PCIE_ELBI_REG_BASE       (PCIE_DBI_SLV_BASE+0x00200000)
#define PCIE_DMA_REG_BASE        (PCIE_DBI_SLV_BASE+0x00380000)
#define PCIE_ATU_REG_BASE        (PCIE_DBI_SLV_BASE+0x00300000)
#define PCIE_MSIX_TAB_REG_BASE   (PCIE_DBI_SLV_BASE+0x00180000)
#define PCIE_MSIX_PDA_REG_BASE   (PCIE_DBI_SLV_BASE+0x00280000)
#define PCIE_DAT_SLV_BASE        0x1100000000
#define PCIE_APP_REG_BASE        (PU_COMBO_MGMT_BASE+0x01500000)
#define PCIE_MSTR_DDR_BASE       0x100100000

#define PCIE_CLK_RES_CTRL_ADDR   (PMUA_REG_BASE+0x1F0)
#define PCIE_LOGIC_CTRL_ADDR     (PMUA_REG_BASE+0x1F4)

#define ICU_PCIE_WAKEUP 203
#define ICU_PCIE_INT    125

//=======================================================================================
#elif defined(PCIE_PORTB)
#define PCIE_DBI_SLV_BASE   0x80400000
//#define PCIE_ELBI_REG_BASE  (PCIE_DBI_SLV_BASE+0x00200000)
#define PCIE_DMA_REG_BASE   (PCIE_DBI_SLV_BASE+0x00380000)
#define PCIE_ATU_REG_BASE   (PCIE_DBI_SLV_BASE+0x00300000)
#define PCIE_DAT_SLV_BASE   0x1180000000
#define PCIE_APP_REG_BASE   (PU_COMBO_MGMT_BASE+0x01800000)
#define PCIE_MSTR_DDR_BASE  0x100100000

#define PCIE_CLK_RES_CTRL_ADDR   (PMUA_REG_BASE+0x1D0)
#define PCIE_LOGIC_CTRL_ADDR     (PMUA_REG_BASE+0x1D4)

#define ICU_PCIE_WAKEUP 204
#define ICU_PCIE_INT    126

//=======================================================================================
#elif defined(PCIE_PORTC)
#define PCIE_DBI_SLV_BASE   0x80800000
//#define PCIE_ELBI_REG_BASE  (PCIE_DBI_SLV_BASE+0x00200000)
#define PCIE_DMA_REG_BASE   (PCIE_DBI_SLV_BASE+0x00380000)
#define PCIE_ATU_REG_BASE   (PCIE_DBI_SLV_BASE+0x00300000)
#define PCIE_DAT_SLV_BASE   0x1200000000
#define PCIE_APP_REG_BASE   (PU_COMBO_MGMT_BASE+0x01900000)
#define PCIE_MSTR_DDR_BASE  0x100100000

#define PCIE_CLK_RES_CTRL_ADDR   (PMUA_REG_BASE+0x1C8)
#define PCIE_LOGIC_CTRL_ADDR     (PMUA_REG_BASE+0x1CC)

#define ICU_PCIE_WAKEUP 205
#define ICU_PCIE_INT    127

//=======================================================================================
#elif defined(PCIE_PORTD)

#define PCIE_DBI_SLV_BASE      0x80C00000   //pcie_usb_combo mgmt base + 0
//#define PCIE_ELBI_REG_BASE  (PCIE_DBI_SLV_BASE+0x00200000) //RC only
#define PCIE_DMA_REG_BASE     (PCIE_DBI_SLV_BASE+0x00380000)
#define PCIE_ATU_REG_BASE     (PCIE_DBI_SLV_BASE+0x00300000)
#define PCIE_DAT_SLV_BASE      0x1280000000
#define PCIE_APP_REG_BASE     (PU_COMBO_MGMT_BASE+0x01600000)
#define PCIE_MSTR_DDR_BASE     0x100100000

#define PCIE_CLK_RES_CTRL_ADDR   (PMUA_REG_BASE+0x1E0)
#define PCIE_LOGIC_CTRL_ADDR     (PMUA_REG_BASE+0x1E4)

#define ICU_PCIE_WAKEUP 206
#define ICU_PCIE_INT    129


//=======================================================================================
#elif defined(PCIE_PORTE)

#define PCIE_DBI_SLV_BASE      0x81000000  //pcie_usb_combo mgmt base + 4M
//#define PCIE_ELBI_REG_BASE  (PCIE_DBI_SLV_BASE+0x00200000) //RC only
#define PCIE_DMA_REG_BASE     (PCIE_DBI_SLV_BASE+0x00380000)
#define PCIE_ATU_REG_BASE     (PCIE_DBI_SLV_BASE+0x00300000)
#define PCIE_DAT_SLV_BASE      0x12C0000000
#define PCIE_APP_REG_BASE     (PU_COMBO_MGMT_BASE+0x1700000)
#define PCIE_MSTR_DDR_BASE    0x100100000

#define PCIE_CLK_RES_CTRL_ADDR   (PMUA_REG_BASE+0x1E8)
#define PCIE_LOGIC_CTRL_ADDR     (PMUA_REG_BASE+0x1EC)

#define ICU_PCIE_WAKEUP 207
#define ICU_PCIE_INT    130

#endif

#define REG32(x)	(*((volatile uint32_t *)((uintptr_t)(x))))

// use for ca7 porting from finch
#define PCIe_ISR_UTILS(int_num, isr_func_handler)           \
{                                                           \
   printf("========= PCIe_ISR_UTILS begin =========\n");   \
   prepare_ap_int(int_num, isr_func_handler, 0);            \
   printf("========= PCIe_ISR_UTILS ended =========\n");   \
}

//#define DISABLE_EN_SAMPLE_DATA_AFTER_CDR_LOCKED

void hsio_rcal_ovrd_start(void)
{
    int rd_data;

    REG32(0xD4090000+0x178) &= ~(1<<17); //pu_cal disable

    rd_data=REG32(0xD4090000+0x17c);
    printf("0xD4090000+0x17c: %x\n\n", rd_data);

    REG32(0xD4090000+0x17c)&= ~(0xff<<20); // r_cal_ovrd_ntrim_val, r_cal_ovrd_ptrim_val clear
    REG32(0xD4090000+0x17c)|= 0x9<<20; // r_cal_ovrd_ntrim_val, r_cal_ovrd_ptrim_val
    REG32(0xD4090000+0x17c)|= 0x9<<24; // r_cal_ovrd_ntrim_val, r_cal_ovrd_ptrim_val
    rd_data=REG32(0xD4090000+0x17c);
    printf("0xD4090000+0x17c: %x\n", rd_data);
}

void hsio_rcal_ovrd_check(unsigned int phy_reg_base_addr)
{
    int rd_data;

    REG32(phy_reg_base_addr+0x14c) &= ~(1<<9); //clear phy latch R calibration
    REG32(0xD4090000+0x17c)|= 3<<28; //ovrd_ntrim_en, ovrd_ptrim_en
    REG32(0xD4090000+0x17c)|= (1<<30); //ovrd_stable_val=1
    REG32(0xD4090000+0x17c)|= 1<<31; //ovrd_stable_en

    do{
        rd_data=REG32(phy_reg_base_addr+0x150);
    }while(((rd_data>>8) & 0x1) == 0);  //wait for r calibration status done stable

    REG32(phy_reg_base_addr+0x14c) |= 1<<9; //phy latch R calibration

    rd_data=REG32(phy_reg_base_addr+0x150);
    if(((rd_data>>4 & 0xf)== 0x9) && (rd_data&0xf) == 0x9)
        printf("R calibration value top ovrd success\n");
	else
        printf("R calibration value top ovrd Error\n");
}

void init_x1_phy(unsigned int phy_reg_base_addr, unsigned int clk_res_ctrl_addr)
{
    REG32(clk_res_ctrl_addr) &= 0xbfffffff;//clear hold phy reset

    //printf("Now int Rterm...");
    //porta_rterm();
    //rterm_force();
    printf("Now int init_x1_puphy...\n");
    //disable refclk mode
    //REG32(phy_reg_base_addr+(0x14<<2)) = 0x00006504; //pcie3x2_reg0[1:0]=01 refclk buffer mode selection: driver mode, 10 receiver mode(default), 00/11 disable //FIXME

#ifndef PCIE_100M_REF_CLK
    REG32(phy_reg_base_addr+(0x16<<2)) &= 0xffff0fff;
    REG32(phy_reg_base_addr+(0x16<<2)) |= 0x00002000; // select 24Mhz refclock input pll_reg1[15:13]=2
    REG32(phy_reg_base_addr+(0x17<<2)) &= ~(0x1<<21);//pll_reg7[5], disable select refclk_100_n/p 100Mhz input
    REG32(phy_reg_base_addr+(0x14<<2)) &= ~(0x3); // refclk_mode=2'b00 to disable all
#ifdef PCIE_REF_CLK_OUTPUT
    REG32(phy_reg_base_addr+(0x17<<2)) |= (0x1<<20);//pll_reg7[4] of lane0, enable refclk_100_n/p 100Mhz output
    REG32(phy_reg_base_addr+(0x14<<2)) = 0x00006505; //pcie3x2_reg0[1:0]=01 refclk buffer mode selection: driver mode
#endif
#endif

    //new added, dig_reg_afe_adpt_rst = 1
    REG32(phy_reg_base_addr+(0x50<<2)) |= 1<< 4;

    /* Rx_reg6[6] = 1 */
    REG32(phy_reg_base_addr+(0x19<<2)) |= 1<< 22;

#ifdef CBOOST_OFF
    REG32(phy_reg_base_addr+(0x50<<2)) |= 1<< 1;
    REG32(phy_reg_base_addr+(0x50<<2)) |= 1<< 4;
    REG32(phy_reg_base_addr+(0x53<<2)) |= 1<< 28;
    REG32(phy_reg_base_addr+(0x53<<2)) |= 1<< 29;
    REG32(phy_reg_base_addr+(0x18<<2)) |= 1<< 20;
    REG32(phy_reg_base_addr+(0x18<<2)) &= ~(1<< 27);
#endif

#ifdef DISABLE_EN_SAMPLE_DATA_AFTER_CDR_LOCKED
    REG32(phy_reg_base_addr+(0x01<<2)) &= ~(0x1<<6);//disable en_sample_data_after_cdr_locked
#endif
    REG32(phy_reg_base_addr+(0x16<<2)) &= 0xf0ffffff;//pll_reg1 of lane0, disable ssc pll_reg4[3:0]=4'h0
#ifdef R_CAL_CHECK
    hsio_rcal_start();
    phy_rcal_check_done(phy_reg_base_addr);
    phy_rcal_ovrd_check(phy_reg_base_addr);
    hsio_rcal_ovrd_start();
    hsio_rcal_ovrd_check(phy_reg_base_addr);
#endif

    REG32(phy_reg_base_addr+0x4) &= ~(0x1<<6);
    printf("cdr fix bypass\n");
    REG32(phy_reg_base_addr+0xC) |= (0x1<<2);
    printf("dynamic lock\n");

#ifdef FORCE_RCV_GOOD
    REG32(phy_reg_base_addr+(0x06<<2)) = 0x00000400;//force rcv done
    printf("force rcv good\n");
#endif

#ifdef TX_AMP
    REG32(phy_reg_base_addr+0xb4) &= ~(0x3<<16|0x3<<19);
    REG32(phy_reg_base_addr+0xb4) |= (0x2<<16|0x1<<18|0x2<<19|0x1<<21);

    REG32(phy_reg_base_addr+0xb4) &= ~(0x3<<22);
    REG32(phy_reg_base_addr+0xb4) |= (0x3<<22|0x1<<24);
    printf("tx_amp: 0x%08x\n", REG32(phy_reg_base_addr+0xb4));
#endif

    REG32(phy_reg_base_addr+(0x02<<2)) = 0x00000978;//PU_ADDR_CLK_CFG of lane0, bit[11]cfg_sw_phy_init_done,bit[10:7] aux clk=24M
#ifdef PMA_CLK_GATING_CHECK
    REG32(phy_reg_base_addr+0x2*4) &= 0xffffff87;//clear bit3/4/5/6 to disable clk gate always enalbe
#endif

#ifdef PCIE_SSC_OPEN
    printf("Opening Puphy SSC setting...\n");
    REG32(clk_res_ctrl_addr) |= 0x1<<29; // sris mode
#ifdef PPM_3000
    REG32(phy_reg_base_addr+(0x16<<2)) |= 0x06000000;//pll_reg1 of lane0, disable ssc pll_reg4[3:0]=4'h0
#elif PPM_1500
    REG32(phy_reg_base_addr+(0x16<<2)) |= 0x03000000;//pll_reg1 of lane0, disable ssc pll_reg4[3:0]=4'h0
#else
    REG32(phy_reg_base_addr+(0x16<<2)) |= 0x0A000000;//pll_reg1 of lane0, disable ssc pll_reg4[3:0]=4'h0
#endif
#endif

#if CTRL_FAST_SIM
    printf("Controller Fast Sim Mode Set...\n");
    REG32(PCIE_DBI_SLV_BASE+0x710) |= 1<<7;//Fast mode set
    REG32(PCIE_DBI_SLV_BASE+0x718) &= (~(0x3<<29));// 0x0 (SF_1024): Scaling Factor is 1024 (1ms is 1us).
#endif

}


void init_x2_phy(unsigned int phy_reg_base_addr, unsigned int clk_res_ctrl_addr)
{
    unsigned i;
    uint32_t rd_data;
    (void)rd_data;

    REG32(clk_res_ctrl_addr) &= 0xbfffffff;//clear hold phy reset

    //printf("Now int Rterm...");
    //porta_rterm();
    //rterm_force();
    printf("Now int init_x2_puphy...\n");

#ifndef PCIE_100M_REF_CLK
    REG32(phy_reg_base_addr+(0x16<<2)) &= 0xffff0fff;
    REG32(phy_reg_base_addr+(0x16<<2)) |= 0x00002000; // select 24Mhz refclock input pll_reg2[7:4]=2
    REG32(phy_reg_base_addr+(0x17<<2)) &= ~(0x1<<21);//pll_reg7[5] of lane0, disable select refclk_100_n/p 100Mhz input
    for(i=0;i<2;i++) {
        REG32(phy_reg_base_addr+0x400*i+(0x14<<2)) &= ~(0x3); // refclk_mode=2'b00 to disable all
    }
#ifdef PCIE_REF_CLK_OUTPUT
    REG32(phy_reg_base_addr+(0x17<<2)) |= (0x1<<20);//pll_reg7[4] of lane0, enable refclk_100_n/p 100Mhz output
    REG32(phy_reg_base_addr+(0x14<<2)) = 0x00006505; //pcie3x2_reg0[1:0]=01 refclk buffer mode selection: driver mode
#endif
#endif
    REG32(phy_reg_base_addr+(0x16<<2)) &= 0xf0ffffff;//pll_reg1 of lane0, disable ssc pll_reg4[3:0]=4'h0

#ifdef ASIC_SIMULATION_SPEED_UP
    // for speed up sim PU_ADDR_RXEQ_TIME. Only used in simulation!!!
    rd_data = REG32(phy_reg_base_addr+0x400*i+(0x2d<<2));
    rd_data &= 0xFFFF0000;
    rd_data |= 200;
    for(i=0;i<2;i++) {
        REG32(phy_reg_base_addr+0x400*i+(0x2d<<2)) = rd_data; //for speed up sim PU_ADDR_RXEQ_TIME
    }
#endif

//#ifdef PCIE_SET_PHY_RX_BOOST
//    printf("set lane0-1 rxboost");
//    for(i=0;i<2;i++) {
//        rd_data = REG32(phy_reg_base_addr+0x400*i+0x60);
//        //lane 0-15 force csel value
//        rd_data &= ~(0xf<<16);
//        rd_data |= (0x4<<16);
//        // force csel enable
//        rd_data |= (0x1<<20);
//        REG32(phy_reg_base_addr+0x400*i+0x60) = rd_data;
//    }
//#endif
//
//#ifdef PCIE_SET_PHY_DYNAMIC_LOCK
//    printf("set lane0-1 dynamic lock");
//    for(i=0;i<2;i++) {
//        REG32(phy_reg_base_addr+0x400*i+0xc) |= 0x1<<2; // set cdet_cfg_dynamic_lock
//    }
//#endif
#ifdef R_CAL_CHECK
    hsio_rcal_start();
    for(i=0;i<2;i++) {
        phy_rcal_check_done(phy_reg_base_addr+0x400*i);
    }
    for(i=0;i<2;i++) {
        phy_rcal_ovrd_check(phy_reg_base_addr+0x400*i);
    }
    hsio_rcal_ovrd_start();
    for(i=0;i<2;i++) {
        hsio_rcal_ovrd_check(phy_reg_base_addr+0x400*i);
    }

#endif

    for(i=0;i<2;i++) {
        REG32(phy_reg_base_addr+0x400*i+(0x10<<2)) |= 0x1<<13; // set cfg_tx_send_dummy_data to be 1'b1 for disable dash data
        REG32(phy_reg_base_addr+0x400*i+(0x02<<2)) = 0xf<<3;   //clear bit3/4/5/6 to disable clk gate always enalbe
#ifdef DISABLE_EN_SAMPLE_DATA_AFTER_CDR_LOCKED
        REG32(phy_reg_base_addr+0x400*i+(0x01<<2)) &= ~(0x1<<6);//disable en_sample_data_after_cdr_locked
#endif
        //new added, dig_reg_afe_adpt_rst = 1
        REG32(phy_reg_base_addr+0x400*i+(0x50<<2)) |= 1<< 4;
        /* Rx_reg6[6] = 1 */
        REG32(phy_reg_base_addr+0x400*i+(0x19<<2)) |= 1<< 22;

#ifdef CBOOST_OFF
        REG32(phy_reg_base_addr+0x400*i+(0x50<<2)) |= 1<< 1;
        REG32(phy_reg_base_addr+0x400*i+(0x50<<2)) |= 1<< 4;
        REG32(phy_reg_base_addr+0x400*i+(0x53<<2)) |= 1<< 28;
        REG32(phy_reg_base_addr+0x400*i+(0x53<<2)) |= 1<< 29;
        REG32(phy_reg_base_addr+0x400*i+(0x18<<2)) |= 1<< 20;
        REG32(phy_reg_base_addr+0x400*i+(0x18<<2)) &= ~(1<< 27);
#endif
    }
#ifdef LANE0
    REG32(phy_reg_base_addr+(0x06<<2)) |= 0x1<<10;//force rcv done
    for(i=1;i<2;i++) {
        REG32(phy_reg_base_addr+0x400*i+(0x07<<2)) |= (1<<31); //cfg_force_rcv_bad
    }
#elif PORTA_X8_TO_X4   //for PORTA_X8_TO_X4_LANE_REVERSE case
#ifdef LANE_REVERSE
    for(i=0;i<2;i++) {
        REG32(phy_reg_base_addr+0x400*i+(0x07<<2)) |= (1<<31); //cfg_force_rcv_bad
    }
#endif
#else
#ifdef FORCE_RCV_GOOD
    printf("force rcv good\n");
    for(i=0;i<2;i++) {
        REG32(phy_reg_base_addr+0x400*i+(0x06<<2)) |= 0x1<<10;//force rcv done
    }
#endif

    for(i=0;i<2;i++) {
        REG32(phy_reg_base_addr+0x400*i+0x4) &= ~(0x1<<6);
        printf("cdr fix bypass\n");
        REG32(phy_reg_base_addr+0x400*i+0xC) |= (0x1<<2);
        printf("dynamic lock\n");
    }
#endif

#if 0
    for(i=0;i<2;i++) {
        rd_data = REG32(phy_reg_base_addr+0x400*i+0x6c);
        rd_data &= ~0xff;
        rd_data |= (0x0<<5|0x0<<3|0x2);
        REG32(phy_reg_base_addr+0x400*i+0x6c) = rd_data;
        rd_data = REG32(phy_reg_base_addr+0x400*i+0x6c);
        printf("lfps filter set@%08x: %08x\n", phy_reg_base_addr+0x400*i+0x6c, rd_data);
    }
#endif
#ifdef TX_AMP
    for(i=0;i<2;i++) {
        REG32(phy_reg_base_addr+0x400*i+0xb4) &= ~(0x3<<16|0x3<<19);
        REG32(phy_reg_base_addr+0x400*i+0xb4) |= (0x2<<16|0x1<<18|0x3<<19|0x1<<21);

        REG32(phy_reg_base_addr+0x400*i+0xb4) &= ~(0x3<<22);
        REG32(phy_reg_base_addr+0x400*i+0xb4) |= (0x3<<22|0x1<<24);
        printf("lane%u tx_amp: 0x%08x\n", i, REG32(phy_reg_base_addr+0x400*i+0xb4));
    }
#endif

    for(i=0;i<2;i++) {
        printf("lane%u 0x150: 0x%08x\n", i, REG32(phy_reg_base_addr+0x400*i+0x150));
    }

#ifdef AFE_ADAPT
    for(i=0;i<2;i++) {
        REG32(phy_reg_base_addr+0x400*i+0x60) |= (0x1<<20|0x1<<27);
        printf("afe adapt->lane%d @0x60: %x\n", i, REG32(phy_reg_base_addr+0x400*i+0x60));
    }

    for(i=0;i<2;i++) {
        //REG32(phy_reg_base_addr+0x400*i+0x140) &= ~(0x1<<4|0x1<<1);
        //REG32(phy_reg_base_addr+0x400*i+0x140) |= (0x1<<0|0x1<<3);
        //REG32(phy_reg_base_addr+0x400*i+0x144) &= ~(0xf<<1);
        //REG32(phy_reg_base_addr+0x400*i+0x144) |= (0x8<<1);
        //REG32(phy_reg_base_addr+0x400*i+0x14c) &= ~(0x3<<28);
        //REG32(phy_reg_base_addr+0x400*i+0x14c) |= (0x2<<28);
        //printf("adpat_rst release-> lane%d @0x140: %x\n", i, REG32(phy_reg_base_addr+0x400*i+0x140));
        printf("adpat_rst release-> lane%d @0x14c: %x\n", i, REG32(phy_reg_base_addr+0x400*i+0x14c));
    }

#endif

    //set init done
    for(i=0;i<2;i++) {
        REG32(phy_reg_base_addr+0x400*i+(0x02<<2)) |= (0x1<<11); //cfg_sw_phy_init_done
        REG32(phy_reg_base_addr+0x400*i+(0x02<<2)) &= ~(0xf<<7);
        REG32(phy_reg_base_addr+0x400*i+(0x02<<2)) |= (0x2<<7); //aux clk 24M
    }

#ifdef PCIE_SSC_OPEN
    printf("Opening Puphy SSC setting...\n");
    REG32(clk_res_ctrl_addr) |= 0x1<<29; // sris mode
#ifdef PPM_3000
    REG32(phy_reg_base_addr+(0x16<<2)) |= 0x06000000;//pll_reg1 of lane0, disable ssc pll_reg4[3:0]=4'h0
#elif PPM_1500
    REG32(phy_reg_base_addr+(0x16<<2)) |= 0x03000000;//pll_reg1 of lane0, disable ssc pll_reg4[3:0]=4'h0
#else
    REG32(phy_reg_base_addr+(0x16<<2)) |= 0x0A000000;//pll_reg1 of lane0, disable ssc pll_reg4[3:0]=4'h0
#endif
#endif


#if CTRL_FAST_SIM
    printf("Controller Fast Sim Mode Set...\n");
    REG32(PCIE_DBI_SLV_BASE+0x710) |= 1<<7;//Fast mode set
    REG32(PCIE_DBI_SLV_BASE+0x718) &= (~(0x3<<29));// 0x0 (SF_1024): Scaling Factor is 1024 (1ms is 1us).
    REG32(PCIE_DBI_SLV_BASE+0x718) |= 0x1<<29;// 0x1 (SF_512): Scaling Factor is 256 (1ms is 4us).
#endif

}

void pcie_ssc_open(unsigned int phy_reg_base_addr)
{
#ifdef PCIE_SSC_OPEN
    REG32(phy_reg_base_addr+0x8) |= 1<<23;
#endif
}

void wait_phy_pll_lock(unsigned int phy_reg_base_addr)
{
    int rd_data;
    // waiting pll lock
    printf("waiting pll lock...\n");
    do{
        rd_data = REG32(phy_reg_base_addr+0x8);
        printf("waiting pll lock status: 0x%x\n", rd_data);
    } while((rd_data&0x1)==0);
}

/*
 * 4:0	pcie_usb_combo_mode	RW	0x0	pcie_usb_combo_mode
 *
 * PHY Matrix Configuration
 *
 * [4] PCIe Controller A X8 : 1: Non X8, 0: PCIe A X8
 *
 * [3] PCIe Controller B X2 : 1: PCIe B X2, 0: PCIe A X4
 *
 * [2] PCIe Controller C X1 : 1: USB 0: PCIe
 *
 * [1] PCIe Controller C X1 : 1: USB 0: PCIe
 *
 * [0] PCIe Controller D X1 : 1: USB 0: PCIe
 */

#define K2_APB_SPARE31_BASE		0xD4090178
#define K2_APB_SPARE32_BASE		0xD409017C
void init_phy(void)
{
    static int phy_init_done = 0;

    if (phy_init_done != 0) {
        printf("phy init already\n");
        return;
    } else
        phy_init_done = 1;

    printf("Now int init_puphy...\n");
    REG32(K2_APB_SPARE31_BASE) |= (0x1<<17);
    printf("REG_APB_SPARE31_REG@%08x: 0x%08x\n", K2_APB_SPARE31_BASE, REG32(K2_APB_SPARE31_BASE));
    printf("REG_APB_SPARE32_REG@%08x: 0x%08x\n", K2_APB_SPARE32_BASE, REG32(K2_APB_SPARE32_BASE));

	//check bifurcation mode
#ifdef STATIC_BF
    int rd_data;
    REG32(PMUA_REG_BASE+0x1D8) |= 0x00000010;

    //config combosubsys phymatrix in test case through reg REG32(PMUA_REG_BASE+0x1D8) [4:0]
    //init phy for pu combo subsys
    rd_data=REG32(PMUA_REG_BASE+0x1D8);

    //PCIe E(phy5)
#ifdef PCIE_PORTE
    init_x1_phy(PHY5_REG_BASE, PMUA_REG_BASE+0x1E8);
    wait_phy_pll_lock(PHY5_REG_BASE);
    printf("Now finish puphy PHY5 init ...\n");
    pcie_ssc_open(PHY5_REG_BASE);
#endif

// PCIe D (phy4)
#ifdef PCIE_PORTD
    if( (rd_data&0x1) == 0){
        init_x1_phy(PHY4_REG_BASE, PMUA_REG_BASE+0x1E0);
        wait_phy_pll_lock(PHY4_REG_BASE);
        printf("Now finish puphy PHY4 init ...\n");
        pcie_ssc_open(PHY4_REG_BASE);
    }
#endif

#ifdef PCIE_PORTC
    if(((rd_data>>1)&0x3) == 0){
        //PCIe C x2(phy2,phy3)
        init_x1_phy(PHY2_REG_BASE, PMUA_REG_BASE+0x1C8);
        init_x1_phy(PHY3_REG_BASE, PMUA_REG_BASE+0x1C8);
        wait_phy_pll_lock(PHY2_REG_BASE);
        printf("Now finish puphy PHY2 init ...\n");
        wait_phy_pll_lock(PHY3_REG_BASE);
        printf("Now finish puphy PHY3 init ...\n");
        pcie_ssc_open(PHY2_REG_BASE);
    }

    if(((rd_data>>1)&0x3) == 1){
        // PCIe C X2(phy2)+ USB C(phy3)
        init_x1_phy(PHY2_REG_BASE, PMUA_REG_BASE+0x1C8);
        wait_phy_pll_lock(PHY2_REG_BASE);
        printf("Now finish puphy PHY2 init ...\n");
        pcie_ssc_open(PHY2_REG_BASE);
    }

	if(((rd_data>>1)&0x3) == 2) {
        // USB C(phy2) + PCIe C X2(phy3)
        init_x1_phy(PHY3_REG_BASE, PMUA_REG_BASE+0x1C8);
        wait_phy_pll_lock(PHY3_REG_BASE);
        printf("Now finish puphy PHY3 init ...\n");
        pcie_ssc_open(PHY3_REG_BASE);
    }
#endif

#ifdef PCIE_PORTB
    REG32(PMUA_REG_BASE+0x1D8) |= 0x00000008;
    rd_data=REG32(PMUA_REG_BASE+0x1D8);
    if(((rd_data>>3)&0x1) == 1) {
        init_x2_phy(PHY1_REG_BASE, PMUA_REG_BASE+0x1D0);
        wait_phy_pll_lock(PHY1_REG_BASE);
        printf("Now finish puphy PHY1 init ...\n");
        pcie_ssc_open(PHY1_REG_BASE);
    }
#endif

#ifdef PCIE_PORTA
    REG32(PMUA_REG_BASE+0x1D8) |= 0x00000008;  //default x2
#ifdef PORTA_X4
    REG32(PMUA_REG_BASE+0x1D8) &= ~(0x1<<3); //x4
#endif
    rd_data=REG32(PMUA_REG_BASE+0x1D8);
    if(((rd_data>>3)&0x1) == 1) {
        // PCIe A x2(phy0) + PCIe B x2(phy1)
        init_x2_phy(PHY0_REG_BASE, PMUA_REG_BASE+0x1F0);
        wait_phy_pll_lock(PHY0_REG_BASE);
        printf("Now finish puphy PHY0 init ...\n");
        pcie_ssc_open(PHY0_REG_BASE);
    } else {
        // PCIe A x4(phy0,phy1)
        init_x2_phy(PHY0_REG_BASE, PMUA_REG_BASE+0x1F0);
        init_x2_phy(PHY1_REG_BASE, PMUA_REG_BASE+0x1F0);
        wait_phy_pll_lock(PHY0_REG_BASE);
        printf("Now finish puphy PHY0 init ...\n");
        wait_phy_pll_lock(PHY1_REG_BASE);
        printf("Now finish puphy PHY1 init ...\n");
        pcie_ssc_open(PHY0_REG_BASE);
}
#endif

#else

#ifdef PCIE_PORTA
    init_x2_phy(PHY0_REG_BASE, PMUA_REG_BASE+0x1F0);
    init_x2_phy(PHY1_REG_BASE, PMUA_REG_BASE+0x1F0);
    init_x1_phy(PHY2_REG_BASE, PMUA_REG_BASE+0x1F0);
    init_x1_phy(PHY3_REG_BASE, PMUA_REG_BASE+0x1F0);
    init_x1_phy(PHY4_REG_BASE, PMUA_REG_BASE+0x1F0);
    init_x1_phy(PHY5_REG_BASE, PMUA_REG_BASE+0x1F0);
    wait_phy_pll_lock(PHY0_REG_BASE);
    printf("Now finish puphy PHY0 init ...\n");
    wait_phy_pll_lock(PHY1_REG_BASE);
    printf("Now finish puphy PHY1 init ...\n");
    wait_phy_pll_lock(PHY2_REG_BASE);
    printf("Now finish puphy PHY2 init ...");
    wait_phy_pll_lock(PHY3_REG_BASE);
    printf("Now finish puphy PHY3 init ...");
    wait_phy_pll_lock(PHY4_REG_BASE);
    printf("Now finish puphy PHY4 init ...");
    wait_phy_pll_lock(PHY5_REG_BASE);
    printf("Now finish puphy PHY5 init ...");
    pcie_ssc_open(PHY0_REG_BASE);
#endif
#endif
}
