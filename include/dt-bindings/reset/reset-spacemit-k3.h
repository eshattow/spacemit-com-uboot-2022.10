/* SPDX-License-Identifier: GPL-2.0-only OR */
/*
 * Spacemit K3 reset controller driver
 * Copyright (c) 2025, spacemit Corporation.
 */

#ifndef __DT_BINDINGS_RESET_SAPCEMIT_K3_H__
#define __DT_BINDINGS_RESET_SAPCEMIT_K3_H__
//APBC
#define RESET_UART0              0
#define RESET_UART2              1
#define RESET_UART3              2
#define RESET_UART4              3
#define RESET_UART5              4
#define RESET_UART6              5
#define RESET_UART7              6
#define RESET_UART8              7
#define RESET_UART9              8
#define RESET_GPIO               9
#define RESET_PWM0               10
#define RESET_PWM1               11
#define RESET_PWM2               12
#define RESET_PWM3               13
#define RESET_PWM4               14
#define RESET_PWM5               15
#define RESET_PWM6               16
#define RESET_PWM7               17
#define RESET_PWM8               18
#define RESET_PWM9               19
#define RESET_PWM10              20
#define RESET_PWM11              21
#define RESET_PWM12              22
#define RESET_PWM13              23
#define RESET_PWM14              24
#define RESET_PWM15              25
#define RESET_PWM16              26
#define RESET_PWM17              27
#define RESET_PWM18              28
#define RESET_PWM19              29
#define RESET_SSP3               30
#define RESET_RTC                31
#define RESET_TWSI0              32
#define RESET_TWSI1              33
#define RESET_TWSI2              34
#define RESET_TWSI4              35
#define RESET_TWSI5              36
#define RESET_TWSI6              37
#define RESET_TWSI8              38
#define RESET_TIMERS0            39
#define RESET_TIMERS1            40
#define RESET_TIMERS2            41
#define RESET_TIMERS3            42
#define RESET_TIMERS4            43
#define RESET_TIMERS5            44
#define RESET_TIMERS6            45
#define RESET_TIMERS7            46
#define RESET_AIB                47
#define RESET_ONEWIRE            48
#define RESET_SSPA0              49
#define RESET_SSPA1              50
#define RESET_DRO                51
#define RESET_IR                 52
#define RESET_TSEN               53
#define RESET_IPC_AP2AUD         54
#define RESET_CAN0               55
#define RESET_CAN1               56
#define RESET_CAN2               57
#define RESET_CAN3               58
#define RESET_CAN4               59
//MPMU
#define RESET_WDT                60
#define RESET_RIPC               61
//APMU
#define RESET_CSI                62
#define RESET_CCIC2_PHY          63
#define RESET_CCIC3_PHY          64
#define RESET_ISP                65
#define RESET_ISP_AHB            66
#define RESET_ISP_CIBUS          67
#define RESET_DSI_ESC            68
#define RESET_LCD                69
#define RESET_V2D                70
#define RESET_LCD_SPI_HBUS       71
#define RESET_LCD_SPI_BUS        72
#define RESET_LCD_MCLK           73
#define RESET_LCD_DSCCLK         74
#define RESET_SC2_HCLK           75
#define RESET_CCIC_4X            76
#define RESET_CCIC1_PHY          77
#define RESET_SDH_AXI            78
#define RESET_SDH0               79
#define RESET_SDH1               80
#define RESET_USB2               81
#define RESET_USB3_PORTA         82
#define RESET_USB3_PORTB         83
#define RESET_USB3_PORTC         84
#define RESET_USB3_PORTD         85
#define RESET_QSPI               86
#define RESET_QSPI_BUS           87
#define RESET_DMA                88
#define RESET_AES_WTM            89
#define RESET_MCB_DCLK           90
#define RESET_MCB_ACLK           91
#define RESET_VPU                92
#define RESET_DTC                93
#define RESET_GPU                94
#define RESET_ALZO               95
#define RESET_SDH2               96
#define RESET_MC                 97
#define RESET_CPU0_POP           98
#define RESET_CPU0_SW            99
#define RESET_CPU1_POP           100
#define RESET_CPU1_SW            101
#define RESET_CPU2_POP           102
#define RESET_CPU2_SW            103
#define RESET_CPU3_POP           104
#define RESET_CPU3_SW            105
#define RESET_C0_MPSUB_SW        106
#define RESET_CPU4_POP           107
#define RESET_CPU4_SW            108
#define RESET_CPU5_POP           109
#define RESET_CPU5_SW            110
#define RESET_CPU6_POP           111
#define RESET_CPU6_SW            112
#define RESET_CPU7_POP           113
#define RESET_CPU7_SW            114
#define RESET_C1_MPSUB_SW        115
#define RESET_MPSUB_DBG          116
#define RESET_AUDIO_SYS          117
#define RESET_DSI4LN2_ESCCLK     118
#define RESET_DSI4LN2_LCD_SW     119
#define RESET_UFS_ACLK           120
#define RESET_UFS_EMU_SYMBCLK    121
#define RESET_DSI4LN2_LCD_MCLK   122
#define RESET_DSI4LN2_LCD_DSCCLK 123
#define RESET_DSI4LN2_DPU_ACLK   124
#define RESET_DPU_ACLK           125
#define RESET_EDP0               126
#define RESET_EDP1               127
#define RESET_PCIE_PORTA         128
#define RESET_PCIE_PORTB         129
#define RESET_PCIE_PORTC         130
#define RESET_PCIE_PORTD         131
#define RESET_PCIE_PORTE         132
#define RESET_EMAC0              133
#define RESET_EMAC1              134
#define RESET_EMAC2              135
#define RESET_ESPI               136
//DCIU
#define RESET_HDMA               137
#define RESET_DMA350             138
#define RESET_DMA350_0           139
#define RESET_DMA350_1           140
#define RESET_AXIDMA0            141
#define RESET_AXIDMA1            142
#define RESET_AXIDMA2            143
#define RESET_AXIDMA3            144
#define RESET_AXIDMA4            145
#define RESET_AXIDMA5            146
#define RESET_AXIDMA6            147
#define RESET_AXIDMA7            148
//APBC2
#define RESET_SEC_UART1          149
#define RESET_SEC_SSP2           150
#define RESET_SEC_TWSI3          151
#define RESET_SEC_RTC            152
#define RESET_SEC_TIMERS         153
#define RESET_SEC_JTAG           154
#define RESET_SEC_GPIO           155
//RCPU
#define RESET_RCPU5_RT24_CORE0   156
#define RESET_RCPU5_RT24_CORE1   157

#define RESET_NUMBER             158

#endif /* __DT_BINDINGS_RESET_SAPCEMIT_K3_H__ */
