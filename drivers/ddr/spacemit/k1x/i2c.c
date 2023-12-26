#ifdef CONFIG_K1_X_BOARD_ASIC

#include "i2c_spl.h"
#include <linux/delay.h>
I2C_FAST_MODE i2c_fast_mode = FAST_MODE;

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

uint32_t twsi_base[] = {
	0xd4010000,	/* TWSI0 */
	0xd4011000,	/* TWSI1 */
	0xd4012000,	/* TWSI2 */
	0xf0614000,	/* TWSI3 */
	0xd4012800,	/* TWSI4 */
	0xd4013800,	/* TWSI5 */
	0xd4018800,	/* TWSI6 */
	0xd401d000,	/* TWSI7 */
	0xd401d800,	/* TWSI8 */

};

#define ASR_APBC_BASE		0xd4015000	/* APB Clock Unit */

#define REG_APBC_APBC_TWSI0_CLK_RST	(ASR_APBC_BASE + 0x2c)
#define REG_APBC_APBC_TWSI1_CLK_RST	(ASR_APBC_BASE + 0x30)
#define REG_APBC_APBC_TWSI2_CLK_RST	(ASR_APBC_BASE + 0x38)
#define REG_APBC_APBC_TWSI3_CLK_RST	(0xf0610000    + 0x08)
#define REG_APBC_APBC_TWSI4_CLK_RST	(ASR_APBC_BASE + 0x40)
#define REG_APBC_APBC_TWSI5_CLK_RST	(ASR_APBC_BASE + 0x4c)
#define REG_APBC_APBC_TWSI6_CLK_RST	(ASR_APBC_BASE + 0x60)
#define REG_APBC_APBC_TWSI7_CLK_RST	(ASR_APBC_BASE + 0x68)
#define REG_APBC_APBC_TWSI8_CLK_RST	(ASR_APBC_BASE + 0x20)


static inline void mmio_write_32(uintptr_t addr, uint32_t val)
{
	*(volatile uint32_t *)addr = val;
}
static inline uint32_t mmio_read_32(uintptr_t addr)
{
	return *(volatile uint32_t *)addr;
}

static uint32_t apbc_clk_reg[] = {
	REG_APBC_APBC_TWSI0_CLK_RST,
	REG_APBC_APBC_TWSI1_CLK_RST,
	REG_APBC_APBC_TWSI2_CLK_RST,
	REG_APBC_APBC_TWSI3_CLK_RST,
	REG_APBC_APBC_TWSI4_CLK_RST,
	REG_APBC_APBC_TWSI5_CLK_RST,
	REG_APBC_APBC_TWSI6_CLK_RST,
	REG_APBC_APBC_TWSI7_CLK_RST,
	REG_APBC_APBC_TWSI8_CLK_RST
};


extern void wdt_reset(void);

void i2c_fifo_clear_control(int port_idx)
{
	uint32_t port_base = twsi_base[port_idx];
	uint32_t ICRRegValue = 0;

	ICRRegValue = mmio_read_32(TWSI_ICR + port_base);
	ICRRegValue &=
	    ~(TWSI_ICR_TB | TWSI_ICR_ACKNAK | TWSI_ICR_STOP | TWSI_ICR_START);
	mmio_write_32(TWSI_ICR + port_base, ICRRegValue);
}

void i2c_fifo_master_abort(int port_idx)
{
	uint32_t port_base = twsi_base[port_idx];
	uint32_t ICRRegValue = 0;

	ICRRegValue = mmio_read_32(TWSI_ICR + port_base);
	ICRRegValue |= TWSI_ICR_MA;
	ICRRegValue &= ~TWSI_ICR_TB;
	mmio_write_32(TWSI_ICR + port_base, ICRRegValue);

	ICRRegValue &= ~TWSI_ICR_MA;
	mmio_write_32(TWSI_ICR + port_base, ICRRegValue);
}

void i2c_fifo_clear_status(int port_idx, uint32_t status)
{
	uint32_t port_base = twsi_base[port_idx];

	mmio_write_32(TWSI_ISR + port_base, status);
}

void i2c_clear_tx_rx_fifo(int port_idx)
{
	uint32_t port_base = twsi_base[port_idx];

	/*clear wr/rd ptr of TXFIFO and RXFIFO before enable FIFO */
	mmio_write_32(TWSI_WFIFO_WPTR + port_base, 0x0);
	mmio_write_32(TWSI_WFIFO_RPTR + port_base, 0x0);
	mmio_write_32(TWSI_RFIFO_WPTR + port_base, 0x0);
	mmio_write_32(TWSI_RFIFO_RPTR + port_base, 0x0);
}

void i2c_apbclk_init(int port_idx, I2C_FUNCTION_CLK clk)
{
	LogMsg(1, "i2c_apbclk_init: %x\n", port_idx);
	mmio_write_32(0xd4015020, (clk << 4) | 0x4);
	mmio_write_32(apbc_clk_reg[port_idx], (clk << 4) | 0x7);
	mmio_write_32(apbc_clk_reg[port_idx], (clk << 4) | 0x3);
	LogMsg(1, "i2c_apbclk_init: %x done\n", port_idx);
}

static void i2c_fifo_bus_reset(int port_idx)
{
	uint32_t port_base = twsi_base[port_idx];
	uint32_t clk_cnt = 0, bus_status = 0;

	/* if bus is locked, reset unit. 0:locked */
	bus_status = mmio_read_32(TWSI_IBMR + port_base);
	if(!(bus_status & TWSI_IBMR_SDA) || !(bus_status & TWSI_IBMR_SCL)) {
		/* controller reset */
		mmio_write_32(TWSI_ICR + port_base, TWSI_ICR_UR);
		udelay(5);
		mmio_write_32(TWSI_ICR + port_base, 0x0);

		/* set load counter register */
		mmio_write_32(TWSI_ILCR + port_base, 0x4024bb56);
		/* set wait counter register */
		mmio_write_32(TWSI_IWCR + port_base, 0x1a | (1 << 5) | (20 << 10));

		bus_status = mmio_read_32(TWSI_IBMR + port_base);
		if(!(bus_status & TWSI_IBMR_SCL))
			LogMsg(0, "i2c unit reset failed\n");
	}

	while(clk_cnt < 0x9) {
		/* check whether the SDA is still locked by slave */
		bus_status = mmio_read_32(TWSI_IBMR + port_base);
		if(bus_status & TWSI_IBMR_SDA)
			break;

		/* if still locked, send one clk to slave to request release */
		mmio_write_32(TWSI_RST_CYC + port_base, 0x1);
		mmio_write_32(TWSI_ICR + port_base, TWSI_ICR_BUS_RESET);
		udelay(20);
		clk_cnt++;
	}

	bus_status = mmio_read_32(TWSI_IBMR + port_base);
	if(clk_cnt >=9 && !(bus_status & TWSI_IBMR_SDA))
		LogMsg(1, "i2c bus reset reaches the max 9-clocks\n");
	else
		LogMsg(1, "i2c bus reset, send clk: %d\n", clk_cnt);
}

static void I2CResetUnit(int port_idx)
{
	uint32_t port_base = twsi_base[port_idx];

	mmio_write_32(TWSI_ICR + port_base, TWSI_ICR_UR);
	mmio_write_32(TWSI_ISR + port_base, I2C_INT_ALL);
	mmio_write_32(TWSI_ICR + port_base, 0);
	//mdelay(100);          /*Delay of 100 millisecond - enable the module to sync with the bus*/

}

void i2c_fifo_init(int port_idx, I2C_FAST_MODE fastMode)
{
	uint32_t port_base = twsi_base[port_idx];

	i2c_fast_mode = fastMode;
	i2c_apbclk_init(port_idx, I2C_FUNCLK_33MHz);
	i2c_clear_tx_rx_fifo(port_idx);
	I2CResetUnit(port_idx);
	i2c_fifo_bus_reset(port_idx);
	mdelay(5);
	mmio_write_32(TWSI_ILCR + port_base, 0x82c469f);
	mmio_write_32(TWSI_IWCR + port_base, 0x142a);
	mmio_write_32(TWSI_ICR + port_base,
		      I2C_INIT | (fastMode << TWSI_ICR_MODE_BASE));
	LogMsg(1, "init 0x%x,0x%x,0x%x\n", mmio_read_32(TWSI_ICR + port_base),
	       mmio_read_32(TWSI_ISR + port_base),
	       mmio_read_32(TWSI_IBMR + port_base));
}

int i2c_fifo_wait_status(int port_idx, uint32_t status)
{
	uint32_t port_base = twsi_base[port_idx];
	uint32_t i = 0;
	while (status != (status & (mmio_read_32(TWSI_ISR + port_base)))) {
		udelay(100);
		i++;
		if (TWSI_ISR_BED & (mmio_read_32(TWSI_ISR + port_base))) {
			LogMsg(0, "bus error detected!0x%x,0x%x\n",
			      mmio_read_32(TWSI_ICR + port_base),
			      mmio_read_32(TWSI_ISR + port_base));
			i2c_fifo_init(port_idx, i2c_fast_mode);
			return -1;
		} else if (i > 2000) {	/*200ms timeout */
			LogMsg(0, "i2c_fifo_wait_status time out!0x%x,0x%x\n",
			      mmio_read_32(TWSI_ICR + port_base),
			      mmio_read_32(TWSI_ISR + port_base));
			i2c_fifo_init(port_idx, i2c_fast_mode);
			return -1;
		}
	}
	return 0;
}

int i2c_fifo_tx_8a_16d(int port_idx, uint8_t slave_addr, uint8_t reg_addr, uint16_t reg_value)
{
	uint32_t port_base = twsi_base[port_idx];
	uint32_t repeat = 3;
	uint32_t status = 0;
	uint8_t value_hb, value_lb;

	value_hb = ( reg_value >> 8) & 0xff;
	value_lb = reg_value & 0xff;

	LogMsg(1, "slave addr %x, reg_addr %x, reg_value %x\n", slave_addr, reg_addr, reg_value);
	while (repeat--) {
		if ((HS_MODE == i2c_fast_mode) || (HS_MODE_FAST == i2c_fast_mode))
			mmio_write_32(TWSI_WFIFO + port_base, START_BYTE_CNTROL | 0x0e);	//Master code for HS
		mmio_write_32(TWSI_WFIFO + port_base,
			      START_BYTE_CNTROL | I2C_SLAVE_WRITE(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base, TB_CNTROL | reg_addr);
		mmio_write_32(TWSI_WFIFO + port_base, TB_CNTROL | value_hb);
		mmio_write_32(TWSI_WFIFO + port_base,
			      END_BYTE_CNTROL | value_lb);

		i2c_fifo_clear_status(port_idx, I2C_INT_ALL);
		status = i2c_fifo_wait_status(port_idx,
					 TWSI_ISR_TXE | TWSI_ISR_TXDONE);

		i2c_fifo_clear_control(port_idx);
		if (0 == status)
			return 0;
	}

	LogMsg(0, "i2c tx 8a_16d failed cr(0x%x),sr(0x%x)\n", mmio_read_32(TWSI_ICR + port_base),
		mmio_read_32(TWSI_ISR + port_base));
	return -1;
}

int i2c_fifo_rx_8a_16d(int port_idx, uint8_t slave_addr, uint8_t reg_addr, uint16_t *rx_val)
{
	uint32_t port_base = twsi_base[port_idx];
	uint16_t value;
	uint8_t value_hb;
	uint32_t repeat = 3;
	uint32_t status = 0;

	while (repeat--) {
		if ((HS_MODE == i2c_fast_mode)
		    || (HS_MODE_FAST == i2c_fast_mode))
			mmio_write_32(TWSI_WFIFO + port_base, START_BYTE_CNTROL | 0x0e);	//Master code for HS
		mmio_write_32(TWSI_WFIFO + port_base,
			      START_BYTE_CNTROL | I2C_SLAVE_WRITE(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base, TB_CNTROL | reg_addr);
		mmio_write_32(TWSI_WFIFO + port_base,
			      START_BYTE_CNTROL | I2C_SLAVE_READ(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base,
			      TB_CNTROL | I2C_SLAVE_READ(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base,
			      RX_END_BYTE_CNTROL | I2C_SLAVE_READ(slave_addr));

		i2c_fifo_clear_status(port_idx, I2C_INT_ALL);
		status = i2c_fifo_wait_status(port_idx,
					 TWSI_ISR_TXE | TWSI_ISR_TXDONE);

		value_hb = mmio_read_32(TWSI_RFIFO + port_base);
		value = mmio_read_32(TWSI_RFIFO + port_base);
		value |= value_hb << 8;

		i2c_fifo_clear_control(port_idx);

		if (0 == status) {
			*rx_val = value;
			return 0;
		}
	}

	LogMsg(0, "i2c rx 8a_16d failed cr(0x%x),sr(0x%x)\n", mmio_read_32(TWSI_ICR + port_base),
		mmio_read_32(TWSI_ISR + port_base));
	return -1;
}

int i2c_fifo_tx_8a_32d(int port_idx, uint8_t slave_addr, uint8_t reg_addr, uint32_t reg_value)
{
	uint32_t port_base = twsi_base[port_idx];
	uint32_t repeat = 3;
	uint32_t status = 0;
	uint8_t tmp4, tmp3, tmp2, tmp1;

	tmp1 = reg_value & 0xff;
	tmp2 = ( reg_value >> 8) & 0xff;
	tmp3 = ( reg_value >> 16) & 0xff;
	tmp4 = ( reg_value >> 24) & 0xff;

	while (repeat--) {
		if ((HS_MODE == i2c_fast_mode) || (HS_MODE_FAST == i2c_fast_mode))
			mmio_write_32(TWSI_WFIFO + port_base, START_BYTE_CNTROL | 0x0e);	//Master code for HS
		mmio_write_32(TWSI_WFIFO + port_base,
			      START_BYTE_CNTROL | I2C_SLAVE_WRITE(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base, TB_CNTROL | reg_addr);
		mmio_write_32(TWSI_WFIFO + port_base, TB_CNTROL | tmp4);
		mmio_write_32(TWSI_WFIFO + port_base, TB_CNTROL | tmp3);
		mmio_write_32(TWSI_WFIFO + port_base, TB_CNTROL | tmp2);
		mmio_write_32(TWSI_WFIFO + port_base,
			      END_BYTE_CNTROL | tmp1);

		i2c_fifo_clear_status(port_idx, I2C_INT_ALL);
		status = i2c_fifo_wait_status(port_idx,
					 TWSI_ISR_TXE | TWSI_ISR_TXDONE);

		i2c_fifo_clear_control(port_idx);
		if (0 == status)
			return 0;
	}

	LogMsg(0, "i2c tx 8a_32d failed cr(0x%x),sr(0x%x)\n", mmio_read_32(TWSI_ICR + port_base),
		mmio_read_32(TWSI_ISR + port_base));
	return -1;
}


int i2c_fifo_rx_8a_32d(int port_idx, uint8_t slave_addr, uint8_t reg_addr, uint32_t *rx_val)
{
	uint32_t port_base = twsi_base[port_idx];
	uint32_t value = 0;
	uint8_t tmp1, tmp2, tmp3, tmp4;
	uint32_t repeat = 3;
	uint32_t status = 0;

	while (repeat--) {
		if ((HS_MODE == i2c_fast_mode)
		    || (HS_MODE_FAST == i2c_fast_mode))
			mmio_write_32(TWSI_WFIFO + port_base, START_BYTE_CNTROL | 0x0e);	//Master code for HS
		mmio_write_32(TWSI_WFIFO + port_base,
			      START_BYTE_CNTROL | I2C_SLAVE_WRITE(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base, TB_CNTROL | reg_addr);
		mmio_write_32(TWSI_WFIFO + port_base,
			      START_BYTE_CNTROL | I2C_SLAVE_READ(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base,
			      TB_CNTROL | I2C_SLAVE_READ(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base,
			      TB_CNTROL | I2C_SLAVE_READ(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base,
			      TB_CNTROL | I2C_SLAVE_READ(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base,
			      RX_END_BYTE_CNTROL | I2C_SLAVE_READ(slave_addr));

		i2c_fifo_clear_status(port_idx, I2C_INT_ALL);
		status = i2c_fifo_wait_status(port_idx,
					 TWSI_ISR_TXE | TWSI_ISR_TXDONE);

		tmp1 = mmio_read_32(TWSI_RFIFO + port_base);
		tmp2 = mmio_read_32(TWSI_RFIFO + port_base);
		tmp3 = mmio_read_32(TWSI_RFIFO + port_base);
		tmp4 = mmio_read_32(TWSI_RFIFO + port_base);

		value = (tmp4 << 24) | (tmp3 << 16) | (tmp2 << 8) | (tmp1);

		i2c_fifo_clear_control(port_idx);

		if (0 == status) {
			*rx_val = value;
			return 0;
		}
	}

	LogMsg(0, "i2c rx 8a_32d failed cr(0x%x),sr(0x%x)\n", mmio_read_32(TWSI_ICR + port_base),
		mmio_read_32(TWSI_ISR + port_base));
	return -1;
}


void i2c_fifo_tx_8a_8d(int port_idx, uint8_t slave_addr, uint8_t reg_addr,
		       uint8_t reg_value)
{
	uint32_t port_base = twsi_base[port_idx];
	uint32_t repeat = 3;
	uint32_t status = 0;
	while (repeat--) {
		if ((HS_MODE == i2c_fast_mode) || (HS_MODE_FAST == i2c_fast_mode))
			mmio_write_32(TWSI_WFIFO + port_base, START_BYTE_CNTROL | 0x0e);	//Master code for HS
		mmio_write_32(TWSI_WFIFO + port_base,
			      START_BYTE_CNTROL | I2C_SLAVE_WRITE(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base, TB_CNTROL | reg_addr);
		mmio_write_32(TWSI_WFIFO + port_base,
			      END_BYTE_CNTROL | reg_value);

		i2c_fifo_clear_status(port_idx, I2C_INT_ALL);
		status = i2c_fifo_wait_status(port_idx,
					 TWSI_ISR_TXE | TWSI_ISR_TXDONE);

		//i2c_clear_tx_rx_fifo(port_idx);
		i2c_fifo_clear_control(port_idx);
		if (0 == status)
			return;
	}
	//wdt_reset();
	while (1) ;
}

int i2c_fifo_rx_8a_8d(int port_idx, uint8_t slave_addr, uint8_t reg_addr)
{
	uint32_t port_base = twsi_base[port_idx];
	uint8_t value;
	uint32_t repeat = 3;
	uint32_t status = 0;

	while (repeat--) {
		if ((HS_MODE == i2c_fast_mode)
		    || (HS_MODE_FAST == i2c_fast_mode))
			mmio_write_32(TWSI_WFIFO + port_base, START_BYTE_CNTROL | 0x0e);	//Master code for HS
		mmio_write_32(TWSI_WFIFO + port_base,
			      START_BYTE_CNTROL | I2C_SLAVE_WRITE(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base, TB_CNTROL | reg_addr);
		mmio_write_32(TWSI_WFIFO + port_base,
			      START_BYTE_CNTROL | I2C_SLAVE_READ(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base,
			      RX_END_BYTE_CNTROL | I2C_SLAVE_READ(slave_addr));

		i2c_fifo_clear_status(port_idx, I2C_INT_ALL);
		status = i2c_fifo_wait_status(port_idx,
					 TWSI_ISR_TXE | TWSI_ISR_TXDONE);

		value = mmio_read_32(TWSI_RFIFO + port_base);

		//i2c_clear_tx_rx_fifo(port_idx);
		i2c_fifo_clear_control(port_idx);

		if (0 == status)
			return value;
	}
	//wdt_reset();
	while (1) ;
}

int i2c_fifo_tx_8a_8d_1(int port_idx, uint8_t slave_addr, uint8_t reg_addr, uint8_t reg_value)
{
	uint32_t port_base = twsi_base[port_idx];
	uint32_t repeat = 3;
	uint32_t status = 0;
	while (repeat--) {
		if ((HS_MODE == i2c_fast_mode) || (HS_MODE_FAST == i2c_fast_mode))
			mmio_write_32(TWSI_WFIFO + port_base, START_BYTE_CNTROL | 0x0e);	//Master code for HS
		mmio_write_32(TWSI_WFIFO + port_base,
			      START_BYTE_CNTROL | I2C_SLAVE_WRITE(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base, TB_CNTROL | reg_addr);
		mmio_write_32(TWSI_WFIFO + port_base,
			      END_BYTE_CNTROL | reg_value);

		i2c_fifo_clear_status(port_idx, I2C_INT_ALL);
		status = i2c_fifo_wait_status(port_idx,
					 TWSI_ISR_TXE | TWSI_ISR_TXDONE);

		//i2c_clear_tx_rx_fifo(port_idx);
		i2c_fifo_clear_control(port_idx);
		if (0 == status)
			return 0;
	}
	LogMsg(0, "i2c tx 8a_8d_1 failed cr(0x%x),sr(0x%x)\n", mmio_read_32(TWSI_ICR + port_base),
				mmio_read_32(TWSI_ISR + port_base));
	return -1;
}

int i2c_fifo_rx_8a_8d_1(int port_idx, uint8_t slave_addr, uint8_t reg_addr, uint8_t *rx_val)
{
	uint32_t port_base = twsi_base[port_idx];
	uint8_t value;
	uint32_t repeat = 3;
	uint32_t status = 0;

	while (repeat--) {
		if ((HS_MODE == i2c_fast_mode)
		    || (HS_MODE_FAST == i2c_fast_mode))
			mmio_write_32(TWSI_WFIFO + port_base, START_BYTE_CNTROL | 0x0e);	//Master code for HS
		mmio_write_32(TWSI_WFIFO + port_base,
			      START_BYTE_CNTROL | I2C_SLAVE_WRITE(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base, TB_CNTROL | reg_addr);
		mmio_write_32(TWSI_WFIFO + port_base,
			      START_BYTE_CNTROL | I2C_SLAVE_READ(slave_addr));
		mmio_write_32(TWSI_WFIFO + port_base,
			      RX_END_BYTE_CNTROL | I2C_SLAVE_READ(slave_addr));

		i2c_fifo_clear_status(port_idx, I2C_INT_ALL);
		status = i2c_fifo_wait_status(port_idx,
					 TWSI_ISR_TXE | TWSI_ISR_TXDONE);

		value = mmio_read_32(TWSI_RFIFO + port_base);

		//i2c_clear_tx_rx_fifo(port_idx);
		i2c_fifo_clear_control(port_idx);

		if (0 == status) {
			*rx_val = value;
			return 0;
		}
	}
	LogMsg(0, "i2c rx 8a_8d_1 failed cr(0x%x),sr(0x%x)\n", mmio_read_32(TWSI_ICR + port_base),
			mmio_read_32(TWSI_ISR + port_base));
	return -1;
}


static uint8_t pmic_i2c_inited = 1;

static void pmic_i2c_init(uint8_t i2c_bus)
{
	if (pmic_i2c_inited) {
		i2c_fifo_init(i2c_bus, STANDARD_MODE);
		pmic_i2c_inited = 0;
	}
}

uint8_t pmic_read(uint8_t i2c_bus, uint8_t addr, uint8_t reg)
{
	pmic_i2c_init(i2c_bus);
	return i2c_fifo_rx_8a_8d(i2c_bus, addr << 1, reg);
}

int pmic_detect(uint8_t i2c_bus, uint8_t addr, uint8_t reg_addr)
{
	uint32_t port_base = twsi_base[i2c_bus];
	uint8_t  slave_addr=addr<<1;
	uint32_t status = 0;

	pmic_i2c_init(i2c_bus);

	if ((HS_MODE == i2c_fast_mode) || (HS_MODE_FAST == i2c_fast_mode))
		mmio_write_32(TWSI_WFIFO + port_base, START_BYTE_CNTROL | 0x0e);
	mmio_write_32(TWSI_WFIFO + port_base,
		      START_BYTE_CNTROL | I2C_SLAVE_WRITE(slave_addr));
	mmio_write_32(TWSI_WFIFO + port_base, TB_CNTROL | reg_addr);
	mmio_write_32(TWSI_WFIFO + port_base,
		      START_BYTE_CNTROL | I2C_SLAVE_READ(slave_addr));
	mmio_write_32(TWSI_WFIFO + port_base,
		      RX_END_BYTE_CNTROL | I2C_SLAVE_READ(slave_addr));

	i2c_fifo_clear_status(i2c_bus, I2C_INT_ALL);
	status = i2c_fifo_wait_status(i2c_bus, TWSI_ISR_TXE | TWSI_ISR_TXDONE);
	i2c_fifo_clear_control(i2c_bus);

	return status;
}

uint8_t pmic_write_with_check(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint8_t reg_val, uint8_t check_val)
{
	int timeout = 3;
	uint32_t val = 0;

	pmic_i2c_init(i2c_bus);

	do {
		i2c_fifo_tx_8a_8d(i2c_bus, addr << 1, reg, reg_val);
		val = i2c_fifo_rx_8a_8d(i2c_bus, addr << 1, reg);
	} while ((val != check_val) && (timeout--));

	if ((timeout < 0) || (val != check_val)) {
		LogMsg(0, "pmic write reg: %x, val: %x failed, cur_val: %x != check_val: %x\n",
		      reg, reg_val, val, check_val);
		//wdt_reset();
		while (1) ;
	} else
		return val;
}

uint8_t pmic_write(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint8_t reg_val)
{
	int timeout = 3;
	uint32_t val = 0;

	pmic_i2c_init(i2c_bus);

	do {
		i2c_fifo_tx_8a_8d(i2c_bus, addr << 1, reg, reg_val);
		val = i2c_fifo_rx_8a_8d(i2c_bus, addr << 1, reg);
	} while ((val != reg_val) && (timeout--));

	if ((timeout < 0) || (val != reg_val)) {
		LogMsg(0, "pmic write reg: %x, val: %x failed, cur_val: %x\n",
		      reg, reg_val, val);
		//wdt_reset();
		while (1) ;
	} else
		return val;
}

int pmic_read8(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint8_t *rx_val)
{
	pmic_i2c_init(i2c_bus);
	return i2c_fifo_rx_8a_8d_1(i2c_bus, addr << 1, reg, rx_val);
}

int pmic_write8(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint8_t reg_val)
{
	int timeout = 3;
	uint8_t rx_val  = reg_val + 1;

	pmic_i2c_init(i2c_bus);

	do {
		i2c_fifo_tx_8a_8d_1(i2c_bus, addr << 1, reg, reg_val);
		i2c_fifo_rx_8a_8d_1(i2c_bus, addr << 1, reg, &rx_val);
	} while ((rx_val != reg_val) && (timeout--));

	if ((timeout < 0) && (rx_val != reg_val)) {
		LogMsg(0, "pmic write reg: %x, val: %x failed, cur_val: %x\n",
		      reg, reg_val, rx_val);
		return -1;
	} else
		return 0;
}

int pmic_read16(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint16_t *rx_val)
{
	pmic_i2c_init(i2c_bus);
	return i2c_fifo_rx_8a_16d(i2c_bus, addr << 1, reg, rx_val);
}

int pmic_write16(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint16_t reg_val)
{
	int timeout = 3;
	// make rx_val difference of reg_val;
	uint16_t rx_val  = reg_val + 1;

	pmic_i2c_init(i2c_bus);

	do {
		i2c_fifo_tx_8a_16d(i2c_bus, addr << 1, reg, reg_val);
		i2c_fifo_rx_8a_16d(i2c_bus, addr << 1, reg, &rx_val);
	} while ((rx_val != reg_val) && (timeout--));

	if ((timeout < 0) && (rx_val != reg_val)) {
		LogMsg(0, "pmic write reg: %x, val: %x failed, cur_val: %x\n",
		      reg, reg_val, rx_val);
		return -1;
	} else
		return 0;
}

int pmic_read32(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint32_t *rx_val)
{
	pmic_i2c_init(i2c_bus);
	return i2c_fifo_rx_8a_32d(i2c_bus, addr << 1, reg, rx_val);
}

int pmic_write32(uint8_t i2c_bus, uint8_t addr, uint8_t reg, uint32_t reg_val)
{
	int timeout = 3;
	// make rx_val difference of reg_val;
	uint32_t rx_val  = reg_val + 1;

	pmic_i2c_init(i2c_bus);

	do {
		i2c_fifo_tx_8a_32d(i2c_bus, addr << 1, reg, reg_val);
		i2c_fifo_rx_8a_32d(i2c_bus, addr << 1, reg, &rx_val);
	} while ((rx_val != reg_val) && (timeout--));

	if ((timeout < 0) && (rx_val != reg_val)) {
		LogMsg(0, "pmic write reg: %x, val: %x failed, cur_val: %x\n",
		      reg, reg_val, rx_val);
		return -1;
	} else
		return 0;
}

uint32_t set_vcc_m1(uint16_t reg_value)
{
	uint32_t i2c_no = 8;
	// make rx_val difference of reg_val;
	uint8_t rx_val  = reg_value + 1;

	pmic_i2c_init(i2c_no);

	pmic_write_with_check(i2c_no, 0x31, 0x30, reg_value, reg_value | (1 << 7));


	return (uint32_t)rx_val;

}

void set_reboot_reason(uint8_t reason)
{
	uint32_t i2c_no = 3;
	uint32_t val = 0;

	i2c_fifo_tx_8a_8d(i2c_no, 0x32 * 2, 0xc0, reason);
	val = i2c_fifo_rx_8a_8d(i2c_no, 0x32 * 2, 0xc0);
	LogMsg(1, "reboot_reason read=0x%x\n", val);
}

void enable_avdd_usb(void)
{
	uint32_t i2c_no = 3;
	uint32_t val = 0;
	val = i2c_fifo_rx_8a_8d(i2c_no, 0x31 * 2, 0x9);
	if (0 == (val & BIT(0))) {
		val |= BIT(0);
		i2c_fifo_tx_8a_8d(i2c_no, 0x31 * 2, 0x9, val);
		LogMsg(1, "LDO enable 0x%x,value 0x%x\n",
		       i2c_fifo_rx_8a_8d(i2c_no, 0x31 * 2, 0x9),
		       i2c_fifo_rx_8a_8d(i2c_no, 0x31 * 2, 0x30));
	}
}

int is_adb_reboot_download_mode(void)
{
	uint32_t i2c_no = 3;
	uint32_t val = 0;
	uint8_t reason = 8;

	val = i2c_fifo_rx_8a_8d(i2c_no, 0x32 * 2, 0xc0);
	if (val == reason) {
		val &= ~reason;
		i2c_fifo_tx_8a_8d(i2c_no, 0x32 * 2, 0xc0, val);
		LogMsg(1, "now reboot_reason 0x%x\n",
		       i2c_fifo_rx_8a_8d(i2c_no, 0x32 * 2, 0xc0));
		enable_avdd_usb();
		return 1;
	} else {
		return 0;
	}
}

void pmic_reset(void)
{
	/* TODO */
}

unsigned int whether_bootup_from_powerdown(void)
{
	uint32_t i2c_no = 3;
	uint32_t pmic_log = 0;

	pmic_log = i2c_fifo_rx_8a_8d(i2c_no, PMIC_88PM802_ADDRESS * 2,
			      POWER_UP_LOG) & 0xff;
	pmic_log |= (i2c_fifo_rx_8a_8d
	     (i2c_no, PMIC_88PM802_ADDRESS * 2, POWER_DOWN_LOG1) & 0xff) << 8;
	pmic_log |= (i2c_fifo_rx_8a_8d
	     (i2c_no, PMIC_88PM802_ADDRESS * 2, POWER_DOWN_LOG2) & 0xff) << 16;
	LogMsg(1, "pmic_log 0x%x\n", pmic_log);
	if (pmic_log & BOOTUP_FROM_POWERDOWN)
		return 1;
	else
		return 0;
}

#endif
