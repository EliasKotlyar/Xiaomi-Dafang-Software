/*
 * Purpose: Low level routines for Terrate Aureon 7.1 family
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

#include "oss_envy24ht_cfg.h"

#define IN
#define OUT
#define UCHAR unsigned char
#define PUCHAR UCHAR*
#define BYTE unsigned char
#define PBYTE BYTE*
#define BOOLEAN unsigned char
#define ULONG unsigned int
#define PULONG ULONG*
#define USHORT unsigned short
#define PUSHORT USHORT*

#define WORD USHORT
#define PWORD PUSHORT

/*
 * Default levels 
 */
#define WM_OUT_DEFAULT          0x7F
#define WM_OUT_MAX              0x7F
#define WM_INP_DEFAULT          0x0C	/* for 0dB */
#define AC97_INP_DEFAULT        0x1f1f
#define AC97_INP_MAX        0x1f

#define REG_CCS		1
#define REG_MT		2

#define WM_MASTER_MODE_CNTRL 0

/* List of the WKN CCS Control Registers. */
#define WKN_CONTROL_REG         0x00
#define WKN_INT_MASK_REG        0x01
#define WKN_INT_STAT_REG        0x02
#define WKN_DMA_INT        0x10
#define WKN_UART_RECV      0x80
#define WKN_UART_TRAN      0x20

/* #define WKN_INDEX_REG           0x03 */

#define WKN_SYS_CONFIG          0x04
#define WKN_49MHZ           0x40
#define WKN_ACLINK_CONFIG       0x05
#define CODEC_AKM           0x80

#define WKN_I2S_CONFIG          0x06
#define WKN_SPDIF_CONFIG        0x07

#define SPDIF_ENABLE_MASK       0x00010000

/* #define WKN_AC97_INDEX          0x08 */
/* #define WKN_AC97_CMDSTAT        0x09 */
#define WKN_TX_QUE              0x0A
#define WKN_RX_QUE              0x0B

#define WKN_MIDI_PORT1_DATA_REG 0x0C
#define WKN_MIDI_PORT1_CMD_REG  0x0D
#define WKN_NMI_EXTSTAT_REG     0x0E

#define WKN_I2C_DEVADDR           0x10
#define WKN_I2C_AKM          0x20
#define WKN_I2C_WRITE          0x01
#define WKN_I2C_READ           0x00
#define WKN_I2C_BYTEADDR          0x11
#define WKN_I2C_DATA              0x12
#define WKN_I2C_STATUS            0x13
#define WKN_I2C_BUSY           0x01
#define WKN_GPIO_DATA0           0x14
#define WKN_GPIO_DATA1           0x15
#define WKN_GPIO_DATA2           0x1E
#define WKN_GPIO_WRT_MASK0       0x16
#define WKN_GPIO_WRT_MASK1       0x17
#define WKN_GPIO_WRT_MASK2       0x1F
#define WKN_GPIO_DIR0           0x18
#define WKN_GPIO_DIR1           0x19
#define WKN_GPIO_DIR2           0x1A
#define WKN_GPIO_DATA16         0x1E
#define WKN_GPIO_WRT_MASK16     0x1F

#define GPIO_DATA_ADR_MASK      0x000000FF
#define GPIO_LD_DATA_H_MASK     0x00000100
#define GPIO_LD_DATA_L_MASK     0x00000200
#define GPIO_LD_ADR_MASK        0x00000400
#define GPIO_GO_MASK            0x00000800
#define GPIO_WM8770_CS_MASK     0x00001000
#define GPIO_INT_REST_MASK      0x00002000
#define GPIO_REMOTE_MASK        0x00004000
#define GPIO_HEADPHONE_MASK     0x00004000
#define GPIO_DIGITAL_SEL        0x00008000
#define GPIO_AC97_RESET_MASK    0x00010000
#define GPIO_SDA_TX_MASK        0x00020000
#define GPIO_SDA_MASK           0x00040000
#define GPIO_SCL_MASK           0x00080000
#define GPIO_WM8770_RS_MASK     0x00100000
#define GPIO_CS8415_CDTO_MASK   0x00200000
#define GPIO_CS8415_CS_MASK     0x00400000

#define WM_DAC1_VOL_CNTRL           0x00
#define WM_DAC2_VOL_CNTRL           0x02
#define WM_DAC3_VOL_CNTRL           0x04
#define WM_DAC4_VOL_CNTRL           0x06
#define WM_MASTER_VOL_CNTRL         0x08
#define WM_POWER_CNTRL              0x18
#define WM_ADC_GAIN_CNTRL           0x19
#define WM_ADC_INPUT_MX             0x1B
#define WM_OUT12_SELECT             0x1C
#define WM_OUT34_SELECT             0x1D

#define CS_CONTROL_1                0x01
#define CS_CONTROL_2                0x02
#define CS_CLK_SRC_CNTRL            0x04
#define CS_SER_OUT_FORMAT           0x06

#define MT_SAMPLE_RATE_REG              0x01
#define MT_48KHZ                 0x0
#define MT_24KHZ                 0x01
#define MT_12KHZ                 0x02
#define MT_9p6KHZ               0x03
#define MT_32KHZ                 0x04
#define MT_16KHZ                 0x05
#define MT_8KHZ                 0x06
#define MT_96KHZ                 0x07
#define MT_192KHZ                0x0E
#define MT_64KHZ                 0x0F
#define MT_44p1KHZ               0x08
#define MT_22p05KHZ              0x09
#define MT_11p025KHZ             0x0A
#define MT_88p2KHZ               0x0B
#define MT_176p4KHZ              0x0C

#define MT_DATA_FORMAT_REG              0x02
#define MT_128X                  0x08
#define MT_INTR_MASK_REG         0x03

#define SRC_PDMA        0x00
#define SRC_PSDIN0_L    0x02
#define SRC_PSDIN0_R    0x03
#define SRC_SPDIN_L     0x06
#define SRC_SPDIN_R     0x07
#define SRC_MASK        0x07

static const UCHAR gWMRegister[] = { WM_DAC1_VOL_CNTRL,
  WM_DAC1_VOL_CNTRL + 1,
  WM_DAC2_VOL_CNTRL,
  WM_DAC2_VOL_CNTRL + 1,
  WM_DAC3_VOL_CNTRL,
  WM_DAC3_VOL_CNTRL + 1,
  WM_DAC4_VOL_CNTRL,
  WM_DAC4_VOL_CNTRL + 1,
  WM_MASTER_VOL_CNTRL,
  WM_MASTER_VOL_CNTRL + 1,
  WM_ADC_GAIN_CNTRL,
  WM_ADC_GAIN_CNTRL + 1
};

#define AC97_IDXREG_RESET           0x00
#define AC97_IDXREG_STEREO_OUT      0x02
#define AC97_IDXREG_MONO_OUT        0x06
#define AC97_IDXREG_PCBEEP          0x0A
#define AC97_IDXREG_PHONE           0x0C
#define AC97_IDXREG_MIC_IN          0x0E
#define AC97_IDXREG_LINE_IN         0x10
#define AC97_IDXREG_CD_IN           0x12
#define AC97_IDXREG_VIDEO_IN        0x14
#define AC97_IDXREG_AUX_IN          0x16
#define AC97_IDXREG_PCM_OUT         0x18
#define AC97_IDXREG_RECORD_SELECT   0x1A
#define AC97_IDXREG_RECORD_GAIN     0x1C
#define AC97_IDXREG_GEN_PURPOSE     0x20
#define AC97_IDXREG_POWER_DOWN      0x26
#define AC97_IDXREG_TEST_CONTROL    0x5A
#define AC97_IDXREG_VENDOR_RESV     0x7A
#define AC97_IDXREG_VENDOR_ID1      0x7C
#define AC97_IDXREG_VENDOR_ID2      0x7E
/* AC97 register - AC97_IDXREG_RECORD_SELECT */
#define AC97_REC_SELECT_MIC         0x0000
#define AC97_REC_SELECT_LINEIN      0x0404
#define AC97_REC_SELECT_ST_MIX      0x0505

/*			   
 * List of the Channel IDs
 */
#define CH_MASTER_LEFT            ((OUT_MASTER<<16)|CH_LEFT)
#define CH_MASTER_RIGHT           ((OUT_MASTER<<16)|CH_RIGHT)
#define CH_MASTER_BOTH            ((OUT_MASTER<<16)|CH_BOTH)

#define CH_OUT12_LEFT             ((OUT_12<<16)|CH_LEFT)
#define CH_OUT12_RIGHT            ((OUT_12<<16)|CH_RIGHT)
#define CH_OUT12_BOTH             ((OUT_12<<16)|CH_BOTH)
#define CH_OUT34_LEFT             ((OUT_34<<16)|CH_LEFT)
#define CH_OUT34_RIGHT            ((OUT_34<<16)|CH_RIGHT)
#define CH_OUT34_BOTH             ((OUT_34<<16)|CH_BOTH)
#define CH_OUT56_LEFT             ((OUT_56<<16)|CH_LEFT)
#define CH_OUT56_RIGHT            ((OUT_56<<16)|CH_RIGHT)
#define CH_OUT56_BOTH             ((OUT_56<<16)|CH_BOTH)
#define CH_OUT78_LEFT             ((OUT_78<<16)|CH_LEFT)
#define CH_OUT78_RIGHT            ((OUT_78<<16)|CH_RIGHT)
#define CH_OUT78_BOTH             ((OUT_78<<16)|CH_BOTH)

#define CH_IN12_LEFT              ((IN_12<<16)|CH_LEFT)
#define CH_IN12_RIGHT             ((IN_12<<16)|CH_RIGHT)
#define CH_IN12_BOTH              ((IN_12<<16)|CH_BOTH)

#define CH_FRONT_BOTH             CH_OUT12_BOTH
#define CH_REAR_BOTH              CH_OUT34_BOTH
#define CH_CENTER                 CH_OUT56_LEFT
#define CH_LFE                    CH_OUT56_RIGHT
#define CH_BS_BOTH                CH_OUT78_BOTH

#define MAX_VOLUME 0x7F
#define MIN_VOLUME 0x00

#define MAX_GAIN   0x1F

#define MT_PLAY_REC_UNDOVR         0x01A
#define MT_INTR_STATUS_REG         0x00
#define MT_INTR_MASK_REG           0x03
#define MT_SPDIF_REG                 0x3C

/* List of AC97 inputs */
#define AC97_CD		0
#define AC97_AUX	1
#define AC97_LINE	2
#define AC97_MIC	3
#define AC97_PHONO	4
#define AC97_LINE2	5
#define AC97_COUNT	6	/* Must match devc->m_AC97Volume definition in envy24ht.h */

/* Channel def */
#define CH_LEFT    0x00000001
#define CH_RIGHT   0x00000002
#define CH_BOTH    (CH_LEFT|CH_RIGHT)
#define CH_NOP     0xFFFFFFFF

/*
 *  List of inputs
 */
#define ADC_CD		0
#define ADC_AUX		1
#define ADC_LINE	2
#define ADC_MIC		3
#define ADC_PHONO	4
#define ADC_WTL		5
#define ADC_LINE_REAR	6
#define ADC_STEREO_MIX	7
#define ADC_COUNT	8	/* Must match the size of m_ADCVolume */

/*
 * List of Lines
 */
#define LINE_OUT_1L	0x00000000
#define LINE_OUT_1R	0x00000001
#define LINE_OUT_2L	0x00000002
#define LINE_OUT_2R	0x00000003
#define LINE_OUT_3L	0x00000004
#define LINE_OUT_3R	0x00000005
#define LINE_OUT_4L	0x00000006
#define LINE_OUT_4R	0x00000007
#define LINE_MASTER	0x00000008
#define LINE_MASTER_	0x00000009
#define LINE_GAIN_L	0x0000000a
#define LINE_GAIN_R	0x0000000b
#define LINE_S_NUM	0x0000000c	/* 12 - Should match devc->m_fDACMute size */

/*
 * List of Stereo Lines
 */
#define OUT_12		0x00000000
#define OUT_34		0x00000001
#define OUT_56		0x00000002
#define OUT_78		0x00000003
#define OUT_MASTER	0x00000004
#define IN_12		0x00000005
#define NUM_LINES	0x00000006

/*
 * SPDIF Out Source
 */
enum
{
  DIGOUT_DIG_IN = 0,
  DIGOUT_ANA_IN,
  DIGOUT_WAVE
};
/*
 * Digital IN Source
 */
enum
{
  DIGIN_OPTICAL = 0,
  DIGIN_COAX,
  DIGIN_CD_IN
};
/*
 * Out-0 Source
 */
enum
{
  SRC_DMA1 = 0,
  SRC_PSDIN0,
  SRC_SPDIN
};

/*
 * Line IN Source (Aureon 7.1 Universe only)
 */
#define SRC_AUX		0
#define SRC_WTL		1
#define SRC_LINE_REAR	2
#define SRC_GROUND	3

/*
 * Clock Source
 */
enum
{
  ICE_INTERNAL_CLOCK = 0,
  ICE_SPDIF_CLOCK
};
/*
 * Function of Frontjack
 */
enum
{
  FRONT_JACK_LINEIN = 0,
  FRONT_JACK_HEADPHONE
};

enum MUX_TP_PIN
{
  CD_IN_MUX_TP_PIN = 1,
  LINE_IN_MUX_TP_PIN,
  AUX_IN_MUX_TP_PIN,
  PHONO_IN_MUX_PIN,
  LINE2_IN_MUX_PIN,
  MIC_IN_MUX_TP_PIN,
  DIG_IN_MUX_TP_PIN,
  STEREO_MIX_MUX_TP_PIN
};

#define AUREON_REMOTE_CNTRL     0x32
/* #define AUREON_REMOTE_ID        0x0016 */
#define AUREON_REMOTE_ID        0x6B86
#define AUREON_PCA_BASEBOARD    0x40

#define MT_LOOPBK_CONTROL         0x02C
#define MT_LOOPBK_CONTROL_DAC_REG       0x030
#define MT_LOOPBK_CONTROL_SPDIF_OUT_REG 0x032

#define WIDTH_BYTE	1
#define WIDTH_WORD	2
#define WIDTH_DWORD	4

#define BITS_ON     0x10
#define BITS_OFF    0x00
#define TOGGLE_ON   0x20

#define CRITSEC_ON	0x00000001

#define WIDTH_MASK  0x0f

#include "spdif.h"
#include "envy24ht.h"

/*! \fn =======================================================================
  Function    : WritePort
-------------------------------------------------------------------------------
  Description : 
  Parameters  : dwRegType -> 
              : dwIndex -> 
              : dwValue -> 
              : dwWidth -> 
-------------------------------------------------------------------------------
  Notes       : 
=============================================================================*/
static void WritePort
  (envy24ht_devc * devc,
   IN ULONG dwRegType, IN ULONG dwIndex, IN ULONG dwValue, IN ULONG dwWidth)
{
  oss_native_word pPort = devc->ccs_base;

  switch (dwRegType)
    {
    case REG_CCS:
      pPort = devc->ccs_base;
      break;

    case REG_MT:
      pPort = devc->mt_base;
      break;
    }

  /* registers are addressible in byte, word or dword */
  switch (dwWidth)
    {
    case WIDTH_BYTE:
      OUTB (devc->osdev, dwValue, pPort + dwIndex);
      break;
    case WIDTH_WORD:
      OUTW (devc->osdev, dwValue, pPort + dwIndex);

      break;
    case WIDTH_DWORD:
      OUTL (devc->osdev, dwValue, pPort + dwIndex);
      break;
    }
}


/*! \fn =======================================================================
  Function    : ReadPort
-------------------------------------------------------------------------------
  Description : 
  Returns     :  -> 
  Parameters  : IN ULONG     dwWidth -> 
                IN ULONG     dwWidth 
-------------------------------------------------------------------------------
  Notes       : 
=============================================================================*/
static ULONG ReadPort
  (envy24ht_devc * devc,
   IN ULONG dwRegType, IN ULONG dwIndex, IN ULONG dwWidth)
{
  oss_native_word pPort = devc->ccs_base;
  ULONG dwData = 0;

  switch (dwRegType)
    {
    case REG_CCS:
      pPort = devc->ccs_base;
      break;

    case REG_MT:
      pPort = devc->mt_base;
      break;
    }

  /* all other registers are addressible in byte, word or dword */
  switch (dwWidth)
    {
    case WIDTH_BYTE:
      dwData = INB (devc->osdev, pPort + dwIndex);
      break;
    case WIDTH_WORD:
      dwData = INW (devc->osdev, pPort + dwIndex);
      break;
    case WIDTH_DWORD:
      dwData = INL (devc->osdev, pPort + dwIndex);
      break;
    }


  return dwData;
}

/*! \fn =======================================================================
  Function    : ReadModifyWritePort
-------------------------------------------------------------------------------
  Description : 
  Returns     : VOID  -> 
  Parameters  : wRegType -> 
              : dwMask -> 
              : dwControl ->
-------------------------------------------------------------------------------
  Notes       : 
=============================================================================*/
static void ReadModifyWritePort
  (envy24ht_devc * devc,
   IN ULONG dwRegType, IN ULONG dwIndex, IN ULONG dwMask, IN ULONG dwControl
   /* dwControl: */
   /* bit[3:0] : data width, 1 => byte, 2 => word, 4 => dword */
   /* bit[4]   : 1 => turn on bit(s); 0 => turn off bit(s) */
   /* bit[5]   : 1 => toggle bit(s) */
   /* bit[6]   : 1 => turn on Hw access critical section */
  )
{
  ULONG dwValue;
  ULONG dwWidth;
  oss_native_word flags;

  dwWidth = dwControl & WIDTH_MASK;

  if (dwControl & CRITSEC_ON)
    {
      MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
    }

  dwValue = ReadPort (devc, dwRegType, dwIndex, dwWidth);

  /* see whether we should turn on or off the bit(s) */
  if (dwControl & BITS_ON)
    dwValue |= dwMask;
  else
    dwValue &= ~dwMask;

  WritePort (devc, dwRegType, dwIndex, dwValue, dwWidth);

  /* see if we need to toggle the bit(s) */
  if (dwControl & TOGGLE_ON)
    {
      dwValue ^= dwMask;
      WritePort (devc, dwRegType, dwIndex, dwValue, dwWidth);
    }

  if (dwControl & CRITSEC_ON)
    {
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
    }

}

/*! \fn =======================================================================
 Function    : WriteGPIO
-------------------------------------------------------------------------------
 Description : Writes masked Data to GPIO Port
 Parameters  : IN ULONG Data -> 
             : IN ULONG Mask -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
WriteGPIO (envy24ht_devc * devc, IN ULONG Data, IN ULONG Mask)
{
  USHORT MaskL;
  UCHAR MaskH;
  USHORT DataL;
  UCHAR DataH;

  /* Do the masking. */
  MaskL = (USHORT) Mask;
  MaskH = (UCHAR) (Mask >> 16);
  DataL = (USHORT) Data;
  DataH = (UCHAR) (Data >> 16);
  devc->gpio_shadow_L &= ~MaskL;
  devc->gpio_shadow_H &= ~MaskH;
  devc->gpio_shadow_L |= (MaskL & DataL);
  devc->gpio_shadow_H |= (MaskH & DataH);

  /* Write Data */
  WritePort (devc, REG_CCS, WKN_GPIO_DATA0, devc->gpio_shadow_L, WIDTH_WORD);
  WritePort (devc, REG_CCS, WKN_GPIO_DATA2, devc->gpio_shadow_H, WIDTH_BYTE);
}

/*! \fn =======================================================================
 Function    : SetSDADir
-------------------------------------------------------------------------------
 Description : 
 Returns     : void  -> 
 Parameters  : IN BOOLEAN fOut -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
SetSDADir (envy24ht_devc * devc, IN BOOLEAN fOut)
{
  if (fOut)
    {
      /* Set GPIO 18 direction to output */
      WritePort (devc, REG_CCS, WKN_GPIO_DIR2, 0xDF, WIDTH_BYTE);
      /* Turn IIC logic to SDA write */
      WriteGPIO (devc, GPIO_SDA_TX_MASK, GPIO_SDA_TX_MASK);
    }
  else
    {
      WriteGPIO (devc, 0, GPIO_SDA_TX_MASK);
      /* Direction "Read" for GPIO 18 (SDA) */
      WritePort (devc, REG_CCS, WKN_GPIO_DIR2, 0xDB, WIDTH_BYTE);
    }
}

/*! \fn =======================================================================
 Function    : SetSDA
-------------------------------------------------------------------------------
 Description : 
 Parameters  : BOOLEAN fSet
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
SetSDA (envy24ht_devc * devc, BOOLEAN fSet)
{
  /* Set GPIO 18 direction to output */
  SetSDADir (devc, 1);
  /* Write SDA */
  WriteGPIO (devc, fSet ? GPIO_SDA_MASK : 0, GPIO_SDA_MASK);
}

/*! \fn =======================================================================
 Function    : GetSDA
-------------------------------------------------------------------------------
 Description : 
 Returns     : 
UCHAR  -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static UCHAR
GetSDA (envy24ht_devc * devc)
{
  UCHAR ulData;

  /* Turn IIC logic to SDA read */
  SetSDADir (devc, 0);
  ulData =
    ((ReadPort (devc, REG_CCS, WKN_GPIO_DATA2, WIDTH_BYTE) & 0x04) == 0x04);
  return ulData;
}

/*! \fn =======================================================================
 Function    : SetIIC
-------------------------------------------------------------------------------
 Description : 
 Parameters  : IN BOOLEAN fSDA -> 
             : IN BOOLEAN fCLK -> 
             : IN ULONG ulSleep -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void SetIIC
  (envy24ht_devc * devc, IN BOOLEAN fSDA, IN BOOLEAN fCLK, IN ULONG ulSleep)
{
  SetSDA (devc, fSDA);
  if (fCLK)
    WriteGPIO (devc, GPIO_SCL_MASK, GPIO_SCL_MASK);
  else
    WriteGPIO (devc, 0, GPIO_SCL_MASK);
  if (ulSleep)
    oss_udelay (ulSleep);
}

/*! \fn =======================================================================
 Function    : IICStart
-------------------------------------------------------------------------------
 Description : 
 Parameters  : ULONG ulSleep -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
IICStart (envy24ht_devc * devc, ULONG ulSleep)
{
  /* falling edge of SDA while SCL is HIGH */
  SetIIC (devc, 1, 1, ulSleep);
  SetIIC (devc, 0, 1, ulSleep);
}

/*! \fn =======================================================================
 Function    : IICStop
-------------------------------------------------------------------------------
 Description : 
 Parameters  : ULONG ulSleep -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
IICStop (envy24ht_devc * devc, ULONG ulSleep)
{
  /* rising edge of SDA while SCL is HIGH */
  SetIIC (devc, 0, 1, ulSleep);
  SetIIC (devc, 1, 1, ulSleep);
  /* Reset Lines (No IIC requirement, but for prevent conflicts with SPI) */
  /*SetIIC(0,0,ulSleep); */
}

/*! \fn =======================================================================
 Function    : IICSendByte
-------------------------------------------------------------------------------
 Description : 
 Returns     : UCHAR  -> 
 Parameters  : CHAR bByte -> 
             : ULONG ulSleep -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static UCHAR IICSendByte
  (envy24ht_devc * devc, IN UCHAR bByte, IN ULONG ulSleep)
{
  UCHAR bDataBit, bAck;
  int i;
  for (i = 7; i >= 0; i--)	/* send byte (MSB first) */
    {
      bDataBit = (bByte >> i) & 0x01;

      SetIIC (devc, bDataBit, 0, ulSleep);
      SetIIC (devc, bDataBit, 1, ulSleep);
      SetIIC (devc, bDataBit, 0, ulSleep);
    }				/* end for i */

  SetIIC (devc, 1, 0, ulSleep);

  /* This is neccesary for PLC */
  SetSDADir (devc, 0);
  /* Get acknowledge */
  SetIIC (devc, 1, 1, ulSleep);
  bAck = GetSDA (devc);
  /*if (fAddress) */
  /*    SetIIC(devc, 1,0,ulSleep); */
  /* else */
  /* this is a start condition but never mind */
  SetIIC (devc, 0, 0, ulSleep);
  return (!bAck);		/* bAck = 0 --> success */
}

#if 0
/*! \fn =======================================================================
 Function    : IICReceiveByte
-------------------------------------------------------------------------------
 Description : 
 Returns     : UCHAR  -> 
 Parameters  : CHAR fLastByte -> 
             : ULONG ulSleep -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static UCHAR IICReceiveByte
  (envy24ht_devc * devc, UCHAR fLastByte, ULONG ulSleep)
{
  UCHAR bRead = 0;
  int i;

  for (i = 7; i >= 0; i--)
    {
      SetIIC (devc, 1, 0, ulSleep);
      SetIIC (devc, 1, 1, ulSleep);
      bRead <<= 1;
      bRead += GetSDA (devc);
    }

  /* -> no acknowledge for last received byte */
  SetIIC (devc, fLastByte, 0, ulSleep);	/* SDA = HIGH for last byte */
  SetIIC (devc, fLastByte, 1, ulSleep);
  SetIIC (devc, fLastByte, 0, ulSleep);	/* clock -> LOW */

  return bRead;
}
#endif

/*! \fn =======================================================================
 Function    : IICWriteBuffer
-------------------------------------------------------------------------------
 Description : 
 Returns     : UCHAR  -> 
 Parameters  : CHAR bIicAddress -> 
             : PUCHAR pbByte -> 
             : USHORT nNoBytes -> 
             : ULONG ulSleep -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static UCHAR IICWriteBuffer
  (envy24ht_devc * devc,
   UCHAR bIicAddress, PUCHAR pbByte, USHORT nNoBytes, ULONG ulSleep)
{
  IICStart (devc, ulSleep);

  /* send IIC address and data byte */
  if (!IICSendByte (devc, bIicAddress, ulSleep))
    goto FAILED;
  /* send buffer */
  do
    {
      if (!IICSendByte (devc, *pbByte, ulSleep))
	goto FAILED;		/* got no acknowledge */
      pbByte++;
      nNoBytes--;
    }
  while (nNoBytes);

  IICStop (devc, ulSleep);
  return 1;

FAILED:
  IICStop (devc, ulSleep);
  return 0;
}

/*! \fn =======================================================================
 Function    : WriteCS8415
-------------------------------------------------------------------------------
 Description : 
 Parameters  : IN UCHAR Register -> 
             : IN UCHAR Value -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void WriteCS8415
  (envy24ht_devc * devc, IN UCHAR Register, IN UCHAR Value)
{
  ULONG i;
  BOOLEAN fData;
  ULONG ulCmd;
  ULONG ulCount;

  /* m_pAdapter->HwEnter(); *//* TODO */

  /* Clock low to prevent IIC Startcondition */
  WriteGPIO (devc, 0, GPIO_SCL_MASK);

  SetSDA (devc, 0);
  /* Chip select (CS low) */
  WriteGPIO (devc, 0, GPIO_CS8415_CS_MASK);

  /* format buffer */
  ulCmd = 0x200000 +		/* chip address + R/W */
    (((ULONG) Register) << 8) +	/* register address */
    Value;			/* Value */
  ulCmd <<= 8;
  ulCount = 24;			/* (AddressOnly) ? 16:24; */
  for (i = 0; i < ulCount; i++)
    {
      fData = (ulCmd & 0x80000000) ? 1 : 0;
      /* CCLK -> low */
      WriteGPIO (devc, 0, GPIO_SCL_MASK);
      oss_udelay (3);
      /* CDTI -> Set data */
      SetSDA (devc, fData);
      oss_udelay (3);
      /* CCLK -> high */
      WriteGPIO (devc, GPIO_SCL_MASK, GPIO_SCL_MASK);
      ulCmd <<= 1;
    }
  WriteGPIO (devc, 0, GPIO_SCL_MASK);	/* CCLK -> low */

  /* Chip deselect (CS high) */
  WriteGPIO (devc, GPIO_CS8415_CS_MASK, GPIO_CS8415_CS_MASK);
  /* m_pAdapter->HwLeave(); *//* TODO */
}

#if 0
/*! \fn =======================================================================
 Function    : ReadCS8415
-------------------------------------------------------------------------------
 Description : 
 Returns     : UCHAR  -> 
 Parameters  : IN UCHAR Register -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
 /*ARGSUSED*/ static UCHAR
ReadCS8415 (envy24ht_devc * devc, IN UCHAR Register)
{
  ULONG i;
  BOOLEAN fData;
  UCHAR cValue = 0;
  USHORT wCmd;

  /* m_pAdapter->HwEnter(); *//* TODO */

  /* Clock low to prevent IIC Startcondition */
  WriteGPIO (devc, 0, GPIO_SCL_MASK);

  SetSDA (devc, 0);

  /* Chip select (CS low) */
  WriteGPIO (devc, 0, GPIO_CS8415_CS_MASK);
  /* Set GPIO21 to read direction */
  WritePort (devc, REG_CCS, WKN_GPIO_DIR2, 0xDF, WIDTH_BYTE);

  wCmd = 0x2100;		/* +  *//* chip address + R/W */
  /*(((USHORT)Register) << 8); *//* register address */
  for (i = 0; i < 16; i++)
    {
      fData = (wCmd & 0x8000) ? 1 : 0;
      /* CCLK -> low */
      WriteGPIO (devc, 0, GPIO_SCL_MASK);
      /* CDTI -> Set data */
      SetSDA (devc, fData);
      oss_udelay (3);
      /* CCLK -> high */
      WriteGPIO (devc, GPIO_SCL_MASK, GPIO_SCL_MASK);
      wCmd <<= 1;
      if (i > 7)
	{
	  /* Read CDTO */
	  cValue <<= 1;
	  cValue +=
	    ((ReadPort (devc, REG_CCS, WKN_GPIO_DATA2, WIDTH_BYTE) & 0x20) ==
	     0x20);
	}
    }

  WriteGPIO (devc, 0, GPIO_SCL_MASK);	/* CCLK -> low */

  /* Chip deselect (CS high) */
  WriteGPIO (devc, GPIO_CS8415_CS_MASK, GPIO_CS8415_CS_MASK);

  /* m_pAdapter->HwLeave(); *//* TODO */
  return cValue;
}
#endif

/*
 * Definition of PCA I/Os
 */
typedef union tagPCA_CFG
{
  unsigned char cReg;
  struct
  {
    unsigned char P00_SourceSel:2;	/* LineIN Selector */
    /* 00 -> Aux */
    /* 01 -> Wavetable */
    /* 11 -> LineIN Rear */
    unsigned char P02_DigSel:1;	/* DigitalIN Selector */
    /* 0  -> Coax */
    /* 1  -> Optical */

    unsigned char P03_LineLED:1;	/* LineLED */
    /* 0 -> Off */
    /* 1 -> On */
    unsigned char P04_unused:4;	/* unused */
  } Bits;
} PCA_CFG;

/*! \fn =======================================================================
 Function    : WritePCA
-------------------------------------------------------------------------------
 Description : 
 Returns     : NTSTATUS  -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
WritePCA (envy24ht_devc * devc)
{
  PCA_CFG tCFG;
  BYTE bByte[2];

  tCFG.cReg = 0;

  switch (devc->m_LineInSource)
    {
    case SRC_AUX:
      tCFG.Bits.P00_SourceSel = 0;
      break;
    case SRC_WTL:
      tCFG.Bits.P00_SourceSel = 1;
      break;
    case SRC_LINE_REAR:
      tCFG.Bits.P00_SourceSel = 2;
      break;
    case SRC_GROUND:
      tCFG.Bits.P00_SourceSel = 3;
      break;

    }

  /* Switch LineLED when Line is selected for record */
  if (devc->m_ADCIndex == ADC_LINE)
    tCFG.Bits.P03_LineLED = 1;
  if (devc->m_DigInSource == DIGIN_OPTICAL)
    tCFG.Bits.P02_DigSel = 1;
  /* Set all I/Os to Output */
  bByte[0] = 3;
  bByte[1] = 0;
  IICWriteBuffer (devc, AUREON_PCA_BASEBOARD, bByte, 2, 30);
  /* Write config */
  bByte[0] = 0x01;
  bByte[1] = tCFG.cReg;
  IICWriteBuffer (devc, AUREON_PCA_BASEBOARD, bByte, 2, 30);
}

/*! \fn =======================================================================
 Function    : SetDigInSource
-------------------------------------------------------------------------------
 Description : 
 Returns     : void  -> 
 Parameters  : IN ULONG Source -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
SetDigInSource (envy24ht_devc * devc, IN ULONG Source)
{
  switch (Source)
    {
    case DIGIN_OPTICAL:
    case DIGIN_COAX:
      WriteCS8415 (devc, CS_CONTROL_2, 0x01);
      break;
    case DIGIN_CD_IN:
      WriteCS8415 (devc, CS_CONTROL_2, 0x00);
      break;
    }
  devc->m_DigInSource = Source;
  WritePCA (devc);
}

/*! \fn =======================================================================
 Function    : SetOUT0Source
-------------------------------------------------------------------------------
 Description : 
 Returns     : void  -> 
 Parameters  : IN ULONG Source -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
SetOUT0Source (envy24ht_devc * devc, IN ULONG Source)
{
  ULONG ulShiftL, ulShiftR;
  ULONG aulShiftL[] = { 0, 8, 11, 14, 17 };
  ULONG aulShiftR[] = { 3, 20, 23, 26, 29 };
  ULONG ulSourceL, ulSourceR;
  int i;

  /* No DigMonitor when Clock is internal */
  if ((devc->m_ClockSource == ICE_INTERNAL_CLOCK) && (Source == SRC_SPDIN))
    return;
  for (i = 0; i < 5; i++)
    {
      ulShiftL = aulShiftL[i];
      ulShiftR = aulShiftR[i];
      switch (Source)
	{
	case SRC_DMA1:
	  ulSourceL = SRC_PDMA;
	  ulSourceR = SRC_PDMA;
	  break;
	case SRC_PSDIN0:
	  ulSourceL = SRC_PSDIN0_L;
	  ulSourceR = SRC_PSDIN0_R;
	  break;
	case SRC_SPDIN:
	  ulSourceL = SRC_SPDIN_L;
	  ulSourceR = SRC_SPDIN_R;
	  break;
	default:		/* Do nothing */
	  return;
	}
      /* First reset all relevant bits */
      ReadModifyWritePort (devc, REG_MT, MT_LOOPBK_CONTROL,
			   SRC_MASK << ulShiftL, WIDTH_DWORD | BITS_OFF);
      ReadModifyWritePort (devc, REG_MT, MT_LOOPBK_CONTROL,
			   SRC_MASK << ulShiftR, WIDTH_DWORD | BITS_OFF);
      /* ..and set routing mask */
      ReadModifyWritePort (devc, REG_MT, MT_LOOPBK_CONTROL,
			   ulSourceL << ulShiftL, WIDTH_DWORD | BITS_ON);
      ReadModifyWritePort (devc, REG_MT, MT_LOOPBK_CONTROL,
			   ulSourceR << ulShiftR, WIDTH_DWORD | BITS_ON);
    }
  devc->m_Out0Source = Source;
}
static void SetADCMux (envy24ht_devc * devc, IN ULONG Value);

/*! \fn =======================================================================
 Function    : SetLineSource
-------------------------------------------------------------------------------
 Description : 
 Parameters  : IN ULONG Source -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
SetLineSource (envy24ht_devc * devc, IN ULONG Source)
{
  switch (Source)
    {
    case SRC_AUX:
      devc->m_AuxMux = 0x00;
      break;
    case SRC_WTL:
      devc->m_AuxMux = 0x44;
      break;
    case SRC_LINE_REAR:
      devc->m_AuxMux = 0x66;
      break;

    }
  /* Update ADCMux */
  SetADCMux (devc, devc->m_ADCMux);
  devc->m_LineInSource = Source;
  WritePCA (devc);
}

/*! \fn =======================================================================
 Function    : SetClockSource
-------------------------------------------------------------------------------
 Description : 
 Parameters  : IN ULONG ClockSource -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
SetClockSource (envy24ht_devc * devc, IN ULONG ClockSource)
{

  if (ClockSource == ICE_INTERNAL_CLOCK)
    {
      ReadModifyWritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, 0x10,
			   WIDTH_BYTE | BITS_OFF);
      /* Disable DigMonitor to avoid noisy output */
      if (devc->m_Out0Source == SRC_SPDIN)
	SetOUT0Source (devc, SRC_DMA1);
    }
  else
    {
      ReadModifyWritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, 0x10,
			   WIDTH_BYTE | BITS_ON);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
    }
  devc->m_ClockSource = ClockSource;
}

/*! \fn =======================================================================
  Function    : ResetGPIO
-------------------------------------------------------------------------------
  Description : 
  Returns     : void  -> 
-------------------------------------------------------------------------------
  Notes       : 
=============================================================================*/
static void
ResetGPIO (envy24ht_devc * devc)
{

  /* Enable all lower GPIOs */
  WritePort (devc, REG_CCS, WKN_GPIO_WRT_MASK0, 0x0, WIDTH_WORD);
  /* Enable all upper GPIOs */
  WritePort (devc, REG_CCS, WKN_GPIO_WRT_MASK2, 0x0, WIDTH_BYTE);
  /* Set GPIO direction  */
  if (devc->subvendor == SSID_AUREON_UNIVERSE)
    {
      /* -> all output except GPIO_CS8415_CDTO_MASK  */
      /* and GPIO_REMOTE_MASK */
      WritePort (devc, REG_CCS, WKN_GPIO_DIR0,
		 ~(GPIO_CS8415_CDTO_MASK | GPIO_REMOTE_MASK), WIDTH_DWORD);
    }
  else
    {
      /* All output except GPIO_CS8415_CDTO_MASK */
      WritePort (devc, REG_CCS, WKN_GPIO_DIR0, ~GPIO_CS8415_CDTO_MASK,
		 WIDTH_DWORD);
    }

  oss_udelay (100);

    /*----------------------------------------------------------------------------
        Reset AC97 Interface  
    -----------------------------------------------------------------------------*/

  WriteGPIO (devc, GPIO_AC97_RESET_MASK, GPIO_AC97_RESET_MASK);
  oss_udelay (3);
  WriteGPIO (devc, 0, GPIO_AC97_RESET_MASK);
  oss_udelay (3);
  WriteGPIO (devc, GPIO_AC97_RESET_MASK, GPIO_AC97_RESET_MASK);

    /*----------------------------------------------------------------------------
        Enable Remote-Control Interrupts  
    -----------------------------------------------------------------------------*/
  WriteGPIO (devc, GPIO_INT_REST_MASK, GPIO_INT_REST_MASK);

  /* Select optical input */
  /* WriteGPIO(devc, GPIO_DIGITAL_SEL, 0); */
}

/*! \fn =======================================================================
 Function    : SetSPDIFConfig
-------------------------------------------------------------------------------
 Description : 
 Returns     : void  -> 
 Parameters  : IN ULONG Config  -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
SetSPDIFConfig (envy24ht_devc * devc, IN ULONG Config)
{
  ReadModifyWritePort (devc, REG_CCS, WKN_SPDIF_CONFIG, 0x80,
		       WIDTH_BYTE | BITS_OFF);
  if (Config & SPDIF_ENABLE_MASK)
    {
      devc->m_f1724SPDIF = 1;
    }
  else
    {
      devc->m_f1724SPDIF = 0;
    }
  /* Reset SPDIF Config */
  ReadModifyWritePort (devc, REG_MT, MT_SPDIF_REG, 0x000F,
		       WIDTH_WORD | BITS_OFF);
  /* Set new Config */
  ReadModifyWritePort (devc, REG_MT, MT_SPDIF_REG, Config & 0xFFFF,
		       WIDTH_WORD | BITS_ON);
  devc->m_SPDIFConfig = Config;
  if (devc->m_f1724SPDIF)
    ReadModifyWritePort (devc, REG_CCS, WKN_SPDIF_CONFIG, 0x80,
			 WIDTH_BYTE | BITS_ON);
  /* Force saving of registers */
  /* devc->m_pAdapter->SetDirty(); *//* TODO */
}

/*! \fn =======================================================================
 Function    : SetFrontjack
-------------------------------------------------------------------------------
 Description : 
 Parameters  : IN ULONG Frontjack -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
SetFrontjack (envy24ht_devc * devc, IN ULONG Frontjack)
{

  switch (Frontjack)
    {
    case FRONT_JACK_LINEIN:
      WriteGPIO (devc, 0, GPIO_HEADPHONE_MASK);
      break;
    case FRONT_JACK_HEADPHONE:
      WriteGPIO (devc, GPIO_HEADPHONE_MASK, GPIO_HEADPHONE_MASK);
      break;
    }
  devc->m_Frontjack = Frontjack;
  /* Force saving of registers */
  /* devc->m_pAdapter->SetDirty(); *//* TODO */

}

/*! \fn =======================================================================
 Function    : Init1724
-------------------------------------------------------------------------------
 Description : 
 Returns     : void  -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
Init1724 (envy24ht_devc * devc)
{
  /* System Config: 4 DACs, one ADC & SPDIF, MPU enabled, 24,576Mhz crystal */
  /* WritePort(REG_CCS, WKN_SYS_CONFIG, 0x2B, WIDTH_BYTE); */
  /* CL_NOTE: New Setings */
  /* System Config: 4 DACs, one ADC & SPDIF, MPU disabled, 24,576Mhz crystal */
  WritePort (devc, REG_CCS, WKN_SYS_CONFIG, 0x0B, WIDTH_BYTE);

  /* Config I2S Interface */
  WritePort (devc, REG_CCS, WKN_ACLINK_CONFIG, 0x80, WIDTH_BYTE);
  WritePort (devc, REG_CCS, WKN_I2S_CONFIG, 0xF9, WIDTH_BYTE);
  WritePort (devc, REG_CCS, WKN_SPDIF_CONFIG, 0x83, WIDTH_BYTE);

  /* Config Interrupt behaviour */
  WritePort (devc, REG_MT, MT_PLAY_REC_UNDOVR, 0, WIDTH_BYTE);
  WritePort (devc, REG_MT, MT_INTR_STATUS_REG, 0, WIDTH_BYTE);
  WritePort (devc, REG_MT, MT_INTR_MASK_REG, 0xFF, WIDTH_BYTE);
  WritePort (devc, REG_CCS, WKN_INT_STAT_REG, 0, WIDTH_BYTE);
  WritePort (devc, REG_CCS, WKN_INT_MASK_REG, 0xFE, WIDTH_BYTE);

  SetSPDIFConfig (devc, 0);
  /* SetSDPIFSource(devc, DIGOUT_WAVE); */
  SetDigInSource (devc, DIGIN_OPTICAL);
  SetFrontjack (devc, FRONT_JACK_LINEIN);
}

/*! \fn =======================================================================
 Function    : WriteWM8770
-------------------------------------------------------------------------------
 Description : 
 Parameters  : IN UCHAR Register -> 
             : IN UCHAR Value -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
void
WriteWM8770 (envy24ht_devc * devc, IN UCHAR Register, IN USHORT Value)
{
  int i;
  BOOLEAN fData;
  USHORT wCmd;
/*    KIRQL       OldIrql; */

  /* m_pAdapter->HwEnter(); */

  /* KeAcquireSpinLock (&m_SPILock,&OldIrql); */


  /* Clock low to prevent IIC Startcondition */
  WriteGPIO (devc, 0, GPIO_SCL_MASK);

  SetSDA (devc, 0);

  /* Chip select (CS low) */
  WriteGPIO (devc, 0, GPIO_WM8770_CS_MASK);

  /* format buffer */
  wCmd = (((USHORT) Register) << 9) + Value;


  for (i = 0; i < 16; i++)
    {
      fData = (wCmd & 0x8000) ? 1 : 0;
      /* CCLK -> low */
      WriteGPIO (devc, 0, GPIO_SCL_MASK);
      oss_udelay (3);
      /* CDTI -> Set data */
      SetSDA (devc, fData);
      oss_udelay (3);
      /* CCLK -> high */
      WriteGPIO (devc, GPIO_SCL_MASK, GPIO_SCL_MASK);
      wCmd <<= 1;
    }
  WriteGPIO (devc, 0, GPIO_SCL_MASK);	/* CCLK -> low */

  /* Chip deselect */
  WriteGPIO (devc, GPIO_WM8770_CS_MASK, GPIO_WM8770_CS_MASK);
}

/*! \fn =======================================================================
 Function    : SetVolReg
-------------------------------------------------------------------------------
 Description : 
 Parameters  : IN ULONG LineIdx -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
SetVolReg (envy24ht_devc * devc, IN ULONG LineIdx)
{
  int iVol, ChnlVol;

  ChnlVol = devc->m_DACVolume[LineIdx];

  /* See if we want to mute anything */
  if (devc->m_fDACMute[LineIdx] || devc->m_fDACMute[LINE_MASTER])
    {
      WriteWM8770 (devc, gWMRegister[LineIdx], 0x100);
      return;
    }

  /* Since master volume is virtualized, we add the attenuations for both */
  /* master volume and channel volume to obtain the overall attenuation */

  /* Get total attenuation */
  iVol = ChnlVol + devc->m_DACVolume[LINE_MASTER] - MAX_VOLUME;
  /* Check against bounds */
  iVol = (iVol < MAX_VOLUME) ? iVol : MAX_VOLUME;
  if (iVol < MIN_VOLUME)
    iVol = MIN_VOLUME;
  WriteWM8770 (devc, gWMRegister[LineIdx], iVol | 0x180);
}


/*! \fn =======================================================================
 Function    : SetMute
-------------------------------------------------------------------------------
 Description : 
 Returns     : VOID  -> 
 Parameters  : IN ULONG ChannelID -> 
             : IN BOOLEAN Mute -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void SetDACMute
  (envy24ht_devc * devc, IN ULONG ChannelID, IN BOOLEAN Mute)
{
  ULONG LineIndex;

  /* Convert ChannelID to line index */
  /* If it is for left channel, we will need to add one */
  LineIndex = ChannelID >> 15;

  /* See if this is for master volume */
  if (LineIndex == LINE_MASTER)
    {
      /* if current setting is not the same as previous setting */
      if (devc->m_fDACMute[LINE_MASTER] != Mute)
	{
	  int i;

	  devc->m_fDACMute[LINE_MASTER] = Mute;

	  /* Need to do it for every single line (excluding Master and Gain) */
	  for (i = 0; i <= LINE_OUT_4R; i++)
	    {
	      SetVolReg (devc, i);
	    }
	  return;
	}
    }

  /* See if this is for left channel */
  if (ChannelID & CH_LEFT)
    {
      /* if current setting is not the same as previous setting */
      if (devc->m_fDACMute[LineIndex] != Mute)
	{
	  devc->m_fDACMute[LineIndex] = Mute;
	  if (LineIndex == LINE_GAIN_L)
	    {
	      WriteWM8770 (devc, gWMRegister[LINE_GAIN_L],
			   devc->
			   m_DACVolume[LINE_GAIN_L] | ((Mute) ? 0x20 : 0x00));
	    }
	  else
	    {
	      SetVolReg (devc, LineIndex);
	    }
	}
    }

  /* See if this is for right channel */
  if (ChannelID & CH_RIGHT)
    {
      LineIndex++;
      /* if current setting is not the same as previous setting */
      if (devc->m_fDACMute[LineIndex] != Mute)
	{
	  devc->m_fDACMute[LineIndex] = Mute;
	  if (LineIndex == LINE_GAIN_R)
	    {
	      WriteWM8770 (devc, gWMRegister[LINE_GAIN_R],
			   devc->
			   m_DACVolume[LINE_GAIN_R] | ((Mute) ? 0x20 : 0x00));
	    }
	  else
	    {
	      SetVolReg (devc, LineIndex);
	    }
	}
    }
  /* m_pAdapter->SetDirty(); *//* TODO */
}



/*=============================================================================
  Function    : SetVolume
-------------------------------------------------------------------------------
  Description : 
  Returns     : VOID  -> 
  Parameters  : IN  ICE_HW_PARAM*   HwParm -> 
-------------------------------------------------------------------------------
  Notes       : 
=============================================================================*/
static void SetDACVolume
  (envy24ht_devc * devc, IN ULONG ChannelID, IN UCHAR Volume)
{
  ULONG LineIndex;
  WORD LeftRight;
  BOOLEAN VolChnged = 0;

  /* Convert ChannelID to line index */
  LineIndex = ChannelID >> 15;
  /* Get the left/right side */
  LeftRight = (WORD) (ChannelID & 0xffff);

  /* Check if left volume is changed */
  if (LeftRight & CH_LEFT)
    {
      if (devc->m_DACVolume[LineIndex] != Volume)
	{
	  devc->m_DACVolume[LineIndex] = Volume;
	  VolChnged = 1;
	}
    }

  /* Check if right volume is changed */
  if (LeftRight & CH_RIGHT)
    {
      if (devc->m_DACVolume[LineIndex + 1] != Volume)
	{
	  devc->m_DACVolume[LineIndex + 1] = Volume;
	  VolChnged = 1;
	}
    }

  /* If any volume is changed, need to touch hardware */
  if (VolChnged)
    {
      /* check if this is for input gain */
      if ((ChannelID >> 16) == IN_12)
	{
	  USHORT WMValue = (USHORT) Volume;
#ifdef NULL_DB
	  WMValue = 0x0C;
#endif
	  if (LeftRight & CH_LEFT)
	    WriteWM8770 (devc, gWMRegister[LineIndex], WMValue);
	  if (LeftRight & CH_RIGHT)
	    WriteWM8770 (devc, gWMRegister[LineIndex + 1], WMValue);
	}
      else
	/* Yap, now check if this is for master volume */
      if ((ChannelID >> 16) == OUT_MASTER)
	{
	  int i;
	  /* Need to do it for every single line (excluding Master and Gain) */
	  for (i = 0; i <= LINE_OUT_4R; i++)
	    {
	      SetVolReg (devc, i);
	    }
	}
      else
	{
	  if (LeftRight & CH_LEFT)
	    SetVolReg (devc, LineIndex);
	  if (LeftRight & CH_RIGHT)
	    SetVolReg (devc, LineIndex + 1);
	}
    }
  /* m_pAdapter->SetDirty(); TODO */
}


/*! \fn =======================================================================
 Function    : GetVolume
-------------------------------------------------------------------------------
 Description : 
 Returns     : UCHAR  -> 
 Parameters  : IN ULONG    ChannelID -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static UCHAR
GetDACVolume (envy24ht_devc * devc, IN ULONG ChannelID)
{
  UCHAR Value;
  USHORT LeftRight;
  ULONG LineIndex;

  /* Convert ChannelID to line index */
  LineIndex = ChannelID >> 15;
  /* Get the left/right side */
  LeftRight = (USHORT) (ChannelID & 0xffff);

  if (LeftRight == CH_LEFT)
    {
      Value = devc->m_DACVolume[LineIndex];
    }
  else
    {
      Value = devc->m_DACVolume[LineIndex + 1];
    }
  return Value;
}


#if 0
/*! \fn =======================================================================
 Function    : GetMute
-------------------------------------------------------------------------------
 Description : 
 Returns     : BOOLEAN  -> 
 Parameters  : IN ULONG    ChannelID -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static BOOLEAN
GetDACMute (envy24ht_devc * devc, IN ULONG ChannelID)
{
  BOOLEAN Value;
  USHORT LeftRight;
  ULONG LineIndex;

  /* Convert ChannelID to line index */
  LineIndex = ChannelID >> 15;
  /* Get the left/right side */
  LeftRight = (USHORT) (ChannelID & 0xffff);

  if (LeftRight == CH_LEFT)
    {
      Value = devc->m_fDACMute[LineIndex];
    }
  else
    {
      Value = devc->m_fDACMute[LineIndex + 1];
    }
  return Value;
}
#endif

/*! \fn =======================================================================
 Function    : SetADCGain
-------------------------------------------------------------------------------
 Description : 
 Parameters  : IN ULONG Index -> 
             : IN ULONG Value -> 
             : IN ULONG Channel -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void SetADCGain
  (envy24ht_devc * devc, IN ULONG Index, IN USHORT Value, IN ULONG Channel)
{
  UCHAR WMReg = 0x19;
  USHORT WMValue;

  /* Set only selected Line */
  if (Index != devc->m_ADCIndex)
    return;

  switch (Channel)
    {
    case CH_LEFT:
      devc->m_ADCVolume[Index] =
	(Value << 8) | (devc->m_ADCVolume[Index] & 0x00FF);
      break;
    case CH_BOTH:
      devc->m_ADCVolume[Index] = (Value << 8) | Value;
      break;
    case CH_NOP:
      /* Hold Value */
      break;
    case CH_RIGHT:
    default:
      devc->m_ADCVolume[Index] = Value | (devc->m_ADCVolume[Index] & 0xFF00);

    }
  WMValue =
    ((devc->m_ADCVolume[Index] & 0x00FF) <
     0x1F) ? (devc->m_ADCVolume[Index] & 0x00FF) : 0x1F;
#ifdef NULL_DB
  WMValue = 0x0C;
#endif
  WriteWM8770 (devc, WMReg, WMValue);
  WMValue = (((devc->m_ADCVolume[Index] >> 8) & 0x00FF) < 0x1F) ?
    ((devc->m_ADCVolume[Index] >> 8) & 0x00FF) : 0x1F;
#ifdef NULL_DB
  WMValue = 0x0C;
#endif
  WriteWM8770 (devc, WMReg + 1, WMValue);
  /* Force saving of registers */
  /* devc->m_pAdapter->SetDirty(); TODO */
}

#if 0
/*! \fn =======================================================================
 Function    : GetADCGain
-------------------------------------------------------------------------------
 Description : 
 Returns     : UCHAR  -> 
 Parameters  : IN ULONG    Index -> 
             : IN ULONG    Channel -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static UCHAR GetADCGain
  (envy24ht_devc * devc, IN ULONG Index, IN ULONG Channel)
{
  UCHAR Value;

  if (Channel == CH_LEFT)
    {
      Value = (UCHAR) ((devc->m_ADCVolume[Index] >> 8) & 0xFF);
    }
  else
    {
      Value = (UCHAR) (devc->m_ADCVolume[Index] & 0xFF);
    }
  return Value;
}
#endif

/*! \fn =======================================================================
 Function    : SetADCMux
-------------------------------------------------------------------------------
 Description : 
 Parameters  : IN Value -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
SetADCMux (envy24ht_devc * devc, IN ULONG Value)
{
  UCHAR MuxVal = 0;
  BOOLEAN fAU;

  /* Store to shadow register */
  devc->m_ADCMux = Value;
  devc->m_fSPDIFRecord = 0;
  fAU = (devc->subvendor == SSID_AUREON_UNIVERSE);
  switch (Value)
    {
    case CD_IN_MUX_TP_PIN:
      devc->m_ADCIndex = ADC_CD;
      MuxVal = (fAU) ? 0x11 : 0x00;
      break;
    case LINE_IN_MUX_TP_PIN:
      devc->m_ADCIndex = ADC_LINE;
      MuxVal = (fAU) ? 0x33 : 0x22;
      break;
    case AUX_IN_MUX_TP_PIN:
      devc->m_ADCIndex = ADC_AUX;
      MuxVal = (fAU) ? devc->m_AuxMux : 0x11;
      break;
    case MIC_IN_MUX_TP_PIN:
      devc->m_ADCIndex = ADC_MIC;
      MuxVal = (fAU) ? 0x55 : 0x33;
      break;
    case DIG_IN_MUX_TP_PIN:
      /* Use SPDIF DMA channel */
      devc->m_fSPDIFRecord = 1;
      break;
    case PHONO_IN_MUX_PIN:
      devc->m_ADCIndex = ADC_PHONO;
      MuxVal = 0x22;
      break;
    case STEREO_MIX_MUX_TP_PIN:
      devc->m_ADCIndex = ADC_STEREO_MIX;
      MuxVal = (fAU) ? 0x77 : 0x44;
      break;

    default:
      devc->m_ADCIndex = ADC_LINE;
      MuxVal = 0x22;

    }
  WriteWM8770 (devc, WM_ADC_INPUT_MX, (UCHAR) MuxVal);
  /* Reset GAIN */
  SetADCGain (devc, devc->m_ADCIndex, 0, CH_NOP);
  /* Update PCA config */
  WritePCA (devc);
  /* Force saving of registers */
  /*devc->m_pAdapter->SetDirty(); TODO */
}

/*! \fn =======================================================================
 Function    : InitWM8770
-------------------------------------------------------------------------------
 Description : 
-------------------------------------------------------------------------------
 Notes       : Call before Registry Read
=============================================================================*/
static void
InitWM8770 (envy24ht_devc * devc)
{
    /*----------------------------------------------------------------------------
        Reset WM8770   
    -----------------------------------------------------------------------------*/
  /* Set SPI Mode */
  WriteGPIO (devc, 0, GPIO_WM8770_RS_MASK);
  oss_udelay (3);
  WriteGPIO (devc, GPIO_WM8770_CS_MASK, GPIO_WM8770_CS_MASK);
  oss_udelay (3);
  WriteGPIO (devc, GPIO_WM8770_RS_MASK, GPIO_WM8770_RS_MASK);
  oss_udelay (100);

    /*----------------------------------------------------------------------------
        Set defaults   
    -----------------------------------------------------------------------------*/

  /* Output defaults */
  SetDACVolume (devc, CH_MASTER_BOTH, WM_OUT_DEFAULT);
  SetDACVolume (devc, CH_FRONT_BOTH, WM_OUT_DEFAULT);
  SetDACVolume (devc, CH_REAR_BOTH, WM_OUT_DEFAULT);
  SetDACVolume (devc, CH_CENTER, WM_OUT_DEFAULT);
  SetDACVolume (devc, CH_LFE, WM_OUT_DEFAULT);
  SetDACVolume (devc, CH_BS_BOTH, WM_OUT_DEFAULT);
  SetDACMute (devc, CH_MASTER_BOTH, 0);
  SetDACMute (devc, CH_FRONT_BOTH, 0);
  SetDACMute (devc, CH_REAR_BOTH, 0);
  SetDACMute (devc, CH_CENTER, 0);
  SetDACMute (devc, CH_LFE, 0);
  SetDACMute (devc, CH_BS_BOTH, 0);
  /* Input */
  SetADCGain (devc, ADC_CD, WM_INP_DEFAULT, CH_BOTH);
  SetADCGain (devc, ADC_AUX, WM_INP_DEFAULT, CH_BOTH);
  SetADCGain (devc, ADC_LINE, WM_INP_DEFAULT, CH_BOTH);
  SetADCGain (devc, ADC_MIC, WM_INP_DEFAULT, CH_BOTH);
  /* Mux */
  SetADCMux (devc, STEREO_MIX_MUX_TP_PIN);

  /* At least Power DAC & ADC */
  WriteWM8770 (devc, WM_POWER_CNTRL, 0x0000);
  /* Power ADC Input Mux */
  WriteWM8770 (devc, WM_ADC_INPUT_MX, 0x0000);
  /* Enable Aux-Output */

  if (devc->subvendor == SSID_PHASE28)
    {
      WriteWM8770 (devc, WM_OUT12_SELECT, 0x0009);
      WriteWM8770 (devc, WM_OUT34_SELECT, 0x0009);
    }
  else
    {
      WriteWM8770 (devc, WM_OUT12_SELECT, 0x000b);
      WriteWM8770 (devc, WM_OUT34_SELECT, 0x0009);
    }
  /* Init Master Mode Register */
  WriteWM8770 (devc, WM_MASTER_MODE_CNTRL, 0x0012);

}

/*! \fn =======================================================================
 Function    : InitCS8415
-------------------------------------------------------------------------------
 Description : 
 Returns     : void  -> 
-------------------------------------------------------------------------------
 Notes       : Has to be called after InitWM8770 since RST must be "high"
=============================================================================*/
static void
InitCS8415 (envy24ht_devc * devc)
{

  /* Init Remote-Controller */
  if (devc->subvendor == SSID_AUREON_UNIVERSE)
    {
      UCHAR bByte[2];
      bByte[0] = (UCHAR) ((AUREON_REMOTE_ID >> 8) & 0xFF);
      bByte[1] = (UCHAR) AUREON_REMOTE_ID;
      IICWriteBuffer (devc, AUREON_REMOTE_CNTRL, bByte, 2, 200);
    }
    /*----------------------------------------------------------------------------
        Reset CS8415   
    -----------------------------------------------------------------------------*/
  /* Set SPI Mode */
  WriteGPIO (devc, GPIO_CS8415_CS_MASK, GPIO_CS8415_CS_MASK);
  oss_udelay (3);
  WriteGPIO (devc, 0, GPIO_CS8415_CS_MASK);
  oss_udelay (100);

  /* Set defaults */
  WriteCS8415 (devc, CS_CONTROL_1, 0x80);
  /* SPDIF mux to RXP1 */
  WriteCS8415 (devc, CS_CONTROL_2, 0x01);
  WriteCS8415 (devc, CS_CLK_SRC_CNTRL, 0x41);
  WriteCS8415 (devc, CS_SER_OUT_FORMAT, 0x05);

  /* all other register remain to their defaults */

}

#define GPIO_WAIT 10

/*! \fn =======================================================================
 Function    : WriteAC97Codec
-------------------------------------------------------------------------------
 Description : Writes to TT-AC97 Interface
 Parameters  : IN  ULONG       Register -> 
             : IN  ULONG      Data -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void WriteAC97Codec
  (envy24ht_devc * devc, IN ULONG Register, IN ULONG Data)
{
  UCHAR TTData;

#ifdef TTTT
  /* CL_TEST */
  /* Reset AC97 */
  WriteGPIO (devc, GPIO_AC97_RESET_MASK, GPIO_AC97_RESET_MASK);
  oss_udelay (3);
  WriteGPIO (devc, 0, GPIO_AC97_RESET_MASK);
  oss_udelay (3);
  WriteGPIO (devc, GPIO_AC97_RESET_MASK, GPIO_AC97_RESET_MASK);
  oss_udelay (3);
  /* Unmute Master */
  WriteAC97Codec (devc, 0x02, 0x00);
#endif

  /* m_pAdapter->HwEnter(); TODO */

  oss_udelay (GPIO_WAIT);

  /* Reset all LD Pins and GO Pin */
  WriteGPIO (devc, 0,
	     GPIO_LD_DATA_H_MASK | GPIO_LD_DATA_L_MASK | GPIO_LD_ADR_MASK |
	     GPIO_GO_MASK);


  /* apply address to TT-AC97 Interface */
  WriteGPIO (devc, Register, GPIO_DATA_ADR_MASK);

  /* Set "Load Address" Pin */
  oss_udelay (GPIO_WAIT);
  WriteGPIO (devc, GPIO_LD_ADR_MASK, GPIO_LD_ADR_MASK);

  /* Reset "Load Address" Pin */
  oss_udelay (GPIO_WAIT);
  WriteGPIO (devc, 0, GPIO_LD_ADR_MASK);

  /* apply data low to TT-AC97 Interface */
  oss_udelay (GPIO_WAIT);
  TTData = (UCHAR) (Data & 0x000000FF);
  WriteGPIO (devc, TTData, GPIO_DATA_ADR_MASK);

  /* Set "Load Data low" Pin */
  oss_udelay (GPIO_WAIT);
  WriteGPIO (devc, GPIO_LD_DATA_L_MASK, GPIO_LD_DATA_L_MASK);

  /* Reset "Load Data low" Pin */
  oss_udelay (GPIO_WAIT);
  WriteGPIO (devc, 0, GPIO_LD_DATA_L_MASK);

  /* apply data high to TT-AC97 Interface */
  oss_udelay (GPIO_WAIT);
  TTData = (UCHAR) ((Data >> 8) & 0x000000FF);
  WriteGPIO (devc, TTData, GPIO_DATA_ADR_MASK);

  /* Set "Load Data high" Pin */
  oss_udelay (GPIO_WAIT);
  WriteGPIO (devc, GPIO_LD_DATA_H_MASK, GPIO_LD_DATA_H_MASK);

  /* Reset "Load Data high" Pin */
  oss_udelay (GPIO_WAIT);
  WriteGPIO (devc, 0, GPIO_LD_DATA_H_MASK);

  /* Set and immediately reset "GO" Pin */
  oss_udelay (GPIO_WAIT);
  WriteGPIO (devc, GPIO_GO_MASK, GPIO_GO_MASK);
  WriteGPIO (devc, 0, GPIO_GO_MASK);
  /* m_pAdapter->HwLeave(); *//* TODO */

}

/*! \fn =======================================================================
 Function    : SetAC97Volume
-------------------------------------------------------------------------------
 Description : 
 Parameters  : IN ULONG    Index -> 
             : IN UHSORT   Value -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void SetAC97Volume
  (envy24ht_devc * devc, IN ULONG Index, IN USHORT left, IN USHORT right)
{
  int AC97Reg = 0, value;

  devc->m_AC97Volume[Index] = left | (right << 8);

  left = AC97_INP_MAX - left;
  right = AC97_INP_MAX - right;

  value = (left << 8) | right;

  switch (devc->subvendor)
    {
    case SSID_AUREON_SKY:
    case SSID_PRODIGY71:
    case SSID_AUREON_SPACE:
      switch (Index)
	{
	case AC97_MIC:
	  AC97Reg = AC97_IDXREG_MIC_IN;
	  break;
	case AC97_LINE:
	  AC97Reg = AC97_IDXREG_LINE_IN;
	  break;
	case AC97_CD:
	  AC97Reg = AC97_IDXREG_CD_IN;
	  break;
	case AC97_AUX:
	  AC97Reg = AC97_IDXREG_AUX_IN;
	  break;
	default:
	  cmn_err (CE_CONT, "Aureon: Bad index %d\n", Index);
	  return;
	}
      break;
    case SSID_AUREON_UNIVERSE:
      switch (Index)
	{
	case AC97_MIC:
	  AC97Reg = AC97_IDXREG_MIC_IN;
	  break;
	case AC97_LINE:
	  AC97Reg = AC97_IDXREG_LINE_IN;
	  break;
	case AC97_CD:
	  AC97Reg = AC97_IDXREG_AUX_IN;
	  break;
	case AC97_PHONO:
	  AC97Reg = AC97_IDXREG_CD_IN;
	  break;
	case AC97_AUX:
	case AC97_LINE2:
	  AC97Reg = AC97_IDXREG_VIDEO_IN;
	  break;
	default:
	  cmn_err (CE_CONT, "Aureon: Bad index %d\n", Index);
	  return;

	}
      break;
    }
  if (!devc->m_fAC97Mute[Index])
    {
      WriteAC97Codec (devc, AC97Reg, value);
    }
  /* Force saving of registers */
  /* m_pAdapter->SetDirty(); TODO */
}


/*! \fn =======================================================================
 Function    : GetAC97Volume
-------------------------------------------------------------------------------
 Description : 
 Returns     : UCHAR  -> 
 Parameters  : IN ULONG    Index -> 
             : IN ULONG    Channel -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static USHORT
GetAC97Volume (envy24ht_devc * devc, IN ULONG Index)
{
  return devc->m_AC97Volume[Index];
}

/*! \fn =======================================================================
 Function    : SetAC97Mute
-------------------------------------------------------------------------------
 Description : 
 Returns     : void  -> 
 Parameters  : IN ULONG    Index -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
SetAC97Mute (envy24ht_devc * devc, IN ULONG Index, BOOLEAN OnOff)
{
  UCHAR AC97Reg = 0;
  devc->m_fAC97Mute[Index] = OnOff;
  switch (devc->subvendor)
    {
    case SSID_AUREON_SKY:
    case SSID_PRODIGY71:
    case SSID_AUREON_SPACE:
      switch (Index)
	{
	case AC97_MIC:
	  AC97Reg = AC97_IDXREG_MIC_IN;
	  break;
	case AC97_LINE:
	  AC97Reg = AC97_IDXREG_LINE_IN;
	  break;
	case AC97_CD:
	  AC97Reg = AC97_IDXREG_CD_IN;
	  break;
	case AC97_AUX:
	  AC97Reg = AC97_IDXREG_AUX_IN;
	  break;
	default:
	  cmn_err (CE_CONT, "Aureon: Bad index %d\n", Index);
	  return;
	}
      break;
    case SSID_AUREON_UNIVERSE:
      switch (Index)
	{
	case AC97_MIC:
	  AC97Reg = AC97_IDXREG_MIC_IN;
	  break;
	case AC97_LINE:
	  AC97Reg = AC97_IDXREG_LINE_IN;
	  break;
	case AC97_CD:
	  AC97Reg = AC97_IDXREG_AUX_IN;
	  break;
	case AC97_PHONO:
	  AC97Reg = AC97_IDXREG_CD_IN;
	  break;
	case AC97_AUX:
	case AC97_LINE2:
	  AC97Reg = AC97_IDXREG_VIDEO_IN;
	  break;
	default:
	  cmn_err (CE_CONT, "Aureon: Bad index %d\n", Index);
	  return;
	}
      break;
    }
#ifdef AC97_MUTE
  WriteAC97Codec (devc, AC97Reg, 0x8000);
#else
  WriteAC97Codec (devc, AC97Reg,
		  (OnOff) ? 0x8000 : devc->m_AC97Volume[Index]);
#endif

  /* Force saving of registers */
  /* m_pAdapter->SetDirty(); *//* TODO */
}

/*! \fn =======================================================================
 Function    : InitAC97
-------------------------------------------------------------------------------
 Description : 
 Returns     : void  -> 
-------------------------------------------------------------------------------
 Notes       : Call before Registry Read
=============================================================================*/
static void
InitAC97 (envy24ht_devc * devc)
{
  /* Reset AC97 */
  oss_udelay (30);
  WriteGPIO (devc, GPIO_AC97_RESET_MASK, GPIO_AC97_RESET_MASK);
  oss_udelay (3);
  WriteGPIO (devc, 0, GPIO_AC97_RESET_MASK);
  oss_udelay (3);
  WriteGPIO (devc, GPIO_AC97_RESET_MASK, GPIO_AC97_RESET_MASK);
  oss_udelay (3);

  /* Unmute Master */
  WriteAC97Codec (devc, 0x02, 0x00);

    /*----------------------------------------------------------------------------
        Set defaults   
    -----------------------------------------------------------------------------*/
  /* Volume */
  SetAC97Volume (devc, AC97_MIC, AC97_INP_DEFAULT, AC97_INP_DEFAULT);
  SetAC97Volume (devc, AC97_LINE, AC97_INP_DEFAULT, AC97_INP_DEFAULT);
  SetAC97Volume (devc, AC97_AUX, AC97_INP_DEFAULT, AC97_INP_DEFAULT);
  SetAC97Volume (devc, AC97_CD, AC97_INP_DEFAULT, AC97_INP_DEFAULT);
  /* Mute */
  SetAC97Mute (devc, AC97_MIC, 0);
  SetAC97Mute (devc, AC97_LINE, 0);
  SetAC97Mute (devc, AC97_AUX, 0);
  SetAC97Mute (devc, AC97_CD, 0);
}

/*! \fn =======================================================================
 Function    : SetSampleRate
-------------------------------------------------------------------------------
 Description : 
 Returns     : NTSTATUS  -> 
 Parameters  : IN ULONG SampleRate -> 
-------------------------------------------------------------------------------
 Notes       : 
=============================================================================*/
static void
SetSampleRate (envy24ht_devc * devc, IN ULONG SampleRate)
{
  WriteWM8770 (devc, WM_MASTER_MODE_CNTRL, 0x0012);

  switch (SampleRate)
    {
    case 48000:
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_48KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      /* WRITE_PORT_UCHAR(WKNMTBase + MT_DATA_FORMAT_REG,(~MT_128X)&READ_PORT_UCHAR(WKNMTBase + MT_DATA_FORMAT_REG)); */
      break;
    case 24000:
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_24KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      break;
    case 12000:
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_12KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      break;
    case 9600:
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_9p6KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      break;
    case 32000:
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_32KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      break;
    case 16000:
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_16KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      break;
    case 8000:
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_8KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      break;
    case 96000:
      WriteWM8770 (devc, WM_MASTER_MODE_CNTRL, 0x001A);
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_96KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      break;
    case 192000:
      WriteWM8770 (devc, WM_MASTER_MODE_CNTRL, 0x001A);
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_192KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_ON);
      break;
    case 64000:
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_64KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      break;
    case 44100:
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_44p1KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      break;
    case 22050:
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_22p05KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      break;
    case 11025:
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_11p025KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      break;
    case 88200:
      WriteWM8770 (devc, WM_MASTER_MODE_CNTRL, 0x001A);
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_88p2KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      break;
    case 176400:
      WriteWM8770 (devc, WM_MASTER_MODE_CNTRL, 0x001A);
      WritePort (devc, REG_MT, MT_SAMPLE_RATE_REG, MT_176p4KHZ, WIDTH_BYTE);
      ReadModifyWritePort (devc, REG_MT, MT_DATA_FORMAT_REG, MT_128X,
			   WIDTH_BYTE | BITS_OFF);
      break;
    default:
      break;

    }
}

static void
aureon_card_init (envy24ht_devc * devc)
{

/* Do not change the order of the following lines */
  Init1724 (devc);
  ResetGPIO (devc);

  if (devc->subvendor != SSID_PHASE28)
    InitAC97 (devc);
  InitWM8770 (devc);
  InitCS8415 (devc);
/* Do not change the order of the above lines */

  SetSPDIFConfig (devc, 0);
  /* SetSDPIFSource(devc, DIGOUT_WAVE); */
  SetDigInSource (devc, DIGIN_COAX);
  SetFrontjack (devc, FRONT_JACK_HEADPHONE);
  SetLineSource (devc, SRC_LINE_REAR);
  SetClockSource (devc, ICE_INTERNAL_CLOCK);

}

static void
aureon_set_rate (envy24ht_devc * devc)
{
  SetSampleRate (devc, devc->speed);
}

static int
aureon_set_ctl (int dev, int ctl, unsigned int cmd, int value)
{
  envy24ht_devc *devc = mixer_devs[dev]->devc;

  if (cmd == SNDCTL_MIX_READ)
    {
      switch (ctl)
	{
	case 1:
	  return devc->m_LineInSource;
	case 3:
	  return devc->m_DigInSource;
	case 4:
	  return devc->m_Frontjack;
	}
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (ctl)
	{
	case 1:
	  SetLineSource (devc, value);
	  return devc->m_LineInSource;
	case 3:
	  SetDigInSource (devc, value);
	  return devc->m_DigInSource;
	case 4:
	  SetFrontjack (devc, value);
	  return devc->m_Frontjack;
	}
    }

  return OSS_EINVAL;
}

static int
aureon_set_vol (int dev, int ChannelID, unsigned int cmd, int value)
{
  envy24ht_devc *devc = mixer_devs[dev]->devc;
  ULONG LineIndex;
  WORD LeftRight;
  int left, right;

  /* Convert ChannelID to line index */
  LineIndex = ChannelID >> 15;
  /* Get the left/right side */
  LeftRight = (WORD) (ChannelID & 0xffff);

  if (cmd == SNDCTL_MIX_READ)
    {
      left = right = 0;
      if (LeftRight & CH_LEFT)
	left = GetDACVolume (devc, (LineIndex << 15) | CH_LEFT);
      if (LeftRight & CH_RIGHT)
	right = GetDACVolume (devc, (LineIndex << 15) | CH_RIGHT);

      if (left == 0)
	left = right;
      else if (right == 0)
	right = left;

      return left | (right << 8);
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      left = (value & 0xff);
      right = ((value >> 8) & 0xff);

      if (LeftRight != CH_BOTH)
	right = left;

      if (LeftRight & CH_LEFT)
	SetDACVolume (devc, (LineIndex << 15) | CH_LEFT, left);

      if (LeftRight & CH_RIGHT)
	SetDACVolume (devc, (LineIndex << 15) | CH_RIGHT, right);

      return left | (right << 8);
    }

  return OSS_EINVAL;
}

static int
aureon_set_ac97 (int dev, int Index, unsigned int cmd, int value)
{
  envy24ht_devc *devc = mixer_devs[dev]->devc;
  int left, right;

  if (cmd == SNDCTL_MIX_READ)
    {
      return GetAC97Volume (devc, Index);
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      left = (value & 0xff);
      right = ((value >> 8) & 0xff);

      SetAC97Volume (devc, Index, left, right);

      return left | (right << 8);
    }

  return OSS_EINVAL;
}

 /*ARGSUSED*/ static int
aureon_mixer_init_common (envy24ht_devc * devc, int dev, int root)
{
  int ctl, group;

  if ((group = mixer_ext_create_group (dev, root, "VOL")) < 0)
    return group;

  if ((ctl = mixer_ext_create_control (dev, group,
				       CH_MASTER_BOTH,
				       aureon_set_vol,
				       MIXT_STEREOSLIDER, "MASTER",
				       WM_OUT_MAX,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       CH_FRONT_BOTH,
				       aureon_set_vol,
				       MIXT_STEREOSLIDER, "FRONT", WM_OUT_MAX,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       CH_REAR_BOTH,
				       aureon_set_vol,
				       MIXT_STEREOSLIDER, "SURROUND",
				       WM_OUT_MAX,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       CH_CENTER,
				       aureon_set_vol,
				       MIXT_MONOSLIDER, "CENTER", WM_OUT_MAX,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       CH_LFE,
				       aureon_set_vol,
				       MIXT_MONOSLIDER, "LFE", WM_OUT_MAX,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       CH_BS_BOTH,
				       aureon_set_vol,
				       MIXT_STEREOSLIDER, "REAR", WM_OUT_MAX,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((group =
       mixer_ext_create_group_flags (dev, root, "TERRATEC", MIXF_FLAT)) < 0)
    return group;

  if ((ctl = mixer_ext_create_control (dev, group,
				       1,
				       aureon_set_ctl,
				       MIXT_ENUM, "LINESRC", 4,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;
  mixer_ext_set_strings (dev, ctl, "AUX WTL REAR GROUND", 0);

  if ((ctl = mixer_ext_create_control (dev, group,
				       3,
				       aureon_set_ctl,
				       MIXT_ENUM, "DIGIN", 3,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;
  mixer_ext_set_strings (dev, ctl, "OPTICAL COAX CD", 0);

  return 0;
}

 /*ARGSUSED*/ static int
aureon_mixer_init_universe (envy24ht_devc * devc, int dev, int group)
{
  int ctl;

  if ((group = mixer_ext_create_group (dev, group, "ENVY24_UNIVERSE")) < 0)
    return group;

  if ((ctl = mixer_ext_create_control (dev, group,
				       4,
				       aureon_set_ctl,
				       MIXT_ENUM, "FRONTJACK", 3,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;
  mixer_ext_set_strings (dev, ctl, "LINEIN HEADPH", 0);

  return 0;
}

 /*ARGSUSED*/ static int
aureon_mixer_init_ac97 (envy24ht_devc * devc, int dev, int group)
{
  int ctl;

  if ((group = mixer_ext_create_group (dev, group, "ENVY24_AC97")) < 0)
    return group;

  if ((ctl = mixer_ext_create_control (dev, group,
				       AC97_MIC,
				       aureon_set_ac97,
				       MIXT_STEREOSLIDER, "MIC", AC97_INP_MAX,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       AC97_LINE,
				       aureon_set_ac97,
				       MIXT_STEREOSLIDER, "LINE",
				       AC97_INP_MAX,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       AC97_CD,
				       aureon_set_ac97,
				       MIXT_STEREOSLIDER, "CD", AC97_INP_MAX,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       AC97_PHONO,
				       aureon_set_ac97,
				       MIXT_STEREOSLIDER, "PHONO",
				       AC97_INP_MAX,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       AC97_AUX,
				       aureon_set_ac97,
				       MIXT_STEREOSLIDER, "AUX", AC97_INP_MAX,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  return 0;
}

static int
aureon_mixer_init (envy24ht_devc * devc, int dev, int root)
{
  aureon_mixer_init_common (devc, dev, root);

  if (devc->subvendor == SSID_AUREON_UNIVERSE)
    aureon_mixer_init_universe (devc, dev, root);

  if (devc->subvendor != SSID_PHASE28)
    aureon_mixer_init_ac97 (devc, dev, root);
  return 0;
}

envy24ht_auxdrv_t envy24ht_aureon_auxdrv = {
  aureon_card_init,
  aureon_mixer_init,
  aureon_set_rate,
  NULL,
  NULL,
  NULL,				/* aureon_private1 */
};
