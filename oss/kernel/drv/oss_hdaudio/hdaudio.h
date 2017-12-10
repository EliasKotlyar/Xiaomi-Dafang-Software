/*
 * Purpose: Common definitions for the hdaudio driver files
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
#define HDA_GCAP	        0x00	/* Global Capabilities  */
#define HDA_VMIN 	        0x02	/* Minor Version */
#define HDA_VMAJ	        0x03	/* Major Version */
#define HDA_OUTPAY		0x04	/* Output Payload Capability */
#define HDA_INPAY 		0x06	/* Input Payload Capability */
#define HDA_GCTL	        0x08	/* Global Control */
#	define CRST			0x00000001	/* Controller reset */
#define HDA_WAKEEN 		0x0C	/* Wake Enable */
#define HDA_STATESTS		0x0E	/* Wake Status */
#define HDA_GSTST	        0x10	/* Global Status */
#define HDA_INTCTL 		0x20	/* Interrupt Control */
#define HDA_INTSTS 		0x24	/* Interrupt Status */
#define HDA_WCCNT	        0x30	/* Wall Clock Counter */
#define HDA_SSYNC	        0x38	/* Stream Synchronization */
#define HDA_CORBLBASE 		0x40	/* CORB Lower Base Address */
#define HDA_CORBUBASE		0x44	/* CORB Upper Base Address */
#define HDA_CORBWP 		0x48	/* CORB Write Pointer */
#define HDA_CORBRP 		0x4A	/* CORB Read Pointer */
#define HDA_CORBCTL		0x4C	/* CORB Control */
#define HDA_CORBSTS		0x4D	/* CORB Status */
#define HDA_CORBSIZE		0x4E	/* CORB Size */
#define HDA_RIRBLBASE 		0x50	/* RIRB Lower Base Address */
#define HDA_RIRBUBASE 		0x54	/* RIRB Upper Base Address */
#define HDA_RIRBWP		0x58	/* RIRB Write Pointer */
#define HDA_RINTCNT 		0x5A	/* Response Interrupt Count */
#define HDA_RIRBCTL 		0x5C	/* RIRB Control */
#define HDA_RIRBSTS 		0x5D	/* RIRB Status */
#define HDA_RIRBSIZE 		0x5E	/* RIRB Size */
#define HDA_IC		        0x60	/* Immediate Command Output Interface */
#define HDA_IR		        0x64	/* Immediate Command Input Interface */
#define HDA_IRS 		0x68	/* Immediate Command Status */
#define HDA_DPLBASE 		0x70	/* DMA Position Lower Base Address */
#define HDA_DPUBASE 		0x74	/* DMA Position Upper Base Address */

#define HDA_SD_CTL		0x0
#define HDA_SD_STS		0x3
#define HDA_SD_LPIB		0x4
#define HDA_SD_CBL		0x8
#define HDA_SD_LVI		0xC
#define HDA_SD_FIFOSIZE		0x10
#define HDA_SD_FORMAT		0x12
#define HDA_SD_BDLPL		0x18
#define HDA_SD_BDLPU		0x1C
#define HDA_SD_LPIBA		0x2004

#define HDA_SDI0CTL		0x80	/* Stream Descriptor Control */
#define HDA_SDI0STS 		0x83	/* Stream Descriptor Status */
#define HDA_SDI0LPIB 		0x84	/* Link Position in Current Buffer */
#define HDA_SDI0CBL		0x88	/* Cyclic Buffer Length */
#define HDA_SDI0LVI		0x8C	/* Last Valid Index */
#define HDA_SDI0FIFOSIZE        0x90	/* FIFO Size */
#define HDA_SDI0FORMAT		0x92	/* Format */
#define HDA_SDI0BDLPL		0x98	/* List Pointer - Lower */
#define HDA_SDI0BDLPU		0x9C	/* List Pointer - Upper */
#define HDA_SDI0LPIBA		0x2084	/* Link Position in Buffer n Alias */

#define HDA_SDI1CTL		0xA0	/* Stream Descriptor Control */
#define HDA_SDI1STS 		0xA3	/* Stream Descriptor Status */
#define HDA_SDI1LPIB 		0xA4	/* Link Position in Current Buffer */
#define HDA_SDI1CBL		0xA8	/* Cyclic Buffer Length */
#define HDA_SDI1LVI		0xAC	/* Last Valid Index */
#define HDA_SDI1FIFOSIZE        0xB0	/* FIFO Size */
#define HDA_SDI1FORMAT		0xB2	/* Format */
#define HDA_SDI1BDLPL		0xB8	/* List Pointer - Lower */
#define HDA_SDI1BDLPU		0xBC	/* List Pointer - Upper */
#define HDA_SDI1LPIBA		0x20A4	/* Link Position in Buffer n Alias */

#define HDA_SDI2CTL		0xC0	/* Stream Descriptor Control */
#define HDA_SDI2STS 		0xC3	/* Stream Descriptor Status */
#define HDA_SDI2LPIB 		0xC4	/* Link Position in Current Buffer */
#define HDA_SDI2CBL		0xC8	/* Cyclic Buffer Length */
#define HDA_SDI2LVI		0xCC	/* Last Valid Index */
#define HDA_SDI2FIFOSIZ		0xD0	/* FIFO Size */
#define HDA_SDI2FORMAT		0xD2	/* Format */
#define HDA_SDI2BDLPL		0xD8	/* List Pointer - Lower */
#define HDA_SDI2BDLPU		0xDC	/* List Pointer - Upper */
#define HDA_SDI2LPIBA		0x20D4	/* Link Position in Buffer n Alias */

#define HDA_SDI3CTL		0xE0	/* Stream Descriptor Control */
#define HDA_SDI3STS 		0xE3	/* Stream Descriptor Status */
#define HDA_SDI3LPIB 		0xE4	/* Link Position in Current Buffer */
#define HDA_SDI3CBL		0xE8	/* Cyclic Buffer Length */
#define HDA_SDI3LVI		0xEC	/* Last Valid Index */
#define HDA_SDI3FIFOSIZE        0xF0	/* FIFO Size */
#define HDA_SDI3FORMAT		0xF2	/* Format */
#define HDA_SDI3BDLPL		0xF8	/* List Pointer - Lower */
#define HDA_SDI3BDLPU		0xFC	/* List Pointer - Upper */
#define HDA_SDI3LPIBA		0x20E4	/* Link Position in Buffer n Alias */

#define HDA_SDO0CTL		0x100	/* Stream Descriptor Control */
#define HDA_SDO0STS 		0x103	/* Stream Descriptor Status */
#define HDA_SDO0LPIB 		0x104	/* Link Position in Current Buffer */
#define HDA_SDO0CBL		0x108	/* Cyclic Buffer Length */
#define HDA_SDO0LVI		0x10C	/* Last Valid Index */
#define HDA_SDO0FIFOSIZE        0x110	/* FIFO Size */
#define HDA_SDO0FORMAT		0x112	/* Format */
#define HDA_SDO0BDLPL		0x118	/* List Pointer - Lower */
#define HDA_SDO0BDLPU		0x11C	/* List Pointer - Upper */
#define HDA_SDO0LPIBA		0x2104	/* Link Position in Buffer n Alias */

#define HDA_SDO1CTL		0x120	/* Stream Descriptor Control */
#define HDA_SDO1STS 		0x123	/* Stream Descriptor Status */
#define HDA_SDO1LPIB 		0x124	/* Link Position in Current Buffer */
#define HDA_SDO1CBL		0x128	/* Cyclic Buffer Length */
#define HDA_SDO1LVI		0x12C	/* Last Valid Index */
#define HDA_SDO1FIFOSIZE        0x130	/* FIFO Size */
#define HDA_SDO1FORMAT		0x132	/* Format */
#define HDA_SDO1BDLPL		0x138	/* List Pointer - Lower */
#define HDA_SDO1BDLPU		0x13C	/* List Pointer - Upper */
#define HDA_SDO1LPIBA		0x2124	/* Link Position in Buffer n Alias */

#define HDA_SDO2CTL		0x140	/* Stream Descriptor Control */
#define HDA_SDO2STS 		0x143	/* Stream Descriptor Status */
#define HDA_SDO2LPIB 		0x144	/* Link Position in Current Buffer */
#define HDA_SDO2CBL		0x148	/* Cyclic Buffer Length */
#define HDA_SDO2LVI		0x14C	/* Last Valid Index */
#define HDA_SDO2FIFOSIZE        0x150	/* FIFO Size */
#define HDA_SDO2FORMAT		0x152	/* Format */
#define HDA_SDO2BDLPL		0x158	/* List Pointer - Lower */
#define HDA_SDO2BDLPU		0x15C	/* List Pointer - Upper */
#define HDA_SDO2LPIBA		0x2144	/* Link Position in Buffer n Alias */

#define HDA_SDO3CTL		0x160	/* Stream Descriptor Control */
#define HDA_SDO3STS 		0x163	/* Stream Descriptor Status */
#define HDA_SDO3LPIB 		0x164	/* Link Position in Current Buffer */
#define HDA_SDO3CBL		0x168	/* Cyclic Buffer Length */
#define HDA_SDO3LVI		0x16C	/* Last Valid Index */
#define HDA_SDO3FIFOSIZE        0x170	/* FIFO Size */
#define HDA_SDO3FORMAT		0x172	/* Format */
#define HDA_SDO3BDLPL		0x178	/* List Pointer - Lower */
#define HDA_SDO3BDLPU		0x17C	/* List Pointer - Upper */

#define HWINFO_SIZE	256

/*
 * Debugging ioctl calls
 */

typedef struct
{
  int cad, wid;
  char name[32];
} hda_name_t;

typedef struct
{
  int cad, wid;
  char info[4000];
} hda_widget_info_t;

#define HDA_IOCTL_WRITE		__SIOWR('H', 0, int)
#define HDA_IOCTL_READ		__SIOWR('H', 1, int)
#define HDA_IOCTL_NAME		__SIOWR('H', 2, hda_name_t)
#define HDA_IOCTL_WIDGET	__SIOWR('H', 3, hda_widget_info_t)
