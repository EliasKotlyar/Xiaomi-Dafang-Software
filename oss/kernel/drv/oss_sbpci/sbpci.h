/*
 * Purpose: Definitions for the Creative/Ensoniq AudioPCI97 driver.
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */

#ifndef ES1371_H
#define ES1371_H

/* CONCERT PCI-SIG defines */
#define CONC_PCI_VENDID     0x1274U
#define CONC_PCI_DEVID      0x1371U


/* Concert97 direct register offset defines */
#define CONC_bDEVCTL_OFF    0x00	/* Device control/enable */
#define CONC_bMISCCTL_OFF   0x01	/* Miscellaneous control */
#define CONC_bGPIO_OFF      0x02	/* General purpose I/O control */
#define CONC_bJOYCTL_OFF    0x03	/* Joystick control (decode) */
#define CONC_dSTATUS_OFF    0x04	/* long status register */
#define CONC_bINTSTAT_OFF   0x04	/* Device interrupt status */
#define CONC_bCODECSTAT_OFF 0x05	/* CODEC interface status */
#define CONC_bINTSUMM_OFF   0x07	/* Interrupt summary status */
#define CONC_bUARTDATA_OFF  0x08	/* UART data R/W - read clears RX int */
#define CONC_bUARTCSTAT_OFF 0x09	/* UART control and status */
#define CONC_bUARTTEST_OFF  0x0a	/* UART test control reg */
#define CONC_bMEMPAGE_OFF   0x0c	/* Memory page select */
#define CONC_dSRCIO_OFF     0x10	/* I/O ctl/stat/data for SRC RAM */
#define CONC_dCODECCTL_OFF  0x14	/* CODEC control - dword read/write */
#define CONC_wNMISTAT_OFF   0x18	/* Legacy NMI status */
#define CONC_bNMIENA_OFF    0x1a	/* Legacy NMI enable */
#define CONC_bNMICTL_OFF    0x1b	/* Legacy control */
#define CONC_bSERFMT_OFF    0x20	/* Serial device control */
#define CONC_bSERCTL_OFF    0x21	/* Serial device format */
#define CONC_bSKIPC_OFF     0x22	/* DAC skip count reg */
#define CONC_wSYNIC_OFF     0x24	/* Synth int count in sample frames */
#define CONC_wSYNCIC_OFF    0x26	/* Synth current int count */
#define CONC_wDACIC_OFF     0x28	/* DAC int count in sample frames */
#define CONC_wDACCIC_OFF    0x2a	/* DAC current int count */
#define CONC_wADCIC_OFF     0x2c	/* ADC int count in sample frames */
#define CONC_wADCCIC_OFF    0x2e	/* ADC current int count */
#define CONC_MEMBASE_OFF    0x30	/* Memory window base - 16 byte window */

/* Concert memory page-banked register offset defines */
#define CONC_dSYNPADDR_OFF  0x30	/* Synth host frame PCI phys addr */
#define CONC_wSYNFC_OFF     0x34	/* Synth host frame count in DWORDS */
#define CONC_wSYNCFC_OFF    0x36	/* Synth host current frame count */
#define CONC_dDACPADDR_OFF  0x38	/* DAC host frame PCI phys addr */
#define CONC_wDACFC_OFF     0x3c	/* DAC host frame count in DWORDS */
#define CONC_wDACCFC_OFF    0x3e	/* DAC host current frame count */
#define CONC_dADCPADDR_OFF  0x30	/* ADC host frame PCI phys addr */
#define CONC_wADCFC_OFF     0x34	/* ADC host frame count in DWORDS */
#define CONC_wADCCFC_OFF    0x36	/* ADC host current frame count */

/* Concert memory page number defines */
#define CONC_SYNRAM_PAGE    0x00	/* Synth host/serial I/F RAM */
#define CONC_DACRAM_PAGE    0x04	/* DAC host/serial I/F RAM */
#define CONC_ADCRAM_PAGE    0x08	/* ADC host/serial I/F RAM */
#define CONC_SYNCTL_PAGE    0x0c	/* Page bank for synth host control */
#define CONC_DACCTL_PAGE    0x0c	/* Page bank for DAC host control */
#define CONC_ADCCTL_PAGE    0x0d	/* Page bank for ADC host control */
#define CONC_FIFO0_PAGE     0x0e	/* page 0 of UART "FIFO" (rx stash) */
#define CONC_FIFO1_PAGE     0x0f	/* page 1 of UART "FIFO" (rx stash) */

/* PCM format defines */
#define CONC_PCM_DAC_STEREO 0x04
#define CONC_PCM_DAC_16BIT  0x08
#define CONC_PCM_DAC_MASK   0xf3
#define CONC_PCM_ADC_STEREO 0x10
#define CONC_PCM_ADC_16BIT  0x20
#define CONC_PCM_ADC_MASK   0xcf

/* Device Control defines */
#define CONC_DEVCTL_PCICLK_DS    0x01	/* PCI Clock Disable */
#define CONC_DEVCTL_XTALCLK_DS   0x02	/* Crystal Clock Disable */
#define CONC_DEVCTL_JSTICK_EN    0x04	/* Joystick Enable */
#define CONC_DEVCTL_UART_EN      0x08	/* UART Enable  */
#define CONC_DEVCTL_ADC_EN       0x10	/* ADC Enable (record) */
#define CONC_DEVCTL_DAC2_EN      0x20	/* DAC2 Enable (playback) */
#define CONC_DEVCTL_DAC1_EN      0x40	/* DAC1 Enabale (synth) */

/* Misc Control defines */
#define CONC_MISCCTL_PDLEV_D0      0x00	/* These bits reflect the */
#define CONC_MISCCTL_PDLEV_D1      0x01	/* power down state of  */
#define CONC_MISCCTL_PDLEV_D2      0x02	/* the part */
#define CONC_MISCCTL_PDLEV_D3      0x03	/* */
#define CONC_MISCCTL_CCBINTRM_EN   0x04	/* CCB module interrupt mask */

#define CONC_MISCCTL_SYNC_RES      0x40	/* for AC97 warm reset */

/* Serial Control defines */
#define CONC_SERCTL_DAC1IE    0x01	/* playback interrupt enable P1_INT_EN */
#define CONC_SERCTL_DAC2IE    0x02	/* playback interrupt enable P2_INT_EN */
#define CONC_SERCTL_ADCIE     0x04	/* record interrupt enable   R1_INT_EN    */
#define CONC_SERCTL_DACPAUSE  0x10	/* playback pause */
#define CONC_SERCTL_R1LOOP    0x80
#define CONC_SERCTL_P2LOOP    0x40
#define CONC_SERCTL_P1LOOP    0x20

/* Interrupt Status defines */
#define CONC_INTSTAT_ADCINT  0x01	/* A/D interrupt pending bit */
#define CONC_INTSTAT_DAC2INT 0x02	/* DAC2 interrupt pending bit */
#define CONC_INTSTAT_DAC1INT 0x04	/* DAC1 interrupt pending bit */
#define CONC_INTSTAT_UARTINT 0x08	/* UART interrupt pending bit */
#define CONC_INTSTAT_PENDING 0x80000000	/* this bit set high while'st we have an interrupt */
/* DEVCTL register masks */
/*#define CONC_DEVCTL_D1EN    0x40 */
/*#define CONC_DEVCTL_D2EN    0x20 */
/*#define CONC_DEVCTL_ADEN    0x10 */

/* SERCTL register masks */
/*#define CONC_SERCTL_P1_INT_EN 0x01 */
/*#define CONC_SERCTL_P2_INT_EN 0x02 */
/*#define CONC_SERCTL_R1_INT_EN 0x04 */

/* JOYCTL register defines */
#define CONC_JOYCTL_200    0x00
#define CONC_JOYCTL_208    0x01
#define CONC_JOYCTL_210    0x02
#define CONC_JOYCTL_218    0x03


/* UARTCSTAT register masks  */
#define CONC_UART_RXRDY      0x01
#define CONC_UART_TXRDY      0x02
#define CONC_UART_TXINT      0x04
#define CONC_UART_RXINT      0x80

#define CONC_UART_CTL        0x03
#define CONC_UART_TXINTEN    0x20
#define CONC_UART_RXINTEN    0x80

/* Logical index for each DMA controller on chip - used for */
/* generic routines that access all DMA controllers */
#define CONC_SYNTH_DAC      0
#define CONC_WAVE_DAC       1
#define CONC_WAVE_ADC       2

/* defines for the CONCERT97 Sample Rate Converters */

/* register/base equates for the SRC RAM */
#define SRC_SYNTH_FIFO      0x00
#define SRC_DAC_FIFO        0x20
#define SRC_ADC_FIFO        0x40
#define SRC_ADC_VOL_L       0x6c
#define SRC_ADC_VOL_R       0x6d
#define SRC_SYNTH_BASE      0x70
#define SRC_DAC_BASE        0x74
#define SRC_ADC_BASE        0x78
#define SRC_SYNTH_VOL_L     0x7c
#define SRC_SYNTH_VOL_R     0x7d
#define SRC_DAC_VOL_L       0x7e
#define SRC_DAC_VOL_R       0x7f

#define SRC_TRUNC_N_OFF     0x00
#define SRC_INT_REGS_OFF    0x01
#define SRC_ACCUM_FRAC_OFF  0x02
#define SRC_VFREQ_FRAC_OFF  0x03


/* miscellaneous control defines */
/*#define SRC_IOPOLL_COUNT    0x1000UL */
#define SRC_IOPOLL_COUNT    0x20000UL
#define SRC_WENABLE         (1UL << 24)
#define SRC_BUSY            (1UL << 23)
#define SRC_DISABLE         (1UL << 22)
#define SRC_SYNTHFREEZE     (1UL << 21)
#define SRC_DACFREEZE       (1UL << 20)
#define SRC_ADCFREEZE       (1UL << 19)
#define SRC_CTLMASK         0x00780000UL

#endif /* ES1371_H */
