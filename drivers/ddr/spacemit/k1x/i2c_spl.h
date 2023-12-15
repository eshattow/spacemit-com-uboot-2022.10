#ifndef __I2C_H__
#define __I2C_H__
#include <linux/types.h>


#define TWSI_ICR			(0x0000)	/* 32 bit       TWSI TWSI Control Register */
#define TWSI_ISR			(0x0004)	/* 32 bit       TWSI TWSI Status Register */
#define TWSI_ISAR			(0x0008)	/* 32 bit       TWSI TWSI Slave Address Register */
#define TWSI_IDBR			(0x000c)	/* 32 bit       TWSI TWSI Data Buffer Register */
#define TWSI_ILCR			(0x0010)	/* 32 bit       TWSI TWSI Load Count Register */
#define TWSI_IWCR			(0x0014)	/* 32 bit       TWSI TWSI Wait Count Register */
#define TWSI_RST_CYC		(0x0018)	/* 32 bit       TWSI The TWSI  bus reset cycle counter defines the tcycles of SCL during bus reset */
#define TWSI_IBMR			(0x001c)	/* 32 bit       TWSI TWSI Bus Monitor Register */
#define TWSI_WFIFO			(0x0020)	/* 32 bit       TWSI TWSI Write FIFO Register */
#define TWSI_WFIFO_WPTR		(0x0024)	/* 32 bit       TWSI TWSI Write FIFO Write Pointer Register */
#define TWSI_WFIFO_RPTR		(0x0028)	/* 32 bit       TWSI TWSI Write FIFO Read Pointer Register */
#define TWSI_RFIFO			(0x002c)	/* 32 bit       TWSI TWSI Read FIFO Register */
#define TWSI_RFIFO_WPTR		(0x0030)	/* 32 bit       TWSI TWSI Read FIFO Write Pointer Register */
#define TWSI_RFIFO_RPTR		(0x0034)	/* 32 bit       TWSI TWSI Read FIFO Read Pointer Register */

/*	TWSI_ICR		0x0000	TWSI TWSI Control Register */
#define TWSI_ICR_BUS_RESET			(BIT(11))	/*The twsi will do bus reset upon this bit set.this bit is self-cleared  */
#define TWSI_ICR_DMA_EN				(BIT(7))	/* DMA Enable for both TX and RX FIFOs */
#define TWSI_ICR_RXOV_IE			(BIT(31))	/* Receive FIFO overrun Interrupt Enable */
#define TWSI_ICR_RXF_IE				(BIT(30))	/* Receive FIFO full Interrupt Enable */
#define TWSI_ICR_TXE_IE				(BIT(28))	/* Transmit FIFO Empty Interrupt Enable */
#define TWSI_ICR_RXHF_IE			(BIT(29))	/* Receive FIFO Half Full Interrupt Enable */
#define TWSI_ICR_TXDONE_IE			(BIT(27))	/* Transaction Done Interrupt Enable */
#define TWSI_ICR_TXBEGIN			(BIT(4))	/* Transaction Begin */
#define TWSI_ICR_FIFOEN				(BIT(5))	/* FIFO mode */
#define TWSI_ICR_GPIOEN				(BIT(6))	/* GPIO mode Enable for SCL during HS mode */
#define TWSI_ICR_MSDE				(BIT(26))	/* Master Stop Detected Enable */
#define TWSI_ICR_MSDIE				(BIT(25))	/* Master Stop Detected Interrupt Enable */
#define TWSI_ICR_MODE_MSK			0x300	/* Bus Mode (Master operation) */
#define TWSI_ICR_MODE_BASE			8
#define TWSI_ICR_UR					(BIT(10))	/* Unit Reset */
#define TWSI_ICR_SADIE				(BIT(23))	/* Slave Address Detected Interrupt Enable */
#define TWSI_ICR_ALDIE				(BIT(18))	/* Arbitration Loss Detected Interrupt Enable */
#define TWSI_ICR_SSDIE				(BIT(24))	/* Slave Stop Detected Interrupt Enable */
#define TWSI_ICR_BEIE				(BIT(22))	/* Bus Error Interrupt Enable */
#define TWSI_ICR_DRFIE				(BIT(20))	/* DBR Receive Full Interrupt Enable */
#define TWSI_ICR_ITEIE				(BIT(19))	/* IDBR Transmit Empty Interrupt Enable */
#define TWSI_ICR_GCD				(BIT(21))	/* General Call Disable */
#define TWSI_ICR_IUE				(BIT(14))	/* TWSI Unit Enable */
#define TWSI_ICR_SCLE				(BIT(13))	/* SCL Enable */
#define TWSI_ICR_MA					(BIT(12))	/* Master Abort */
#define TWSI_ICR_TB					(BIT(3))	/* Transfer Byte */
#define TWSI_ICR_ACKNAK				(BIT(2))	/* The positive/negative acknowledge control bit */
#define TWSI_ICR_STOP				(BIT(1))	/* Stop */
#define TWSI_ICR_START				(BIT(0))	/* Start */

/*	TWSI_ISR		0x0004	TWSI TWSI Status Register */
#define TWSI_ISR_RXOV		(BIT(31))	/* Receive FIFO Overrun (FIFO mode) */
#define TWSI_ISR_RXF		(BIT(30))	/* Receive FIFO Full (FIFO mode) */
#define TWSI_ISR_TXE		(BIT(28))	/* Transmit FIFO Empty(FIFO mode) */
#define TWSI_ISR_RXHF		(BIT(29))	/* Receive FIFO Half Full  (FIFO mode) */
#define TWSI_ISR_TXDONE		(BIT(27))	/* Transaction Done (FIFO mode) */
#define TWSI_ISR_MSD		(BIT(26))	/* Master Stop Detected */
#define TWSI_ISR_EBB		(BIT(17))	/* Early Bus Busy */
#define TWSI_ISR_BED		(BIT(22))	/* Bus Error Detected */
#define TWSI_ISR_SAD		(BIT(23))	/* Slave Address Detected */
#define TWSI_ISR_GCAD		(BIT(21))	/* General Call Address Detected */
#define TWSI_ISR_IRF		(BIT(20))	/* IDBR Receive Full */
#define TWSI_ISR_ITE		(BIT(19))	/* IDBR Transmit Empty */
#define TWSI_ISR_ALD		(BIT(18))	/* Arbitration Loss Detected */
#define TWSI_ISR_SSD		(BIT(24))	/* Slave Stop Detected */
#define TWSI_ISR_IBB		(BIT(16))	/* TWSI Bus Busy */
#define TWSI_ISR_UB			(BIT(15))	/* Unit Busy */
#define TWSI_ISR_ACKNAK		(BIT(14))	/* ACK/NACK Status */
#define TWSI_ISR_RWM		(BIT(13))	/* Read/write Mode */

/*	TWSI_IBMR		0x001c TWSI TWSI Bus Monitor Register */
#define TWSI_IBMR_SDA		(BIT(0))	/* SDA line level */
#define TWSI_IBMR_SCL		(BIT(1))	/* SCL line level */

typedef enum {
	STANDARD_MODE = 0,	/*100Kbps */
	FAST_MODE = 1,		/*400Kbps */
	HS_MODE = 2,		/*3.4 Mbps slave/3.3 Mbps master,standard mode when not doing a high speed transfer */
	HS_MODE_FAST = 3,	/*3.4 Mbps slave/3.3 Mbps master,fast mode when not doing a high speed transfer */

} I2C_FAST_MODE;

typedef enum {
	I2C_FUNCLK_33MHz = 0,	/*up to 3.4M bps for HS */
	I2C_FUNCLK_52MHz = 1,
	I2C_FUNCLK_62P4MHz = 2,	/*up to 1.8M bps for HS */
} I2C_FUNCTION_CLK;

#define I2C_INIT	(TWSI_ICR_FIFOEN|TWSI_ICR_GPIOEN|TWSI_ICR_IUE|TWSI_ICR_SCLE)

#define I2C_INT_ALL		(TWSI_ISR_RXOV|TWSI_ISR_RXF|TWSI_ISR_RXHF|TWSI_ISR_TXE|TWSI_ISR_TXDONE|	\
						TWSI_ISR_MSD|TWSI_ISR_SSD|TWSI_ISR_SAD|TWSI_ISR_BED|TWSI_ISR_GCAD|		\
						TWSI_ISR_IRF|TWSI_ISR_ITE|TWSI_ISR_ALD)

#define START_BYTE_CNTROL	((TWSI_ICR_TB|TWSI_ICR_START)<<8)	//0X900
#define TB_CNTROL			((TWSI_ICR_TB)<<8)	//0x800
#define END_BYTE_CNTROL		((TWSI_ICR_TB|TWSI_ICR_STOP)<<8)	//0Xa00
#define RX_END_BYTE_CNTROL		((TWSI_ICR_TB|TWSI_ICR_ACKNAK|TWSI_ICR_STOP)<<8)	//0Xa00

#define TX_FIFO_DEPTH			8
#define RX_FIFO_DEPTH			8

#define I2C_SLAVE_WRITE(slv)        ( (slv) & (~0x1) )	/* Master is writing to the slave */
#define I2C_SLAVE_READ(slv)         ( (slv) | 0x00000001 )	/* Master is reading from the slave */

#define PMIC_88PM802_ADDRESS 	0x30

#define ONKEY_WAKEUP			(1 << 0)
#define CHG_WAKEUP			(1 << 1)
#define EXTON_WAKEUP			(1 << 2)
#define SMPL_WU_LOG			(1 << 3)
#define RTC_ALARM_WAKEUP		(1 << 4)
#define FAULT_WAKEUP			(1 << 5)
#define BAT_WAKEUP			(1 << 6)
#define WLCHG_WAKEUP			(1 << 7)
#define OVER_TEMP			(1 << 8)
#define UV_VANA5			(1 << 9)
#define SW_PDOWN			(1 << 10)
#define FL_ALARM			(1 << 11)
#define WD				(1 << 12)
#define LONG_ONKEY			(1 << 13)
#define OV_VSYS				(1 << 14)
#define RTC_RESET			(1 << 15)
#define HYB_DONE			(1 << 16)
#define UV_VBAT				(1 << 17)
#define HW_RESET2			(1 << 18)
#define PGOOD_PDOWN			(1 << 19)
#define LONKEY_RTC			(1 << 20)
#define HW_RESET1			(1 << 21)
#define BOOTUP_FROM_POWERDOWN	(OVER_TEMP | UV_VANA5 | SW_PDOWN | RTC_RESET | UV_VBAT | LONKEY_RTC)

#define POWER_UP_LOG			(0x17)
#define POWER_DOWN_LOG1		(0xE5)
#define POWER_DOWN_LOG2		(0xE6)

#define PMIC_88PM802_PWR_BASE		0x31

void pmic_reset(void);
uint8_t pmic_read(uint8_t i2c_bus, uint8_t addr, uint8_t reg);
uint8_t pmic_write(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint8_t reg_val);
uint8_t pmic_write_with_check(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint8_t reg_val, uint8_t check_val);

int pmic_read8(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint8_t *rx_val);
int pmic_write8(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint8_t reg_val);
int pmic_read16(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint16_t *rx_val);
int pmic_write16(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint16_t reg_val);
int pmic_read32(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint32_t *rx_val);
int pmic_write32(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint32_t reg_val);
int is_adb_reboot_download_mode(void);
uint32_t set_vcc_m1(uint16_t reg_value);

#endif /* __I2C_H__ */
