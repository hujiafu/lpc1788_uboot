/*
 * (C) Copyright 2011
 *
 * Alexander Potashev, Emcraft Systems, aspotashev@emcraft.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Board specific code for the Embedded Artists LPC1788 board
 */

#include <common.h>
#include <netdev.h>
#include <video_fb.h>
#include <asm/arch/lpc178x_gpio.h>

/*
 * SDRAM-specific configuration
 */
/*
 * Programmable delay value for EMC outputs in command delayed mode
 */
#define LPC178X_EMC_CMDDLY	0x10
/*
 * Programmable delay value for the feedback clock that controls input data
 * sampling
 */
#define LPC178X_EMC_FBCLKDLY	0x10
/*
 * IS42S32800B SDRAM: 32-bit, 4 banks, 12 rows, 9 cols.
 * See Table 133 "Address mapping" in the LPC178x/7x User Manual.
 */
#define LPC178X_EMC_AM		0x89

#if defined(CONFIG_LPC178X_EMC_HALFCPU)
/*
 * Timing for 54MHz bus
 */
#define LPC178X_EMC_RAS		2
#define LPC178X_EMC_CAS		2
/* Command delayed strategy, using EMCCLKDELAY */
#define LPC178X_EMC_RDCFG_RD	1
/* Precharge command period (tRP) = 1 EMC clock cycle */
#define LPC178X_EMC_T_RP	1
/* Active to precharge command perion (tRAS) = 3 clocks */
#define LPC178X_EMC_T_RAS	3
/* Self-refresh exit time (tSREX) = 4 clocks */
#define LPC178X_EMC_T_SREX	4
/* Last-data-out to active command time (tAPR) = 2 clocks */
#define LPC178X_EMC_T_APR	2
/* Data-in to active command (tDAL) = 2 clocks */
#define LPC178X_EMC_T_DAL	2
/* Write recovery time (tWR) = 2 clocks */
#define LPC178X_EMC_T_WR	2
/* Active to active command perion (tRC) = 4 clocks */
#define LPC178X_EMC_T_RC	4
/*
 * Auto-refresh period and auto-refresh to active command period
 * (tRFC) = 4 clocks
 */
#define LPC178X_EMC_T_RFC	4
/* Exit self-refresh to active command time (tXSR) = 4 clocks */
#define LPC178X_EMC_T_XSR	4
/* Active bank A to active bank B latency (tRRD) = 1 clock */
#define LPC178X_EMC_T_RRD	1
/* Load mode register to active command time (tMRD) = 1 clock */
#define LPC178X_EMC_T_MRD	1

#else

/*
 * Timing for 108MHz bus
 */
#define LPC178X_EMC_RAS		4
#define LPC178X_EMC_CAS		4
/* Command delayed strategy, using EMCCLKDELAY */
#define LPC178X_EMC_RDCFG_RD	2
/* Precharge command period (tRP) = 1 EMC clock cycle */
#define LPC178X_EMC_T_RP	2
/* Active to precharge command perion (tRAS) = 3 clocks */
#define LPC178X_EMC_T_RAS	6
/* Self-refresh exit time (tSREX) = 4 clocks */
#define LPC178X_EMC_T_SREX	8
/* Last-data-out to active command time (tAPR) = 2 clocks */
#define LPC178X_EMC_T_APR	4
/* Data-in to active command (tDAL) = 2 clocks */
#define LPC178X_EMC_T_DAL	4
/* Write recovery time (tWR) = 2 clocks */
#define LPC178X_EMC_T_WR	4
/* Active to active command perion (tRC) = 4 clocks */
#define LPC178X_EMC_T_RC	8
/*
 * Auto-refresh period and auto-refresh to active command period
 * (tRFC) = 4 clocks
 */
#define LPC178X_EMC_T_RFC	8
/* Exit self-refresh to active command time (tXSR) = 4 clocks */
#define LPC178X_EMC_T_XSR	8
/* Active bank A to active bank B latency (tRRD) = 1 clock */
#define LPC178X_EMC_T_RRD	2
/* Load mode register to active command time (tMRD) = 1 clock */
#define LPC178X_EMC_T_MRD	2

#endif
/*
 * Refresh timer.
 * Indicates the multiple of 16 CCLKs between SDRAM refresh cycles.
 */
#define LPC178X_EMC_REFRESH		0x2E
#define LPC178X_EMC_REFRESH_FAST	2	/* Only for initialization */

/*
 * See IS42S32800B mode register (IS42S32800B datasheet, page 16).
 * CAS2, Burst Length = 4.
 */
#define LPC178X_EMC_MODEREG_BL		2	/* Burst Length code */
#define LPC178X_EMC_MODEREG_CAS		2	/* CAS Latency */

/*
 * SCC registers
 */
/*
 * EMC Delay Control register
 */
/* Programmable delay value for EMC outputs in command delayed mode */
#define LPC178X_SCC_EMCDLYCTL_CMDDLY_BITS	0
/*
 * Programmable delay value for the feedback clock that controls input data
 * sampling
 */
#define LPC178X_SCC_EMCDLYCTL_FBCLKDLY_BITS	8

/*
 * EMC registers
 */
/*
 * EMC Control register
 */
#define LPC178X_EMC_CTRL_EN_MSK		(1 << 0)

/* EMC data pins (DQ0..DQ31) */
#define LPC178X_EMC_DATA_PINS	31
#if !defined(CONFIG_SYS_FLASH_CS)
/* EMC row/column address pins (A0..A11) */
#define LPC178X_EMC_ADDR_PINS	12
#else
/* ..and NOR Flash pins up to A22 */
#define LPC178X_EMC_ADDR_PINS	22
#endif

/*
 * Dynamic Memory Control register
 */
/* Dynamic memory clock enable (CE) */
#define LPC178X_EMC_DYCTRL_CE_MSK	(1 << 0)
/* Dynamic memory clock control (CS) */
#define LPC178X_EMC_DYCTRL_CS_MSK	(1 << 1)
/* SDRAM initialization (I) */
#define LPC178X_EMC_DYCTRL_I_BITS	7
#define LPC178X_EMC_DYCTRL_I_NORMAL	0
#define LPC178X_EMC_DYCTRL_I_MODE	1
#define LPC178X_EMC_DYCTRL_I_PALL	2	/* precharge all */
#define LPC178X_EMC_DYCTRL_I_NOP	3	/* no operation */

/*
 * Dynamic Memory Read Configuration register:
 *     Read data strategy (RD)
 */
#define LPC178X_EMC_DYRDCFG_RD_BITS	0

/*
 * The SDRAM chip (IS42S32800B) mode register.
 * See IS42S32800B datasheet, page 16.
 */
#define LPC178X_EMC_MODEREG_BL_BITS	0	/* Burst Length */
#define LPC178X_EMC_MODEREG_CAS_BITS	4	/* CAS Latency */

#define LPC178X_EMC_MODEREG_VALUE \
	((LPC178X_EMC_MODEREG_BL << LPC178X_EMC_MODEREG_BL_BITS) | \
	(LPC178X_EMC_MODEREG_CAS << LPC178X_EMC_MODEREG_CAS_BITS))

/*
 * Offset of the 12 least-significant bits of mode register (A0..A11)
 * in addresses on the AHB bus.
 *
 * In the high-performance mode the shift should be the following:
 * 13 = 9 (column bits) + 2 (bank select bits) + 2 (32 bits)
 *    1. IS42S32800B SDRAM has 512 columns, therefore 9 bits are used for the column number.
 *    2. Bank select field has 2 bits.
 *    3. `2` is log2(32/8), because the SDRAM chip is 32-bit, and its
 *        internal addresses do not have 2 least-significant bits of
 *        the AHB bus addresses.
 *
 * In the low-power mode this shift will be different.
 */
#define LPC178X_EMC_MODEREG_ADDR_SHIFT	13

/*
 * Dynamic Memory registers (per chip)
 */
/*
 * Dynamic Memory Configuration register
 */
/* Address mapping */
#define LPC178X_EMC_DYCFG_AM_BITS	7
/* Buffer enable */
#define LPC178X_EMC_DYCFG_B_MSK		(1 << 19)
/*
 * Dynamic Memory RAS & CAS Delay register
 */
/* RAS latency */
#define LPC178X_EMC_DYRASCAS_RAS_BITS	0
/* CAS latency */
#define LPC178X_EMC_DYRASCAS_CAS_BITS	8

/*
 * EMC per-chip registers for DRAM.
 *
 * This structure must be 0x20 bytes in size
 * (for `struct lpc178x_emc_regs` to be correct.)
 */
struct lpc178x_emc_dy_regs {
	u32 cfg;	/* Dynamic Memory Configuration register */
	u32 rascas;	/* Dynamic Memory RAS & CAS Delay registers */
	u32 rsv0[6];
};

/*
 * EMC controls for Static Memory CS. Each block occupies 0x20 bytes.
 */
struct lpc178x_emc_st_regs {
	u32 cfg;	/* Static Memory Configuration register */
	u32 we;		/* CS to WE delay register */
	u32 oe;		/* CS to OE delay register */
	u32 rd;		/* CS to Read delay register */
	u32 page;	/* async page mode access delay */
	u32 wr;		/* CS to Write delay register */
	u32 ta;		/* number of turnaround cycles */
	u32 rsv0[1];
};

/*
 * EMC (External Memory Controller) register map
 * Should be mapped at 0x2009C000.
 */
struct lpc178x_emc_regs {
	/* 0x2009C000 */
	u32 emcctrl;	/* EMC Control register */
	u32 emcsts;	/* EMC Status register */
	u32 emccfg;	/* EMC Configuration register */
	u32 rsv0[5];

	/* 0x2009C020 */
	u32 dy_ctrl;	/* Dynamic Memory Control register */
	u32 dy_rfsh;	/* Dynamic Memory Refresh Timer register */
	u32 dy_rdcfg;	/* Dynamic Memory Read Configuration register */
	u32 rsv1;

	/* 0x2009C030 */
	u32 dy_trp;	/* Dynamic Memory Precharge Command Period register */
	u32 dy_tras;	/* Dynamic Memory Active to Precharge Command
				Period register */
	u32 dy_srex;	/* Dynamic Memory Self-refresh Exit Time register */
	u32 dy_apr;	/* Dynamic Memory Last Data Out to Active
				Time register */
	u32 dy_dal;	/* Dynamic Memory Data-in to Active Command
				Time register */
	u32 dy_wr;	/* Dynamic Memory Write Recovery Time register */
	u32 dy_rc;	/* Dynamic Memory Active to Active Command
				Period register */
	u32 dy_rfc;	/* Dynamic Memory Auto-refresh Period register */
	u32 dy_xsr;	/* Dynamic Memory Exit Self-refresh register */
	u32 dy_rrd;	/* Dynamic Memory Active Bank A to
				Active Bank B Time register */
	u32 dy_mrd;	/* Dynamic Memory Load Mode register to
				Active Command Time */
	/* 0x2009C05C */
	u32 rsv2[41];

	/* 0x2009C100 */
	struct lpc178x_emc_dy_regs dy[4];	/* 4 DRAM chips are possible */
	u32 rsv3[32];
	/* 0x2009C200 */
	struct lpc178x_emc_st_regs st[4];	/* 4 Static RAM devices (flash) */
};

#define LPC178X_EMC_BASE		(LPC178X_AHB_PERIPH_BASE + 0x0001C000)
#define LPC178X_EMC			((volatile struct lpc178x_emc_regs *) \
					LPC178X_EMC_BASE)

DECLARE_GLOBAL_DATA_PTR;

/*
 * GPIO pin configuration table for EA-LPC1788-32
 *
 * This table does not list all GPIO pins that will be configured. See also
 * the code in `gpio_init()`.
 */
static const struct lpc178x_gpio_pin_config ea_lpc1788_gpio[] = {
	/*
	 * GPIO configuration for UART
	 */
#if CONFIG_LPC178X_UART_PORT == 0
	/* P0.2 (D) = UART0 TXD */
	{{0,  2}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 0, 0)},
	/* P0.3 (D) = UART0 RXD */
	{{0,  3}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 0, 0)},
#elif CONFIG_LPC178X_UART_PORT == 2
	/* P0.10 (D) = U2_TXD */
	{{0, 10}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 0, 0)},
	/* P0.11 (D) = U2_RXD */
	{{0, 11}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 0, 0)},
#else /* Neither UART0 nor UART2 */
#error This configuration of GPIO pins supports only UART0 or UART2
#endif

#ifdef CONFIG_NR_DRAM_BANKS
	/*
	 * GPIO configuration for SDRAM
	 */
#define LPC178X_GPIO_EMC_REGVAL \
	(LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0))

	/* Configure EMC bank address select 0 and 1 (BA0, BA1) */
	{{4, 13}, LPC178X_GPIO_EMC_REGVAL},
	{{4, 14}, LPC178X_GPIO_EMC_REGVAL},

	/* Configure EMC column address strobe (CAS) */
	{{2, 16}, LPC178X_GPIO_EMC_REGVAL},
	/* Configure EMC row address strobe (RAS) */
	{{2, 17}, LPC178X_GPIO_EMC_REGVAL},

	/* Configure EMC write enable (WE) */
	{{4, 25}, LPC178X_GPIO_EMC_REGVAL},

	/* Configure EMC clock input (CLK) */
	{{2, 18}, LPC178X_GPIO_EMC_REGVAL},
	/* Configure EMC clock enable (CKE) */
	{{2, 24}, LPC178X_GPIO_EMC_REGVAL},

	/* Configure EMC chip select (DYCS0) */
	{{2, 20}, LPC178X_GPIO_EMC_REGVAL},

	/* Configure EMC I/O mask (DQM0..DQM3) */
	{{2, 28}, LPC178X_GPIO_EMC_REGVAL},
	{{2, 29}, LPC178X_GPIO_EMC_REGVAL},
	{{2, 30}, LPC178X_GPIO_EMC_REGVAL},
	{{2, 31}, LPC178X_GPIO_EMC_REGVAL},
#endif /* CONFIG_NR_DRAM_BANKS */

#ifdef CONFIG_LPC178X_ETH
	/*
	 * GPIO configuration for Ethernet
	 */
	/* P1.0 (D) = RMII ENET_TXD0 */
	{{1,  0}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	/* P1.1 (D) = RMII ENET_TXD1 */
	{{1,  1}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	/* P1.4 (D) = RMII ENET_TX_EN */
	{{1,  4}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	/* P1.8 (D) = RMII CRS */
	{{1,  8}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	/* P1.9 (D) = RMII RXD0 */
	{{1,  9}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	/* P1.10 (D) = RMII RXD1 */
	{{1, 10}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	/* P1.14 (D) = RMII RXER */
	{{1, 14}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 0, 0)},
	/* P1.15 (D) = RMII CLK */
	{{1, 15}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 0, 0)},
	/* P1.16 (D) = RMII MCD */
	{{1, 16}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 0, 0)},
	/* P1.17 (D) = RMII MDIO */
	{{1, 17}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 0, 0)},
	
	/* P1.13 (D) = RX_DV CFG NORMAL IO*/
	{{1, 13}, LPC178X_GPIO_CONFIG_D(0, LPC178X_NO_PULLUP, 0, 0, 0, 0)},
	/* P1.18 (D) = RST */
	{{1, 18}, LPC178X_GPIO_CONFIG_D(0, LPC178X_NO_PULLUP, 0, 0, 0, 0)},
#endif /* CONFIG_LPC178X_ETH */
#ifdef CONFIG_SYS_FLASH_CS
	/*
	 * GPIO configuration for Flash.
	 */
	/* P4.30 (D) = NOR FLash CS0 */
	{{4, 30}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	/* P4.31 (D) = NOR FLash CS1 */
	{{4, 31}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	/* P4.24 (D) = NOR FLash BOE */
	{{4, 24}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	/* P4.25 (D) = NOR FLash BWE */
	{{4, 25}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	/* P4.26 (D) = Data Buffer BLS0 */
	{{4, 26}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	/* P4.27 (D) = Data Buffer BLS1 */
	{{4, 27}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	
	{{4, 0}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 1}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 2}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 3}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 4}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 5}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 6}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 7}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 8}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 9}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 10}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 11}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 12}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 13}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 14}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 15}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 16}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 17}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 18}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 19}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 20}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 21}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 22}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},
	{{4, 23}, LPC178X_GPIO_CONFIG_D(1, LPC178X_NO_PULLUP, 0, 0, 1, 0)},

#endif

#ifdef CONFIG_VIDEO
	{{2, 12}, LPC178X_GPIO_CONFIG_D(5, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[3]
	{{2, 6}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[4]
	{{2, 7}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[5]
	{{2, 8}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[6]
	{{4, 29}, LPC178X_GPIO_CONFIG_D(5, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[7]
	
	{{1, 20}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[10]
	{{1, 21}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[11]
	{{1, 22}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[12]
	{{1, 23}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[13]
	{{1, 24}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[14]
	{{1, 25}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[15]
	
	{{2, 13}, LPC178X_GPIO_CONFIG_D(5, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[19]
	{{1, 26}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[20]
	{{1, 27}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[21]
	{{1, 28}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[22]
	{{1, 29}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_VD[23]
	
	{{2, 2}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_DCLK
	{{2, 3}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_FP
	{{2, 4}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_ENABLE_M
	{{2, 5}, LPC178X_GPIO_CONFIG_D(7, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_LP
	
	{{2, 0}, LPC178X_GPIO_CONFIG_D(0, LPC178X_NO_PULLUP, 0, 0, 0, 0)}, //LCD_PWR PWM1[1]


#endif	
};

struct lpc178x_pwm_regs {
	u32 pwm_ir;
	u32 pwm_tcr;
	u32 pwm_tc;
	u32 pwm_pr;
	u32 pwm_pc;
	u32 pwm_mcr;
	u32 pwm_mr0;
	u32 pwm_mr1;
	u32 pwm_mr2;
	u32 pwm_mr3;
	u32 pwm_ccr;
	u32 pwm_cr0;
	u32 pwm_cr1;
	u32 pwm_cr2;
	u32 pwm_cr3;
	u32 pwm_rcv0[1];
	u32 pwm_mr4;
	u32 pwm_mr5;
	u32 pwm_mr6;
	u32 pwm_pcr;
	u32 pwm_ler;
	u32 pwm_rcv1[7];
	u32 pwm_ctcr;
};
#define LPC178X_PWM0_BASE		(LPC178X_APB0PERIPH_BASE + 0x00014000)
#define LPC178X_PWM1_BASE		(LPC178X_APB0PERIPH_BASE + 0x00018000)
#define LPC178X_PWM1			((volatile struct lpc178x_pwm_regs *)LPC178X_PWM1_BASE)

struct lpc178x_lcd_regs {
	u32 lcd_timh;
	u32 lcd_timv;
	u32 lcd_pol;
	u32 lcd_le;
	u32 lcd_upbase;
	u32 lcd_lpbase;
	u32 lcd_ctrl;
	u32 lcd_intmsk;
	u32 lcd_intraw;
	u32 lcd_intstat;
	u32 lcd_intclr;
	u32 lcd_upcurr;
	u32 lcd_lpcurr;
	u32 lcd_rcv1[115];
	u32 lcd_pal[128];
	u32 lcd_rcv2[256];
	u32 crsr_img[256];
	u32 crsr_ctrl;
	u32 crsr_cfg;
	u32 crsr_pal0;
	u32 crsr_pal1;
	u32 crsr_xy;
	u32 crsr_clip;
	u32 crsr_rcv1[2];
	u32 crsr_intmsk;
	u32 crsr_intclr;
	u32 crsr_intraw;
	u32 crsr_intstat;
};

#define LPC178X_LCD_BASE		(LPC178X_AHB_PERIPH_BASE + 0x00008000)
#define LPC178X_LCD			((volatile struct lpc178x_lcd_regs *)LPC178X_LCD_BASE)
/*
 * Configure all necessary GPIO pins
 */
static void gpio_init(void)
{
	/*
	 * Enable power on GPIO. This is not really necessary, because power
	 * on GPIO is enabled on SoC reset.
	 */
	lpc178x_periph_enable(LPC178X_SCC_PCONP_PCGPIO_MSK, 1);

	/*
	 * Configure GPIO pins using the `ea_lpc1788_gpio[]` table
	 */
	lpc178x_gpio_config_table(ea_lpc1788_gpio, ARRAY_SIZE(ea_lpc1788_gpio));

#ifdef CONFIG_NR_DRAM_BANKS
	/*
	 * Configure GPIO pins used for the External Memory Controller (EMC)
	 */
	struct lpc178x_gpio_dsc dsc;

	/* Configure EMC data pins (DQ0..DQ31) */
	dsc.port = 3;
	for (dsc.pin = 0; dsc.pin <= LPC178X_EMC_DATA_PINS; dsc.pin++)
		lpc178x_gpio_config(&dsc, LPC178X_GPIO_EMC_REGVAL);

	/*
	 * Configure EMC row/column address pins (A0..A11) and
	 * NOR FLash address pins.
	*/
	dsc.port = 4;
	for (dsc.pin = 0; dsc.pin <= LPC178X_EMC_ADDR_PINS; dsc.pin++)
		lpc178x_gpio_config(&dsc, LPC178X_GPIO_EMC_REGVAL);
#endif
}

/*
 * Early hardware init.
 */
int board_init(void)
{
	volatile struct lpc178x_emc_st_regs *st;

	/*
	 * Enable power on EMC
	 */
	lpc178x_periph_enable(LPC178X_SCC_PCONP_PCEMC_MSK, 1);

	/*
	 * Clock delay for EMC
	 */
	LPC178X_SCC->emcdlyctl =
		(LPC178X_EMC_CMDDLY << LPC178X_SCC_EMCDLYCTL_CMDDLY_BITS) |
		(LPC178X_EMC_FBCLKDLY << LPC178X_SCC_EMCDLYCTL_FBCLKDLY_BITS);

	/*
	 * Enable EMC
	 */
	LPC178X_EMC->emcctrl = LPC178X_EMC_CTRL_EN_MSK;
	/*
	 * Little-endian mode
	 */
	LPC178X_EMC->emccfg = 0;

	/*
	 * Enable GPIO pins
	 */
	gpio_init();

#ifdef CONFIG_SYS_FLASH_CS
	/* Set timing for flash */
	st = &LPC178X_EMC->st[CONFIG_SYS_FLASH_CS];
	st->cfg = CONFIG_SYS_FLASH_CFG;
	st->we = CONFIG_SYS_FLASH_WE;
	st->oe = CONFIG_SYS_FLASH_OE;
	st->rd = CONFIG_SYS_FLASH_RD;
	st->page  = CONFIG_SYS_FLASH_PAGE;
	st->wr = CONFIG_SYS_FLASH_WR;
	st->ta = CONFIG_SYS_FLASH_TA;
#endif
	return 0;
}

/*
 * Dump pertinent info to the console.
 */
int checkboard(void)
{
	printf("Board: EA-LPC1788 rev %s\n",
		CONFIG_SYS_BOARD_REV_STR);

	return 0;
}

/*
 * Configure board specific parts.
 */
#ifdef CONFIG_MISC_INIT_R
int misc_init_r(void)
{
	/* TBD */
	return 0;
}
#endif /* CONFIG_MISC_INIT_R */

/*
 * Setup external RAM.
 */
int dram_init(void)
{
	volatile struct lpc178x_emc_dy_regs *dy;
	u32 tmp32;

	dy = &LPC178X_EMC->dy[CONFIG_SYS_RAM_CS];

	/*
	 * Address mapping (see Table 133 from the LPC178x/7x User Manual)
	 */
	dy->cfg = (LPC178X_EMC_AM << LPC178X_EMC_DYCFG_AM_BITS);

	/*
	 * Configure DRAM timing
	 */
	dy->rascas =
		(LPC178X_EMC_RAS << LPC178X_EMC_DYRASCAS_RAS_BITS) |
		(LPC178X_EMC_CAS << LPC178X_EMC_DYRASCAS_CAS_BITS);
	LPC178X_EMC->dy_rdcfg =
		(LPC178X_EMC_RDCFG_RD << LPC178X_EMC_DYRDCFG_RD_BITS);
	LPC178X_EMC->dy_trp  = LPC178X_EMC_T_RP - 1;
	LPC178X_EMC->dy_tras = LPC178X_EMC_T_RAS - 1;
	LPC178X_EMC->dy_srex = LPC178X_EMC_T_SREX - 1;
	LPC178X_EMC->dy_apr  = LPC178X_EMC_T_APR - 1;
	LPC178X_EMC->dy_dal  = LPC178X_EMC_T_DAL;
	LPC178X_EMC->dy_wr   = LPC178X_EMC_T_WR - 1;
	LPC178X_EMC->dy_rc   = LPC178X_EMC_T_RC - 1;
	LPC178X_EMC->dy_rfc  = LPC178X_EMC_T_RFC - 1;
	LPC178X_EMC->dy_xsr  = LPC178X_EMC_T_XSR - 1;
	LPC178X_EMC->dy_rrd  = LPC178X_EMC_T_RRD - 1;
	LPC178X_EMC->dy_mrd  = LPC178X_EMC_T_MRD - 1;
	udelay(100000);

	/*
	 * Issue SDRAM NOP (no operation) command
	 */
	LPC178X_EMC->dy_ctrl =
		LPC178X_EMC_DYCTRL_CE_MSK | LPC178X_EMC_DYCTRL_CS_MSK |
		(LPC178X_EMC_DYCTRL_I_NOP << LPC178X_EMC_DYCTRL_I_BITS);
	udelay(200000);

	/*
	 * Pre-charge all with fast refresh
	 */
	LPC178X_EMC->dy_ctrl =
		LPC178X_EMC_DYCTRL_CE_MSK | LPC178X_EMC_DYCTRL_CS_MSK |
		(LPC178X_EMC_DYCTRL_I_PALL << LPC178X_EMC_DYCTRL_I_BITS);
	LPC178X_EMC->dy_rfsh = LPC178X_EMC_REFRESH_FAST;
	udelay(1000);

	/*
	 * Set refresh period
	 */
	LPC178X_EMC->dy_rfsh = LPC178X_EMC_REFRESH;

	/*
	 * Load mode word, CAS2, burst of 4
	 */
	LPC178X_EMC->dy_ctrl =
		LPC178X_EMC_DYCTRL_CE_MSK | LPC178X_EMC_DYCTRL_CS_MSK |
		(LPC178X_EMC_DYCTRL_I_MODE << LPC178X_EMC_DYCTRL_I_BITS);
	tmp32 = *(volatile u32 *)(CONFIG_SYS_RAM_BASE |
		(LPC178X_EMC_MODEREG_VALUE << LPC178X_EMC_MODEREG_ADDR_SHIFT));

	/*
	 * Normal mode
	 */
	LPC178X_EMC->dy_ctrl =
		(LPC178X_EMC_DYCTRL_I_NORMAL << LPC178X_EMC_DYCTRL_I_BITS);

	/*
	 * Enable DRAM buffer
	 */
	dy->cfg = (LPC178X_EMC_AM << LPC178X_EMC_DYCFG_AM_BITS) |
		LPC178X_EMC_DYCFG_B_MSK;

	/*
	 * Fill in global info with description of DRAM configuration
	 */
	gd->bd->bi_dram[0].start = CONFIG_SYS_RAM_BASE;
	gd->bd->bi_dram[0].size  = CONFIG_SYS_RAM_SIZE;

	return 0;
}

#ifdef CONFIG_LPC178X_ETH
/*
 * Register ethernet driver
 */
int board_eth_init(bd_t *bis)
{
	return lpc178x_eth_driver_init(bis);
}
#endif

#ifdef CONFIG_FLASH_CFI_LEGACY
ulong board_flash_get_legacy (ulong base, int banknum, flash_info_t *info)
{
#if 0
	if (banknum == 0) {	/* non-CFI flash */
		info->portwidth = FLASH_CFI_16BIT;
		info->chipwidth = FLASH_CFI_BY16;
		//info->interface = FLASH_CFI_X16;
		info->interface = FLASH_CFI_X16X32;
		return 1;
	} else
		return 0;
#else
		info->portwidth = FLASH_CFI_16BIT;
		info->chipwidth = FLASH_CFI_BY16;
		info->interface = FLASH_CFI_X16X32;
		return 0;
#endif
}
#endif

#define PWM_CLK	1000000
#define PWM_TOTCNT	1000
#define PWM_LOWCNT_START	600
void pwm_init(){
	
	long pclk;	
	int pwm_div;	

	pclk = clock_get(CLOCK_PCLK);
	pwm_div = pclk/PWM_CLK - 1;

	lpc178x_periph_enable(LPC178X_SCC_PCONP_PWM1_MSK, 1);

	LPC178X_PWM1->pwm_ir = 0x73f; //clear all pending
	LPC178X_PWM1->pwm_tcr = 0x0;
	LPC178X_PWM1->pwm_ctcr = 0x0;
	LPC178X_PWM1->pwm_mcr = 0x0;
	LPC178X_PWM1->pwm_ccr = 0x0;
	LPC178X_PWM1->pwm_pcr = 0x0;
	LPC178X_PWM1->pwm_ler = 0x0;

	LPC178X_PWM1->pwm_pr = pwm_div;
	LPC178X_PWM1->pwm_mr0 = PWM_TOTCNT;
	LPC178X_PWM1->pwm_ler |= 0x1<<0; //update mr0
	//LPC178X_PWM1->pwm_tcr |= 0x1<<1; //pwm counter reset
	//LPC178X_PWM1->pwm_tcr &= 0x9; //

	LPC178X_PWM1->pwm_mr1 = PWM_LOWCNT_START;
	LPC178X_PWM1->pwm_ler |= 0x1<<1; //update mr1
	//LPC178X_PWM1->pwm_tcr |= 0x1<<1; //pwm counter reset
	//LPC178X_PWM1->pwm_tcr &= 0x9; //
	
	LPC178X_PWM1->pwm_mcr = 0x1<<1; //PWMTC reset when PWMTC == PWMMR0
	LPC178X_PWM1->pwm_pcr |= 0x1<<9; //pwm1[1] output enable
	LPC178X_PWM1->pwm_tcr |= 0x1<<1; //pwm counter reset
	LPC178X_PWM1->pwm_tcr &= 0x9; //
	
	LPC178X_PWM1->pwm_tcr |= 0x1<<0; //pwm counter enable
	LPC178X_PWM1->pwm_tcr |= 0x1<<3; //pwm enable

}


#define C_GLCD_PWR_ENA_DIA_DLY	10000

void GLCD_Ctrl(unsigned int bEna){
	volatile unsigned long i;	

	if(bEna){
		LPC178X_LCD->lcd_ctrl |= (1<<0);
		for(i=C_GLCD_PWR_ENA_DIA_DLY; i; i--);
		LPC178X_LCD->lcd_ctrl |= (1<<11);
	}else{
		LPC178X_LCD->lcd_ctrl &= ~(1<<11);
		for(i=C_GLCD_PWR_ENA_DIA_DLY; i; i--);
		LPC178X_LCD->lcd_ctrl &= ~(1<<0);
	}
}


void board_video_init(GraphicDevice *pGD){
	
	long pclk;
	int lcd_div;

	pclk = clock_get(CLOCK_SYSTICK);
	lcd_div = pclk / (pGD->modeIdent[0] * 1000000);

	printf("lcd_div = %d\n", lcd_div);

	lpc178x_periph_enable(LPC178X_SCC_PCONP_LCD_MSK, 1);
	
	GLCD_Ctrl(0);

	LPC178X_LCD->crsr_ctrl &= ~(0x1<<0); //不使用光标
	LPC178X_LCD->lcd_ctrl = 0x0;
	LPC178X_LCD->lcd_ctrl |= (0x6<<1);   //16bpp, 5:6:5 mode
	LPC178X_LCD->lcd_ctrl |= (0x1<<5);   // TFT panel
	LPC178X_LCD->lcd_ctrl &= ~(0x1<<7);  // single panel
	LPC178X_LCD->lcd_ctrl &= ~(0x1<<8);  // RGB normal sequence
	LPC178X_LCD->lcd_ctrl &= ~(0x1<<9);  // little order
	LPC178X_LCD->lcd_ctrl &= ~(0x1<<10); // little order in one byte
	LPC178X_LCD->lcd_ctrl &= ~(0x1<<11); // disable LCD_VD[0:23]

	if(lcd_div > 0)
		lcd_div -= 1;
	else
		lcd_div = 0;

	LPC178X_SCC->lcd_cfg = lcd_div;

	LPC178X_LCD->lcd_pol |= (1<<26); // bypass inrenal clk divider
	LPC178X_LCD->lcd_pol &= ~(1<<5); // clock source for LCD is CCLK
	LPC178X_LCD->lcd_pol |= (1<<11); // LCDFP pin is active Low and inactive HIGH
	LPC178X_LCD->lcd_pol |= (1<<12); // LCDLP pin is active Low and inactive HIGH
	LPC178X_LCD->lcd_pol |= (1<<13); // data is driven out into the LCD on the falling edge

	//active high
	LPC178X_LCD->lcd_pol &= ~(1<<14); // LCD_ENAB_M is active high
	LPC178X_LCD->lcd_pol &= ~(0x3ff<<16);
	LPC178X_LCD->lcd_pol |= ((pGD->winSizeX)-1)<<16; //pixel per line 
	
	// init Horizontal Timing
	LPC178X_LCD->lcd_timh = 0; 
	LPC178X_LCD->lcd_timh |= ((pGD->modeIdent[1]) - 1)<<24; 
	LPC178X_LCD->lcd_timh |= ((pGD->modeIdent[2]) - 1)<<16; 
	LPC178X_LCD->lcd_timh |= ((pGD->modeIdent[5]) - 1)<<8; 
	LPC178X_LCD->lcd_timh |= ((pGD->winSizeX)/16 - 1)<<2; 
	
	// init Vertical Timing
	LPC178X_LCD->lcd_timv = 0; 
	LPC178X_LCD->lcd_timv |= (pGD->modeIdent[3])<<24; 
	LPC178X_LCD->lcd_timv |= (pGD->modeIdent[4])<<16; 
	LPC178X_LCD->lcd_timv |= ((pGD->modeIdent[6]) - 1)<<10; 
	LPC178X_LCD->lcd_timv |= (pGD->winSizeY) - 1; 
	
	// init lcd base addr	
	LPC178X_LCD->lcd_upbase = pGD->frameAdrs; 
	LPC178X_LCD->lcd_lpbase = pGD->frameAdrs; 

	GLCD_Ctrl(1);

	pwm_init();
}

