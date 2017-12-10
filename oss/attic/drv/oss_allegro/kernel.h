/******************************************************************************
 *                                                                            *
 *                       (C) 1997-1999 ESS Technology, Inc.                   *
 *                                                                            *
 * This source code, its compiled object code, and its associated data sets   *
 * are copyright (C) 1997-1999 ESS Technology, Inc. This source code and its  *
 * associated data sets are trade secrets of ESS Technology, Inc.             *
 *                                                                            *
 ******************************************************************************/

/*---------------------------------------------------------------------------
 *              Copyright (C) 1997-1999, ESS Technology, Inc.
 *---------------------------------------------------------------------------
 * FILENAME: kernel.h V2.10  08/19/99
 *---------------------------------------------------------------------------
 * DESCRIPTION: Header file for Maestro 3/Allegro host kernel
 *---------------------------------------------------------------------------
 * AUTHOR:  Henry Tang / Hong Kim /Alger Yeung/Don Kim
 *---------------------------------------------------------------------------
 * HISTORY:
 *    09/03/97  HT  Created.
 *    05/21/99  AY  SwitchClient flags
 *    07/29/99  AY  Adding 4-speaker support
 *    08/18/99  AY  Adding SPDIF IN  support
 *    08/18/99  AY  Adding SPDIF IN  support
 *    08/18/99  AY  Remove PIO and SoundBlaster support
 *    08/18/99  AY  Reduce Cpythru to 2 instances instead of 4
 *	  09/22/99  HK  M3I feature
 *---------------------------------------------------------------------------
 */


#ifndef __KERNEL_H
#define __KERNEL_H

/* */
/* client IDs */
/* */
/* FM client is a special case */
/* */

#define CLIENT_CPYTHRU                  0
#define CLIENT_MODEM                    1
#define CLIENT_POS3D                    2
#define CLIENT_SPKVIRT                  3
#define CLIENT_SPKVIRT_CRL              4
#define CLIENT_SRC                      5
#define CLIENT_MINISRC                  6
#define CLIENT_SPDIF                    7
#define NUMBER_OF_CLIENTS               (CLIENT_SPDIF + 1)

#define CLIENT_FM                       NUMBER_OF_CLIENTS

#define MASK_CLIENT_CPYTHRU             (1 << CLIENT_CPYTHRU    )
#define MASK_CLIENT_MODEM               (1 << CLIENT_MODEM      )
#define MASK_CLIENT_POS3D               (1 << CLIENT_POS3D      )
#define MASK_CLIENT_SPKVIRT             (1 << CLIENT_SPKVIRT    )
#define MASK_CLIENT_SPKVIRT_CRL         (1 << CLIENT_SPKVIRT_CRL)
#define MASK_CLIENT_SRC                 (1 << CLIENT_SRC        )
#define MASK_CLIENT_MINISRC             (1 << CLIENT_MINISRC    )
#define MASK_CLIENT_SPDIF               (1 << CLIENT_SPDIF      )

/* WARNING! DANGER! WARNING! DANGER! WARNING! DANGER! WARNING! DANGER! */
/* */
/* If you modify any memory map and/or definitions below be sure to reflect */
/* the changes in the DSP version found in KERNEL.INC. */
/* */
/* WARNING! DANGER! WARNING! DANGER! WARNING! DANGER! WARNING! DANGER! */


/* */
/* DSP memory map */
/* */

#define REV_A_CODE_MEMORY_BEGIN         0x0000
#define REV_A_CODE_MEMORY_END           0x0FFF
#define REV_A_CODE_MEMORY_UNIT_LENGTH   0x0040
#define REV_A_CODE_MEMORY_LENGTH        (REV_A_CODE_MEMORY_END - REV_A_CODE_MEMORY_BEGIN + 1)

#define REV_B_CODE_MEMORY_BEGIN         0x0000
#define REV_B_CODE_MEMORY_END           0x0BFF
#define REV_B_CODE_MEMORY_UNIT_LENGTH   0x0040
#define REV_B_CODE_MEMORY_LENGTH        (REV_B_CODE_MEMORY_END - REV_B_CODE_MEMORY_BEGIN + 1)

#if (REV_A_CODE_MEMORY_LENGTH % REV_A_CODE_MEMORY_UNIT_LENGTH)
#error Assumption about code memory unit length failed.
#endif
#if (REV_B_CODE_MEMORY_LENGTH % REV_B_CODE_MEMORY_UNIT_LENGTH)
#error Assumption about code memory unit length failed.
#endif

#define REV_A_DATA_MEMORY_BEGIN         0x1000
#define REV_A_DATA_MEMORY_END           0x2FFF
#define REV_A_DATA_MEMORY_UNIT_LENGTH   0x0080
#define REV_A_DATA_MEMORY_LENGTH        (REV_A_DATA_MEMORY_END - REV_A_DATA_MEMORY_BEGIN + 1)

#define REV_B_DATA_MEMORY_BEGIN         0x1000
/*#define REV_B_DATA_MEMORY_END           0x23FF */
#define REV_B_DATA_MEMORY_END           0x2BFF
#define REV_B_DATA_MEMORY_UNIT_LENGTH   0x0080
#define REV_B_DATA_MEMORY_LENGTH        (REV_B_DATA_MEMORY_END - REV_B_DATA_MEMORY_BEGIN + 1)

#if (REV_A_DATA_MEMORY_LENGTH % REV_A_DATA_MEMORY_UNIT_LENGTH)
#error Assumption about data memory unit length failed.
#endif
#if (REV_B_DATA_MEMORY_LENGTH % REV_B_DATA_MEMORY_UNIT_LENGTH)
#error Assumption about data memory unit length failed.
#endif

#define CODE_MEMORY_MAP_LENGTH          (64 + 1)
#define DATA_MEMORY_MAP_LENGTH          (64 + 1)

#if (CODE_MEMORY_MAP_LENGTH < ((REV_A_CODE_MEMORY_LENGTH / REV_A_CODE_MEMORY_UNIT_LENGTH) + 1))
#error Code memory map length too short.
#endif
#if (DATA_MEMORY_MAP_LENGTH < ((REV_A_DATA_MEMORY_LENGTH / REV_A_DATA_MEMORY_UNIT_LENGTH) + 1))
#error Data memory map length too short.
#endif
#if (CODE_MEMORY_MAP_LENGTH < ((REV_B_CODE_MEMORY_LENGTH / REV_B_CODE_MEMORY_UNIT_LENGTH) + 1))
#error Code memory map length too short.
#endif
#if (DATA_MEMORY_MAP_LENGTH < ((REV_B_DATA_MEMORY_LENGTH / REV_B_DATA_MEMORY_UNIT_LENGTH) + 1))
#error Data memory map length too short.
#endif


/* */
/* Kernel code memory definition */
/* */

#define KCODE_VECTORS_BEGIN             0x0000
#define KCODE_VECTORS_END               0x002F
#define KCODE_VECTORS_UNIT_LENGTH       0x0002
#define KCODE_VECTORS_LENGTH            (KCODE_VECTORS_END - KCODE_VECTORS_BEGIN + 1)


/* */
/* Kernel data memory definition */
/* */

#define KDATA_BASE_ADDR                 0x1000
#define KDATA_BASE_ADDR2                0x1080

#define KDATA_TASK0                     (KDATA_BASE_ADDR + 0x0000)
#define KDATA_TASK1                     (KDATA_BASE_ADDR + 0x0001)
#define KDATA_TASK2                     (KDATA_BASE_ADDR + 0x0002)
#define KDATA_TASK3                     (KDATA_BASE_ADDR + 0x0003)
#define KDATA_TASK4                     (KDATA_BASE_ADDR + 0x0004)
#define KDATA_TASK5                     (KDATA_BASE_ADDR + 0x0005)
#define KDATA_TASK6                     (KDATA_BASE_ADDR + 0x0006)
#define KDATA_TASK7                     (KDATA_BASE_ADDR + 0x0007)
#define KDATA_TASK_ENDMARK              (KDATA_BASE_ADDR + 0x0008)

#define KDATA_CURRENT_TASK              (KDATA_BASE_ADDR + 0x0009)
#define KDATA_TASK_SWITCH               (KDATA_BASE_ADDR + 0x000A)

#define KDATA_INSTANCE0_POS3D           (KDATA_BASE_ADDR + 0x000B)
#define KDATA_INSTANCE1_POS3D           (KDATA_BASE_ADDR + 0x000C)
#define KDATA_INSTANCE2_POS3D           (KDATA_BASE_ADDR + 0x000D)
#define KDATA_INSTANCE3_POS3D           (KDATA_BASE_ADDR + 0x000E)
#define KDATA_INSTANCE4_POS3D           (KDATA_BASE_ADDR + 0x000F)
#define KDATA_INSTANCE5_POS3D           (KDATA_BASE_ADDR + 0x0010)
#define KDATA_INSTANCE6_POS3D           (KDATA_BASE_ADDR + 0x0011)
#define KDATA_INSTANCE7_POS3D           (KDATA_BASE_ADDR + 0x0012)
#define KDATA_INSTANCE8_POS3D           (KDATA_BASE_ADDR + 0x0013)
#define KDATA_INSTANCE_POS3D_ENDMARK    (KDATA_BASE_ADDR + 0x0014)

#define KDATA_INSTANCE0_SPKVIRT         (KDATA_BASE_ADDR + 0x0015)
#define KDATA_INSTANCE_SPKVIRT_ENDMARK  (KDATA_BASE_ADDR + 0x0016)

#define KDATA_INSTANCE0_SPDIF           (KDATA_BASE_ADDR + 0x0017)
#define KDATA_INSTANCE_SPDIF_ENDMARK    (KDATA_BASE_ADDR + 0x0018)

#define KDATA_INSTANCE0_MODEM           (KDATA_BASE_ADDR + 0x0019)
#define KDATA_INSTANCE_MODEM_ENDMARK    (KDATA_BASE_ADDR + 0x001A)

#define KDATA_INSTANCE0_SRC             (KDATA_BASE_ADDR + 0x001B)
#define KDATA_INSTANCE1_SRC             (KDATA_BASE_ADDR + 0x001C)
#define KDATA_INSTANCE_SRC_ENDMARK      (KDATA_BASE_ADDR + 0x001D)

#define KDATA_INSTANCE0_MINISRC         (KDATA_BASE_ADDR + 0x001E)
#define KDATA_INSTANCE1_MINISRC         (KDATA_BASE_ADDR + 0x001F)
#define KDATA_INSTANCE2_MINISRC         (KDATA_BASE_ADDR + 0x0020)
#define KDATA_INSTANCE3_MINISRC         (KDATA_BASE_ADDR + 0x0021)
#define KDATA_INSTANCE_MINISRC_ENDMARK  (KDATA_BASE_ADDR + 0x0022)

#define KDATA_INSTANCE0_CPYTHRU         (KDATA_BASE_ADDR + 0x0023)
#define KDATA_INSTANCE1_CPYTHRU         (KDATA_BASE_ADDR + 0x0024)
#define KDATA_INSTANCE_CPYTHRU_ENDMARK  (KDATA_BASE_ADDR + 0x0025)

#define KDATA_CURRENT_DMA               (KDATA_BASE_ADDR + 0x0026)
#define KDATA_DMA_SWITCH                (KDATA_BASE_ADDR + 0x0027)
#define KDATA_DMA_ACTIVE                (KDATA_BASE_ADDR + 0x0028)

#define KDATA_DMA_XFER0                 (KDATA_BASE_ADDR + 0x0029)
#define KDATA_DMA_XFER1                 (KDATA_BASE_ADDR + 0x002A)
#define KDATA_DMA_XFER2                 (KDATA_BASE_ADDR + 0x002B)
#define KDATA_DMA_XFER3                 (KDATA_BASE_ADDR + 0x002C)
#define KDATA_DMA_XFER4                 (KDATA_BASE_ADDR + 0x002D)
#define KDATA_DMA_XFER5                 (KDATA_BASE_ADDR + 0x002E)
#define KDATA_DMA_XFER6                 (KDATA_BASE_ADDR + 0x002F)
#define KDATA_DMA_XFER7                 (KDATA_BASE_ADDR + 0x0030)
#define KDATA_DMA_XFER8                 (KDATA_BASE_ADDR + 0x0031)
#define KDATA_DMA_XFER_ENDMARK          (KDATA_BASE_ADDR + 0x0032)

#define KDATA_I2S_SAMPLE_COUNT          (KDATA_BASE_ADDR + 0x0033)
#define KDATA_I2S_INT_METER             (KDATA_BASE_ADDR + 0x0034)
#define KDATA_I2S_ACTIVE                (KDATA_BASE_ADDR + 0x0035)

#define KDATA_TIMER_COUNT_RELOAD        (KDATA_BASE_ADDR + 0x0036)
#define KDATA_TIMER_COUNT_CURRENT       (KDATA_BASE_ADDR + 0x0037)

#define KDATA_HALT_SYNCH_CLIENT         (KDATA_BASE_ADDR + 0x0038)
#define KDATA_HALT_SYNCH_DMA            (KDATA_BASE_ADDR + 0x0039)
#define KDATA_HALT_ACKNOWLEDGE          (KDATA_BASE_ADDR + 0x003A)

#define KDATA_ADC1_XFER0                (KDATA_BASE_ADDR + 0x003B)
#define KDATA_ADC1_XFER_ENDMARK         (KDATA_BASE_ADDR + 0x003C)
#define KDATA_ADC1_LEFT_VOLUME			(KDATA_BASE_ADDR + 0x003D)
#define KDATA_ADC1_RIGHT_VOLUME  		(KDATA_BASE_ADDR + 0x003E)
#define KDATA_ADC1_LEFT_SUR_VOL			(KDATA_BASE_ADDR + 0x003F)
#define KDATA_ADC1_RIGHT_SUR_VOL		(KDATA_BASE_ADDR + 0x0040)

#define KDATA_ADC2_XFER0                (KDATA_BASE_ADDR + 0x0041)
#define KDATA_ADC2_XFER_ENDMARK         (KDATA_BASE_ADDR + 0x0042)
#define KDATA_ADC2_LEFT_VOLUME			(KDATA_BASE_ADDR + 0x0043)
#define KDATA_ADC2_RIGHT_VOLUME			(KDATA_BASE_ADDR + 0x0044)
#define KDATA_ADC2_LEFT_SUR_VOL			(KDATA_BASE_ADDR + 0x0045)
#define KDATA_ADC2_RIGHT_SUR_VOL		(KDATA_BASE_ADDR + 0x0046)

#define KDATA_CD_XFER0					(KDATA_BASE_ADDR + 0x0047)
#define KDATA_CD_XFER_ENDMARK			(KDATA_BASE_ADDR + 0x0048)
#define KDATA_CD_LEFT_VOLUME			(KDATA_BASE_ADDR + 0x0049)
#define KDATA_CD_RIGHT_VOLUME			(KDATA_BASE_ADDR + 0x004A)
#define KDATA_CD_LEFT_SUR_VOL			(KDATA_BASE_ADDR + 0x004B)
#define KDATA_CD_RIGHT_SUR_VOL			(KDATA_BASE_ADDR + 0x004C)

#define KDATA_MIC_XFER0					(KDATA_BASE_ADDR + 0x004D)
#define KDATA_MIC_XFER_ENDMARK			(KDATA_BASE_ADDR + 0x004E)
#define KDATA_MIC_VOLUME				(KDATA_BASE_ADDR + 0x004F)
#define KDATA_MIC_SUR_VOL				(KDATA_BASE_ADDR + 0x0050)

#define KDATA_I2S_XFER0                 (KDATA_BASE_ADDR + 0x0051)
#define KDATA_I2S_XFER_ENDMARK          (KDATA_BASE_ADDR + 0x0052)

#define KDATA_CHI_XFER0                 (KDATA_BASE_ADDR + 0x0053)
#define KDATA_CHI_XFER_ENDMARK          (KDATA_BASE_ADDR + 0x0054)

#define KDATA_SPDIF_XFER                (KDATA_BASE_ADDR + 0x0055)
#define KDATA_SPDIF_CURRENT_FRAME       (KDATA_BASE_ADDR + 0x0056)
#define KDATA_SPDIF_FRAME0              (KDATA_BASE_ADDR + 0x0057)
#define KDATA_SPDIF_FRAME1              (KDATA_BASE_ADDR + 0x0058)
#define KDATA_SPDIF_FRAME2              (KDATA_BASE_ADDR + 0x0059)

#define KDATA_SPDIF_REQUEST             (KDATA_BASE_ADDR + 0x005A)
#define KDATA_SPDIF_TEMP                (KDATA_BASE_ADDR + 0x005B)

/*AY SPDIF IN */
#define KDATA_SPDIFIN_XFER0             (KDATA_BASE_ADDR + 0x005C)
#define KDATA_SPDIFIN_XFER_ENDMARK      (KDATA_BASE_ADDR + 0x005D)
#define KDATA_SPDIFIN_INT_METER         (KDATA_BASE_ADDR + 0x005E)

#define KDATA_DSP_RESET_COUNT           (KDATA_BASE_ADDR + 0x005F)
#define KDATA_DEBUG_OUTPUT              (KDATA_BASE_ADDR + 0x0060)

#define KDATA_KERNEL_ISR_LIST           (KDATA_BASE_ADDR + 0x0061)

#define KDATA_KERNEL_ISR_CBSR1          (KDATA_BASE_ADDR + 0x0062)
#define KDATA_KERNEL_ISR_CBER1          (KDATA_BASE_ADDR + 0x0063)
#define KDATA_KERNEL_ISR_CBCR           (KDATA_BASE_ADDR + 0x0064)
#define KDATA_KERNEL_ISR_AR0            (KDATA_BASE_ADDR + 0x0065)
#define KDATA_KERNEL_ISR_AR1            (KDATA_BASE_ADDR + 0x0066)
#define KDATA_KERNEL_ISR_AR2            (KDATA_BASE_ADDR + 0x0067)
#define KDATA_KERNEL_ISR_AR3            (KDATA_BASE_ADDR + 0x0068)
#define KDATA_KERNEL_ISR_AR4            (KDATA_BASE_ADDR + 0x0069)
#define KDATA_KERNEL_ISR_AR5            (KDATA_BASE_ADDR + 0x006A)
#define KDATA_KERNEL_ISR_BRCR           (KDATA_BASE_ADDR + 0x006B)
#define KDATA_KERNEL_ISR_PASR           (KDATA_BASE_ADDR + 0x006C)
#define KDATA_KERNEL_ISR_PAER           (KDATA_BASE_ADDR + 0x006D)

#define KDATA_CLIENT_SCRATCH0           (KDATA_BASE_ADDR + 0x006E)
#define KDATA_CLIENT_SCRATCH1           (KDATA_BASE_ADDR + 0x006F)
#define KDATA_KERNEL_SCRATCH            (KDATA_BASE_ADDR + 0x0070)
#define KDATA_KERNEL_ISR_SCRATCH        (KDATA_BASE_ADDR + 0x0071)

#define KDATA_OUEUE_LEFT                (KDATA_BASE_ADDR + 0x0072)
#define KDATA_QUEUE_RIGHT               (KDATA_BASE_ADDR + 0x0073)

#define KDATA_ADC1_REQUEST              (KDATA_BASE_ADDR + 0x0074)
#define KDATA_ADC2_REQUEST              (KDATA_BASE_ADDR + 0x0075)
#define KDATA_CD_REQUEST				(KDATA_BASE_ADDR + 0x0076)
#define KDATA_MIC_REQUEST				(KDATA_BASE_ADDR + 0x0077)

#define KDATA_ADC1_MIXER_REQUEST        (KDATA_BASE_ADDR + 0x0078)
#define KDATA_ADC2_MIXER_REQUEST        (KDATA_BASE_ADDR + 0x0079)
#define KDATA_CD_MIXER_REQUEST			(KDATA_BASE_ADDR + 0x007A)
#define KDATA_MIC_MIXER_REQUEST			(KDATA_BASE_ADDR + 0x007B)
#define KDATA_MIC_SYNC_COUNTER			(KDATA_BASE_ADDR + 0x007C)


/* */
/* second segment */
/* */

/* smart mixer buffer */

#define KDATA_MIXER_WORD0               (KDATA_BASE_ADDR2 + 0x0000)
#define KDATA_MIXER_WORD1               (KDATA_BASE_ADDR2 + 0x0001)
#define KDATA_MIXER_WORD2               (KDATA_BASE_ADDR2 + 0x0002)
#define KDATA_MIXER_WORD3               (KDATA_BASE_ADDR2 + 0x0003)
#define KDATA_MIXER_WORD4               (KDATA_BASE_ADDR2 + 0x0004)
#define KDATA_MIXER_WORD5               (KDATA_BASE_ADDR2 + 0x0005)
#define KDATA_MIXER_WORD6               (KDATA_BASE_ADDR2 + 0x0006)
#define KDATA_MIXER_WORD7               (KDATA_BASE_ADDR2 + 0x0007)
#define KDATA_MIXER_WORD8               (KDATA_BASE_ADDR2 + 0x0008)
#define KDATA_MIXER_WORD9               (KDATA_BASE_ADDR2 + 0x0009)
#define KDATA_MIXER_WORDA               (KDATA_BASE_ADDR2 + 0x000A)
#define KDATA_MIXER_WORDB               (KDATA_BASE_ADDR2 + 0x000B)
#define KDATA_MIXER_WORDC               (KDATA_BASE_ADDR2 + 0x000C)
#define KDATA_MIXER_WORDD               (KDATA_BASE_ADDR2 + 0x000D)
#define KDATA_MIXER_WORDE               (KDATA_BASE_ADDR2 + 0x000E)
#define KDATA_MIXER_WORDF               (KDATA_BASE_ADDR2 + 0x000F)

#define KDATA_MIXER_XFER0               (KDATA_BASE_ADDR2 + 0x0010)
#define KDATA_MIXER_XFER1               (KDATA_BASE_ADDR2 + 0x0011)
#define KDATA_MIXER_XFER2               (KDATA_BASE_ADDR2 + 0x0012)
#define KDATA_MIXER_XFER3               (KDATA_BASE_ADDR2 + 0x0013)
#define KDATA_MIXER_XFER4               (KDATA_BASE_ADDR2 + 0x0014)
#define KDATA_MIXER_XFER5               (KDATA_BASE_ADDR2 + 0x0015)
#define KDATA_MIXER_XFER6               (KDATA_BASE_ADDR2 + 0x0016)
#define KDATA_MIXER_XFER7               (KDATA_BASE_ADDR2 + 0x0017)
#define KDATA_MIXER_XFER8               (KDATA_BASE_ADDR2 + 0x0018)
#define KDATA_MIXER_XFER9               (KDATA_BASE_ADDR2 + 0x0019)
#define KDATA_MIXER_XFER_ENDMARK        (KDATA_BASE_ADDR2 + 0x001A)

#define KDATA_MIXER_TASK_NUMBER         (KDATA_BASE_ADDR2 + 0x001B)
#define KDATA_CURRENT_MIXER             (KDATA_BASE_ADDR2 + 0x001C)
#define KDATA_MIXER_ACTIVE              (KDATA_BASE_ADDR2 + 0x001D)
#define KDATA_MIXER_BANK_STATUS         (KDATA_BASE_ADDR2 + 0x001E)
#define KDATA_DAC_LEFT_VOLUME	        (KDATA_BASE_ADDR2 + 0x001F)
#define KDATA_DAC_RIGHT_VOLUME          (KDATA_BASE_ADDR2 + 0x0020)

/* AY */
/* */
/* 4 speaker support */
/* */

#define KDATA_DAC2_REQUEST              (KDATA_BASE_ADDR2 + 0x0021)

#define KDATA_FMIXER_XFER0              (KDATA_BASE_ADDR2 + 0x0022)
#define KDATA_FMIXER_XFER_ENDMARK       (KDATA_BASE_ADDR2 + 0x0023)

#define KDATA_RMIXER_XFER0              (KDATA_BASE_ADDR2 + 0x0024)
#define KDATA_RMIXER_XFER_ENDMARK       (KDATA_BASE_ADDR2 + 0x0025)


#if (REV_A_DATA_MEMORY_UNIT_LENGTH - 0x0080)
#error Assumption about DATA_MEMORY_UNIT_LENGTH size failed.
#endif


/* */
/* Client data memory definition */
/* */

#define CDATA_INSTANCE_READY            0x00

#define CDATA_HOST_SRC_ADDRL            0x01
#define CDATA_HOST_SRC_ADDRH            0x02
#define CDATA_HOST_SRC_END_PLUS_1L      0x03
#define CDATA_HOST_SRC_END_PLUS_1H      0x04
#define CDATA_HOST_SRC_CURRENTL         0x05
#define CDATA_HOST_SRC_CURRENTH         0x06

#define CDATA_IN_BUF_CONNECT            0x07
#define CDATA_OUT_BUF_CONNECT           0x08

#define CDATA_IN_BUF_BEGIN              0x09
#define CDATA_IN_BUF_END_PLUS_1         0x0A
#define CDATA_IN_BUF_HEAD               0x0B
#define CDATA_IN_BUF_TAIL               0x0C

#define CDATA_OUT_BUF_BEGIN             0x0D
#define CDATA_OUT_BUF_END_PLUS_1        0x0E
#define CDATA_OUT_BUF_HEAD              0x0F
#define CDATA_OUT_BUF_TAIL              0x10

#define CDATA_DMA_CONTROL               0x11
#define CDATA_RESERVED                  0x12

#define CDATA_FREQUENCY                 0x13
#define CDATA_LEFT_VOLUME               0x14
#define CDATA_RIGHT_VOLUME              0x15
#define CDATA_LEFT_SUR_VOL              0x16
#define CDATA_RIGHT_SUR_VOL             0x17

#define CDATA_HEADER_LEN                0x18

/* */
/* DMA control definition */
/* */

#define DMACONTROL_BLOCK_MASK           0x000F
#define  DMAC_BLOCK0_SELECTOR           0x0000
#define  DMAC_BLOCK1_SELECTOR           0x0001
#define  DMAC_BLOCK2_SELECTOR           0x0002
#define  DMAC_BLOCK3_SELECTOR           0x0003
#define  DMAC_BLOCK4_SELECTOR           0x0004
#define  DMAC_BLOCK5_SELECTOR           0x0005
#define  DMAC_BLOCK6_SELECTOR           0x0006
#define  DMAC_BLOCK7_SELECTOR           0x0007
#define  DMAC_BLOCK8_SELECTOR           0x0008
#define  DMAC_BLOCK9_SELECTOR           0x0009
#define  DMAC_BLOCKA_SELECTOR           0x000A
#define  DMAC_BLOCKB_SELECTOR           0x000B
#define  DMAC_BLOCKC_SELECTOR           0x000C
#define  DMAC_BLOCKD_SELECTOR           0x000D
#define  DMAC_BLOCKE_SELECTOR           0x000E
#define  DMAC_BLOCKF_SELECTOR           0x000F
#define DMACONTROL_PAGE_MASK            0x00F0
#define  DMAC_PAGE0_SELECTOR            0x0030
#define  DMAC_PAGE1_SELECTOR            0x0020
#define  DMAC_PAGE2_SELECTOR            0x0010
#define  DMAC_PAGE3_SELECTOR            0x0000
#define DMACONTROL_AUTOREPEAT           0x1000
#define DMACONTROL_STOPPED              0x2000
#define DMACONTROL_DIRECTION            0x0100


/* */
/* Direct mixer definition */
/* */

#define DIRECTMIXER_ADC1                0x0001
#define DIRECTMIXER_ADC2                0x0002


/* */
/* DSP to Host interrupt request definition */
/* */

#define DSP2HOST_REQ_PIORECORD          0x01
#define DSP2HOST_REQ_I2SRATE            0x02
#define DSP2HOST_REQ_TIMER              0x04

/* */
/* memory check code uses this areas */
/* */

#define		FLAGADD1				0x1400	/* dsp internal data */
#define		FLAGADD2				0x1800	/* dsp internal data */
#define		FLAGADD3				0x1000	/* dsp internal data */


/* WARNING! DANGER! WARNING! DANGER! WARNING! DANGER! WARNING! DANGER! */
/* */
/* If you modify any memory map and/or definitions above be sure to reflect */
/* the changes in the DSP version found in KERNEL.INC. */
/* */
/* WARNING! DANGER! WARNING! DANGER! WARNING! DANGER! WARNING! DANGER! */


#define F_FREE                          0x00
#define F_USED                          0x01
#define F_END                           -1


/* */
/* Kernel/client memory allocation */
/* */

#define NUM_UNITS_KERNEL_CODE          16
#define NUM_UNITS_KERNEL_DATA           2

#define NUM_UNITS_KERNEL_CODE_WITH_HSP 16
#ifdef NT_MODEL
#define NUM_UNITS_KERNEL_DATA_WITH_HSP  5
#else
#define NUM_UNITS_KERNEL_DATA_WITH_HSP  4
#endif

#define NUM_UNITS( BYTES, UNITLEN )    ((((BYTES+1)>>1) + (UNITLEN-1)) / UNITLEN)


/* */
/* Maximum instances */
/* */

#define MAX_TASKS                       (KDATA_TASK_ENDMARK - KDATA_TASK0)

#define MAX_INSTANCE_CPYTHRU            (KDATA_INSTANCE_CPYTHRU_ENDMARK - KDATA_INSTANCE0_CPYTHRU)
#define MAX_INSTANCE_MODEM              (KDATA_INSTANCE_MODEM_ENDMARK - KDATA_INSTANCE0_MODEM)
#define MAX_INSTANCE_POS3D              (KDATA_INSTANCE_POS3D_ENDMARK - KDATA_INSTANCE0_POS3D)
#define MAX_INSTANCE_SPKVIRT            (KDATA_INSTANCE_SPKVIRT_ENDMARK - KDATA_INSTANCE0_SPKVIRT)
#define MAX_INSTANCE_SRC                (KDATA_INSTANCE_SRC_ENDMARK - KDATA_INSTANCE0_SRC)
#define MAX_INSTANCE_MINISRC            (KDATA_INSTANCE_MINISRC_ENDMARK - KDATA_INSTANCE0_MINISRC)
#define MAX_INSTANCE_SPDIF              (KDATA_INSTANCE_SPDIF_ENDMARK - KDATA_INSTANCE0_SPDIF)

#define MAX_VIRTUAL_DMA_CHANNELS        (KDATA_DMA_XFER_ENDMARK - KDATA_DMA_XFER0)
#define MAX_VIRTUAL_ADC1_CHANNELS       (KDATA_ADC1_XFER_ENDMARK - KDATA_ADC1_XFER0)
#define MAX_VIRTUAL_ADC2_CHANNELS       (KDATA_ADC2_XFER_ENDMARK - KDATA_ADC2_XFER0)
#define MAX_VIRTUAL_CD_CHANNELS			(KDATA_CD_XFER_ENDMARK - KDATA_CD_XFER0)
#define MAX_VIRTUAL_MIC_CHANNELS       (KDATA_MIC_XFER_ENDMARK - KDATA_MIC_XFER0)

#define MAX_VIRTUAL_I2S_CHANNELS        (KDATA_I2S_XFER_ENDMARK - KDATA_I2S_XFER0)
#define MAX_VIRTUAL_CHI_CHANNELS        (KDATA_CHI_XFER_ENDMARK - KDATA_CHI_XFER0)
#define MAX_VIRTUAL_SOUNDBLASTER_CHANNELS  (KDATA_SOUNDBLASTER_XFER_ENDMARK - KDATA_SOUNDBLASTER_XFER0)
#define MAX_VIRTUAL_SPDIFIN_CHANNELS    (KDATA_SPDIFIN_XFER_ENDMARK - KDATA_SPDIFIN_XFER0)

/*AY */
#define MAX_VIRTUAL_MIXER_CHANNELS      (KDATA_MIXER_XFER_ENDMARK - KDATA_MIXER_XFER0)
#define MAX_VIRTUAL_FMIXER_CHANNELS     (KDATA_FMIXER_XFER_ENDMARK - KDATA_FMIXER_XFER0)
#define MAX_VIRTUAL_RMIXER_CHANNELS     (KDATA_RMIXER_XFER_ENDMARK - KDATA_RMIXER_XFER0)

/* */
/* Hardware instance flags */
/* */

#define HWI_FLAG_UNLOADED               0x00000001
#define HWI_FLAG_I2S_SECONDPASS         0x00000002
#define HWI_FLAG_FM_LOADED              0x00000004
#define HWI_FLAG_SUSPENDED              0x00000008
#define HWI_FLAG_HSP_PRESENT            0x00000010
#define HWI_FLAG_MEM_CHECK				0x00000020

/* */
/* Client input/output buffer connectivity */
/* */

#define KCONNECT_NONE                   0x0000
#define KCONNECT_DMA                    0x0001
#define KCONNECT_ADC1                   0x0002
#define KCONNECT_ADC2                   0x0003
#define KCONNECT_CD		                0x0004
#define KCONNECT_MIC                    0x0005
#define KCONNECT_I2S                    0x0006
#define KCONNECT_CHI                    0x0007
#define KCONNECT_SOUNDBLASTER           0x0008
#define KCONNECT_SPDIF                  0x0009
#define KCONNECT_PIO                    0x000A
#define KCONNECT_MIXER                  0x000B
#define KCONNECT_SPDIFIN                0x000C
#define KCONNECT_FMIXER                 0x000D	/*AY */
#define KCONNECT_RMIXER                 0x000E	/*AY */
#define KCONNECT_SAME	                0x000F
#define NUMBER_OF_CONNECTIONS           (KCONNECT_SAME + 1)

#define MASK_KCONNECT_NONE              (1 << KCONNECT_NONE)
#define MASK_KCONNECT_DMA               (1 << KCONNECT_DMA)
#define MASK_KCONNECT_ADC1              (1 << KCONNECT_ADC1)
#define MASK_KCONNECT_ADC2              (1 << KCONNECT_ADC2)
#define MASK_KCONNECT_CD	            (1 << KCONNECT_CD)
#define MASK_KCONNECT_MIC               (1 << KCONNECT_MIC)
#define MASK_KCONNECT_I2S               (1 << KCONNECT_I2S)
#define MASK_KCONNECT_CHI               (1 << KCONNECT_CHI)
#define MASK_KCONNECT_SOUNDBLASTER      (1 << KCONNECT_SOUNDBLASTER)
#define MASK_KCONNECT_SPDIF             (1 << KCONNECT_SPDIF)
#define MASK_KCONNECT_MIXER             (1 << KCONNECT_MIXER)
#define MASK_KCONNECT_SPDIFIN           (1 << KCONNECT_SPDIFIN)
#define MASK_KCONNECT_FMIXER            (1 << KCONNECT_FMIXER)	/*AY */
#define MASK_KCONNECT_RMIXER            (1 << KCONNECT_RMIXER)	/*AY */
#define MASK_KCONNECT_SAME              (1 << KCONNECT_SAME)

/* */
/* Open/Close flags */
/* */

#define KOPENCLOSE_SYNCHRONOUS          0x0001

/* */
/* Switch client */
#define KENABLE_CLIENT                  0
#define KDISABLE_CLIENT                 1


/* */
/* DSP timeout */
/* */

#define DSP_TIMEOUT                     10000


/* */
/* DMA transfer alteration flags */
/* */

#define KALTER_AUTOREPEAT               0x0001
#define KALTER_POSITION                 0x0002


/* */
/* DSP hardware */
/* */

#define DSP_PORT_TIMER_COUNT            0x06
#define DSP_PORT_MEMORY_INDEX           0x80
#define DSP_PORT_MEMORY_TYPE            0x82
#define DSP_PORT_MEMORY_DATA            0x84
#define DSP_PORT_CONTROL_REG_A          0xA2
#define DSP_PORT_CONTROL_REG_B          0xA4
#define DSP_PORT_CONTROL_REG_C          0xA6

#define MEMTYPE_INTERNAL_CODE           0x0002
#define MEMTYPE_INTERNAL_DATA           0x0003
#define MEMTYPE_MASK                    0x0003

#define REGB_ENABLE_RESET               0x01
#define REGB_STOP_CLOCK                 0x10

#define REGC_DISABLE_FM_MAPPING         0x02

#define DP_SHIFT_COUNT                  7

#define DMA_BLOCK_LENGTH                32


/* */
/* kernel binary image storage */
/* */

typedef struct tagKERNEL_BIN
{

  PWORD pwBinCode;
  DWORD dwLengthCode;

}
KERNEL_BIN, *PKERNEL_BIN;


/* */
/* client binary image storage */
/* */

typedef struct tagCLIENT_BIN
{

  DWORD dwCodeAddress;

  PWORD pwBinVect;
  PWORD pwBinCode;
  PWORD pwBinData;

  DWORD dwLengthVect;
  DWORD dwLengthCode;
  DWORD dwLengthData;

}
CLIENT_BIN, *PCLIENT_BIN;


/* */
/* FM client binary image storage */
/* */

typedef struct tagFMCLIENT_BIN
{

  DWORD dwCodeAddress;
  DWORD dwData2Address;

  PWORD pwBinVect;
  PWORD pwBinCode;
  PWORD pwBinData;
  PWORD pwBinData2;

  DWORD dwLengthVect;
  DWORD dwLengthCode;
  DWORD dwLengthData;
  DWORD dwLengthData2;

}
FMCLIENT_BIN, *PFMCLIENT_BIN;


/* */
/* client */
/* */

typedef struct tagCLIENT
{

  /* kernel use only */

  PCLIENT_BIN pClient_Bin;
  DWORD dwReferenceCount;
  DWORD dwMaxReference;
  DWORD dwInstanceListArea;
  DWORD dwDspCodeNumUnits;
  PBYTE pbDspCodeMapPtr;

  /* client use */

  DWORD dwDspCodeClientArea;

}
CLIENT, *PCLIENT;


/* */
/* client instance */
/* */

typedef struct tagCLIENT_INST
{

  /* kernel use only */

  DWORD dwClient;
  DWORD dwHostSrcBufferAddr;
  DWORD dwHostSrcBufferLen;
  DWORD dwHostDstBufferAddr;
  DWORD dwHostDstBufferLen;
  DWORD dwHostDstCurrent;
  DWORD dwDSPOutBufferAddr;
  DWORD dwDSPOutBufferLen;
  DWORD dwDSPInConnection;
  DWORD dwDSPOutConnection;
  DWORD dwDspDataNumUnits;
  PBYTE pbDspDataMapPtr;

  /* client use */

  DWORD dwDspDataClientArea;
  DWORD dwDspCodeClientArea;

}
CLIENT_INST, *PCLIENT_INST;


/* */
/* pass through descriptor */
/* */

typedef struct tagPASSTHRU
{

  DWORD dwDSPInConnection;
  DWORD dwDSPOutConnection;
  PBYTE pbDspDataMapPtr;
  DWORD dwDspDataPassThruArea;

  WORD wLeftVolume;
  WORD wRightVolume;

}
PASSTHRU, *PPASSTHRU;


/* */
/* Hardware instance */
/* */

typedef struct tagHWI
{

  DWORD dwDeviceID;
  DWORD dwRevisionID;
  DWORD dwBaseIO;
  DWORD dwFlags;

  PWORD pwSuspendBuffer;

  WORD wI2SSampleCount;
  WORD wI2STimerCount;

  WORD wDspResetCount;

  /* client table */

  CLIENT asClientTable[NUMBER_OF_CLIENTS];

  /* resource lists */

  WORD awTaskList[MAX_TASKS + 1];

  WORD awInstanceCpyThruList[MAX_INSTANCE_CPYTHRU + 1];
  WORD awInstanceModemList[MAX_INSTANCE_MODEM + 1];
  WORD awInstancePos3DList[MAX_INSTANCE_POS3D + 1];
  WORD awInstanceSpkVirtList[MAX_INSTANCE_SPKVIRT + 1];
  WORD awInstanceSRCList[MAX_INSTANCE_SRC + 1];
  WORD awInstanceMINISRCList[MAX_INSTANCE_MINISRC + 1];
  WORD awInstanceSPDIFList[MAX_INSTANCE_SPDIF + 1];

  WORD awVirtualDMAList[MAX_VIRTUAL_DMA_CHANNELS + 1];
  WORD awVirtualADC1List[MAX_VIRTUAL_ADC1_CHANNELS + 1];
  WORD awVirtualADC2List[MAX_VIRTUAL_ADC2_CHANNELS + 1];
  WORD awVirtualCDList[MAX_VIRTUAL_CD_CHANNELS + 1];
  WORD awVirtualMICList[MAX_VIRTUAL_MIC_CHANNELS + 1];

  WORD awVirtualI2SList[MAX_VIRTUAL_I2S_CHANNELS + 1];
  WORD awVirtualCHIList[MAX_VIRTUAL_CHI_CHANNELS + 1];

  WORD awVirtualSPDIFINList[MAX_VIRTUAL_SPDIFIN_CHANNELS + 1];
  WORD awVirtualMIXERList[MAX_VIRTUAL_MIXER_CHANNELS + 1];

  /*AY */
  WORD awVirtualFMIXERList[MAX_VIRTUAL_FMIXER_CHANNELS + 1];
  WORD awVirtualRMIXERList[MAX_VIRTUAL_RMIXER_CHANNELS + 1];

  /* memory maps */

  DWORD dwCodeMemoryBegin;
  DWORD dwCodeMemoryEnd;
  DWORD dwCodeMemoryUnitLength;
  DWORD dwCodeMemoryLength;

  DWORD dwDataMemoryBegin;
  DWORD dwDataMemoryEnd;
  DWORD dwDataMemoryUnitLength;
  DWORD dwDataMemoryLength;

  BYTE abCodeMemoryMap[CODE_MEMORY_MAP_LENGTH];
  BYTE abDataMemoryMap[DATA_MEMORY_MAP_LENGTH];

  /* vector list */

  WORD awVectorList[KCODE_VECTORS_LENGTH];

}
HWI, *PHWI;


/* */
/* function return codes */
/* */

typedef DWORD KRETURN;

#define KRETURN_SUCCESS                 0
#define KRETURN_ERROR_GENERIC           1
#define KRETURN_ERROR_BUSY              2
#define KRETURN_ERROR_UNLOADED          3


/* */
/* external function prototypes */
/* */
#ifdef __cplusplus
extern "C"
{
#endif

  WORD kDspReadWord (allegro_devc * devc, DWORD dwBaseIO, DWORD dwMemType,
		     DWORD dwMemAddr);

  VOID kDspWriteWord
    (allegro_devc * devc, DWORD dwBaseIO, DWORD dwMemType, DWORD dwMemAddr,
     WORD wMemData);

  VOID kDspReadWords
    (allegro_devc * devc, DWORD dwBaseIO,
     DWORD dwMemType, DWORD dwMemAddr, DWORD dwMemLen, PWORD pwHostAddr);

  VOID kDspWriteWords
    (allegro_devc * devc, DWORD dwBaseIO,
     DWORD dwMemType, DWORD dwMemAddr, DWORD dwMemLen, PWORD pwHostAddr);

  VOID kDspWriteZeros
    (allegro_devc * devc, DWORD dwBaseIO, DWORD dwMemType, DWORD dwMemAddr,
     DWORD dwMemLen);

  VOID kPIOInterruptHandler (allegro_devc * devc, PHWI phwi,
			     PCLIENT_INST pClient_Inst);

  VOID kI2SInterruptHandler (allegro_devc * devc, PHWI phwi,
			     PDWORD pdwI2SRate);

  KRETURN kQueryPosition
    (allegro_devc * devc, PHWI phwi,
     PCLIENT_INST pClient_Inst, DWORD dwQueryOutput, PDWORD pdwPosition);

  KRETURN kResetApuBlockCount (allegro_devc * devc, PHWI phwi,
			       PCLIENT_INST pClient_Inst);

  KRETURN kGetApuBlockCount
    (allegro_devc * devc, PHWI phwi, PCLIENT_INST pClient_Inst,
     PDWORD pdwBlockCount);

  KRETURN kInitKernel
    (allegro_devc * devc, PHWI * pphwi,
     DWORD dwDeviceID, DWORD dwRevisionID, DWORD dwBaseIO, DWORD dwFlags);

  KRETURN kDSPMemCheck (allegro_devc * devc, PHWI phwi);

  KRETURN kTermKernel (allegro_devc * devc, PHWI phwi, DWORD dwBaseIO);

  KRETURN kSuspendKernel (allegro_devc * devc, PHWI phwi);

  KRETURN kResumeKernel (allegro_devc * devc, PHWI phwi);

  KRETURN kOpenInstance
    (allegro_devc * devc, PHWI phwi,
     DWORD dwClient,
     DWORD dwFlags, DWORD dwLen, PCLIENT_INST * ppClient_Inst);

  KRETURN kCloseInstance
    (allegro_devc * devc, PHWI phwi, PCLIENT_INST pClient_Inst,
     DWORD dwFlags);

  KRETURN kSwitchClient (allegro_devc * devc, PHWI phwi,
			 PCLIENT_INST pClient_Inst, DWORD dwFlags);

  KRETURN kSetInstanceReady (allegro_devc * devc, PHWI phwi,
			     PCLIENT_INST pClient_Inst);

  KRETURN kSetInstanceNotReady (allegro_devc * devc, PHWI phwi,
				PCLIENT_INST pClient_Inst);

  KRETURN kStartTransfer
    (allegro_devc * devc, PHWI phwi,
     PCLIENT_INST pClient_Inst,
     DWORD dwAutoRepeat,
     DWORD dwHostSrcBufferAddr,
     DWORD dwHostSrcBufferLen,
     DWORD dwHostDstBufferAddr,
     DWORD dwHostDstBufferLen,
     DWORD dwDSPInBufferAddr,
     DWORD dwDSPInBufferLen,
     DWORD dwDSPOutBufferAddr,
     DWORD dwDSPOutBufferLen,
     DWORD dwDSPInConnection, DWORD dwDSPOutConnection);

  KRETURN kStopTransfer (allegro_devc * devc, PHWI phwi,
			 PCLIENT_INST pClient_Inst);

  KRETURN kAlterTransfer
    (allegro_devc * devc, PHWI phwi,
     PCLIENT_INST pClient_Inst,
     DWORD dwFlags, DWORD dwAutoRepeat, DWORD dwPosition);

  KRETURN kSwitchPINConnection
    (allegro_devc * devc, PHWI phwi,
     PCLIENT_INST pClient_Inst,
     DWORD dwDSPInConnection, DWORD dwDSPOutConnection);

  KRETURN kQueryActivity
    (allegro_devc * devc, PHWI phwi, PDWORD pdwClientMasks,
     PDWORD pdwConnectMasks);

  KRETURN kSetTimer (allegro_devc * devc, PHWI phwi, DWORD dwTimeInterval);

  KRETURN kOpenPassThru
    (allegro_devc * devc, PHWI phwi,
     PPASSTHRU * ppPassThru,
     DWORD dwDSPInConnection, DWORD dwDSPOutConnection);

  KRETURN kClosePassThru (allegro_devc * devc, PHWI phwi,
			  PPASSTHRU pPassThru);

  KRETURN kSetVolume
    (allegro_devc * devc, PHWI phwi,
     PCLIENT_INST pClient_Inst,
     WORD wLeftVolume, WORD wRightVolume, WORD wBoosterMode);

  KRETURN kSetRearVolume
    (allegro_devc * devc, PHWI phwi,
     PCLIENT_INST pClient_Inst, WORD wLeftRearVolume, WORD wRightRearVolume);

  KRETURN kSetPassThruVolume
    (allegro_devc * devc, PHWI phwi, PPASSTHRU pPassThru, WORD wLeftVolume,
     WORD wRightVolume);

  KRETURN kSetPassThruRearVolume
    (allegro_devc * devc, PHWI phwi,
     PPASSTHRU pPassThru, WORD wLeftRearVolume, WORD wRightRearVolume);


  KRETURN kSetMasterVolume (allegro_devc * devc, PHWI phwi, WORD wLeftVolume,
			    WORD wRightVolume);


  KRETURN kSetFrequency
    (allegro_devc * devc, PHWI phwi, PCLIENT_INST pClient_Inst,
     WORD wFrequency);

#ifdef __cplusplus
}
#endif


/* */
/* external data declarations */
/* */
extern KERNEL_BIN gsMemChkVectCode;
extern KERNEL_BIN gsKernelVectCode;
extern KERNEL_BIN gsKernelVectCodeWithHSP;



extern CLIENT_BIN gasCpyThruVectCode[];
extern CLIENT_BIN gasModemVectCode[];
extern CLIENT_BIN gasPos3DVectCode[];
extern CLIENT_BIN gasSpkVirtVectCode[];
extern CLIENT_BIN gasSpkVirtVectCode_CRL[];
extern CLIENT_BIN gasSRCVectCode[];
extern CLIENT_BIN gasMINISRCVectCode[];
extern CLIENT_BIN gasSPDIFVectCode[];


extern FMCLIENT_BIN gsFMVectCode;

extern WORD MIXER_TASK_NUMBER;

/* */
/* critical enter/leave */
/* */

#if defined( DOS_MODEL ) || defined( WDM_MODEL )
#define CRITENTER
#define CRITLEAVE
#endif

#if defined( VXD_MODEL )
#define CRITENTER       _asm pushfd \
                        _asm cli

#define CRITLEAVE       _asm popfd
#endif

#ifdef WDM_MODEL
#define KCALL( func )   func
#define KBEGIN( func )  if ( KRETURN_SUCCESS == func ) {
#define KEND()          }
#endif

#ifdef NT_MODEL
#undef  NULL
#define NULL            0
#endif

#endif

/*--------------------------------------------------------------------------- */
/*  End of File: kernel.h */
/*--------------------------------------------------------------------------- */

/******************************************************************************
 *                                                                            *
 *                       (C) 1997-1999 ESS Technology, Inc.                   *
 *                                                                            *
 ******************************************************************************/
