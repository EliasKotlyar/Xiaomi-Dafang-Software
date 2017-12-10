/*
 * Purpose: Card specific routines for several Envy24 based cards.
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

#include "oss_config.h"
#include "ac97.h"
#include "envy24.h"

#if 0
# define PRT_STATUS(v) outb(v&0xff, 0x378)
#else
# define PRT_STATUS(v)
#endif

extern int envy24_gain_sliders;
extern int envy24_virtualout;
extern int envy24_zerolatency;	/* Testing in progress */

#define EWX_DIG_ADDR 0x20	/* IIC address of digital traceiver chip of EWX2496 */

/*
 * Playback engine management
 */

static void
write_spdif (envy24_devc * devc, int d)
{
  int i, cmd;

  cmd = envy24_read_cci (devc, 0x20);
  for (i = 7; i >= 0; i--)
    {
      cmd &= ~0x04;
      envy24_write_cci (devc, 0x20, cmd);
      oss_udelay (1);
      if (d & (1 << i))
	cmd |= 0x08;
      else
	cmd &= ~0x08;
      cmd |= 0x04;
      envy24_write_cci (devc, 0x20, cmd);
      oss_udelay (1);
    }
  cmd &= ~0x04;
  envy24_write_cci (devc, 0x20, cmd);
}

static void
write_codec (envy24_devc * devc, int codec, int addr, int data)
{
  unsigned char cmd;
  int i;
  int tmp;
  cmd = envy24_read_cci (devc, 0x20);
  cmd &= ~codec;
  envy24_write_cci (devc, 0x20, cmd);
  oss_udelay (1);
  addr &= 0x07;
  tmp = 0xa000 | (addr << 8) | data;
  for (i = 15; i >= 0; i--)
    {
      cmd &= ~0x30;
      if (tmp & (1 << i))
	cmd |= 0x10;
      envy24_write_cci (devc, 0x20, cmd);
      oss_udelay (1);
      cmd |= 0x20;
      envy24_write_cci (devc, 0x20, cmd);
      oss_udelay (1);
    }
  cmd |= codec;
  envy24_write_cci (devc, 0x20, cmd);
  oss_udelay (1);
}

#define CDC_CLK 1		/* Clock input to the CODEC's, rising edge clocks data. */
#define CDC_DIN 2		/* Data input to Envy from the CODEC. */
#define CDC_DOUT 3		/* Data output from Envy to the CODEC. */
#define DIG_CS 4		/* Chip select (0=select) for the SPDIF tx/rx. */
#define CDC_CS 5		/* Chip select (0=select) for the CODEC. */
#define D410_MUTE	7	/* Delta 410 codec mute */
#define CS_ASSERT 0		/* Asserted chip select (selects are inverted). */
#define CS_RELEASE 1		/* Idle chip select (selects are inverted). */


#define CS8_CLK CDC_CLK
#define CS8_DIN CDC_DIN
#define CS8_DOUT CDC_DOUT
#define CS8_CS DIG_CS
#define CS_1 CS_ASSERT
#define CS_0 CS_RELEASE
#define CS8_ADDR 0x20		/* Chip SPI/I2C address */
#define CS8_RD 0x01
#define CS8_WR 0x00

#define EWX_DIG_CS				0
#define EWX_CDC_CLK				5
#define EWX_CDC_DOUT			4
#define EWX_CDC_DIN 			EWX_CDC_DOUT
#define EWX_IIC_WRITE			3
#define CX_ASSERT 0		/* Asserted chip select (selects are inverted). */
#define CX_RELEASE 1		/* Idle chip select (selects are inverted). */

void
WriteGPIObit (envy24_devc * devc, int sel, int what)
{
  unsigned char gpio;

  gpio = envy24_read_cci (devc, 0x20);
  gpio &= ~(1 << sel);
  gpio |= (what << sel);
  envy24_write_cci (devc, 0x20, gpio);
}

int
ReadGPIObit (envy24_devc * devc, int sel)
{
  unsigned char gpio;

  gpio = envy24_read_cci (devc, 0x20);
  return !!(gpio & (1 << sel));
}

static void
write_ap_codec (envy24_devc * devc, int bRegister, unsigned char bData)
/*

*****************************************************************************
*  Writes a byte to a specific register of the Delta-AP CODEC.
*  Register must be (0..15).
****************************************************************************/
{
  unsigned char bMask;

  if (devc->model_data->flags & MF_D410)
    bRegister = (bRegister & 0x0F) | 0x20;	/* Add I2C address field. */
  else
    bRegister = (bRegister & 0x0F) | 0xA0;	/* Add I2C address field. */

  /* Assert the CODEC chip select and wait at least 150 nS. */
  /* */
  WriteGPIObit (devc, CDC_CS, CS_ASSERT);
  WriteGPIObit (devc, DIG_CS, CS_RELEASE);

  /* Write the register address byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, CDC_CLK, 0);

      /* Write current data bit. */
      if (bMask & bRegister)
	WriteGPIObit (devc, CDC_DOUT, 1);
      else
	WriteGPIObit (devc, CDC_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, CDC_CLK, 1);
    }


  /* Write the data byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, CDC_CLK, 0);

      /* Write current data bit. */
      if (bMask & bData)
	WriteGPIObit (devc, CDC_DOUT, 1);
      else
	WriteGPIObit (devc, CDC_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, CDC_CLK, 1);
    }

  /* De-assert chip select. */
  /* */
  WriteGPIObit (devc, CDC_CS, CS_RELEASE);
}
static int
read_ap_spdif_reg (envy24_devc * devc, int bRegister)
/*
*****************************************************************************
*  Reads a byte from a specific CS8427 register.
****************************************************************************/
{
  unsigned char bMask;
  unsigned char bRet = 0;
  unsigned char bSPI;

	/****** WRITE MAP ADDRESS FIRST ******/

  /* Drop the chip select low. */
  /* Wait at least 150 nS. */
  /* */
  WriteGPIObit (devc, DIG_CS, CS_ASSERT);

  /* Write the SPI address/cmd byte. */
  /* */
  bSPI = CS8_ADDR + CS8_WR;	/* SPI address field plus WRITE operation. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop clock (GPIO5) low. */
      WriteGPIObit (devc, CDC_CLK, 0);

      /* Write current data bit. */
      if (bMask & bSPI)
	WriteGPIObit (devc, CDC_DOUT, 1);
      else
	WriteGPIObit (devc, CDC_DOUT, 0);

      /* Raise clock (GPIO5). */
      WriteGPIObit (devc, CDC_CLK, 1);
    }


  /* Write the address (MAP) byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop clock (GPIO5) low. */
      WriteGPIObit (devc, CDC_CLK, 0);

      /* Write current data bit. */
      if (bMask & bRegister)
	WriteGPIObit (devc, CDC_DOUT, 1);
      else
	WriteGPIObit (devc, CDC_DOUT, 0);

      /* Raise clock (GPIO5). */
      WriteGPIObit (devc, CDC_CLK, 1);
    }

  /* De-assert chip select(s). */
  /* */
  WriteGPIObit (devc, DIG_CS, CS_RELEASE);


	/****** NOW READ THE DATA ******/

  /* Drop the chip select low. */
  /* Wait at least 150 nS. */
  /* */
  WriteGPIObit (devc, DIG_CS, CS_ASSERT);


  /* Write the SPI address/cmd byte. */
  /* */
  bSPI = CS8_ADDR + CS8_RD;	/* SPI address field plus READ operation. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop clock (GPIO5) low. */
      WriteGPIObit (devc, CDC_CLK, 0);

      /* Write current data bit. */
      if (bMask & bSPI)
	WriteGPIObit (devc, CDC_DOUT, 1);
      else
	WriteGPIObit (devc, CDC_DOUT, 0);

      /* Raise clock (GPIO5). */
      WriteGPIObit (devc, CDC_CLK, 1);
    }


  /* Read the data byte. */
  /* */
  bRet = 0;
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop clock low. */
      WriteGPIObit (devc, CDC_CLK, 0);

      /* Read current data bit. */
      if (ReadGPIObit (devc, CDC_DIN))
	bRet |= bMask;

      /* Raise clock. */
      WriteGPIObit (devc, CDC_CLK, 1);
    }


  /* De-assert chip selects. */
  /* */
  WriteGPIObit (devc, DIG_CS, CS_RELEASE);

  /* Return value. */

  return bRet;
}

static unsigned char
ewx2496_iic_read (envy24_devc * devc, int addr, unsigned char bRegIdx);

static int
read_ewx2496_spdif_reg (envy24_devc * devc, int bRegister)
/*
*****************************************************************************
*  Reads a byte from a specific CS8427 register.
****************************************************************************/
{
  unsigned char ret;

  ret = ewx2496_iic_read (devc, EWX_DIG_ADDR, bRegister);

  return ret;
}

#define read_cs8427_spdif_reg(d, r) devc->model_data->auxdrv->spdif_read_reg(d, r)

static void
write_ap_spdif_reg (envy24_devc * devc, int bRegister, int bData)
/*
*****************************************************************************
*  Writes a byte to a specific register of the Delta-AP S/PDIF chip.
*  Register must be (0..55).
****************************************************************************/
{
  unsigned char bMask;
  unsigned char bSPI;

  /* Assert the CODEC chip select and wait at least 150 nS. */
  /* */
  WriteGPIObit (devc, CDC_CS, CS_0);
  WriteGPIObit (devc, CS8_CS, CS_1);

  /* Write the SPI address/cmd byte. */
  /* */
  bSPI = CS8_ADDR | CS8_WR;
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, CS8_CLK, 0);

      /* Write current data bit. */
      if (bMask & bSPI)
	WriteGPIObit (devc, CS8_DOUT, 1);
      else
	WriteGPIObit (devc, CS8_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, CS8_CLK, 1);
    }

  /* Write the address (MAP) byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, CS8_CLK, 0);

      /* Write current data bit. */
      if (bMask & bRegister)
	WriteGPIObit (devc, CS8_DOUT, 1);
      else
	WriteGPIObit (devc, CS8_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, CS8_CLK, 1);
    }


  /* Write the data byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, CS8_CLK, 0);

      /* Write current data bit. */
      if (bMask & bData)
	WriteGPIObit (devc, CS8_DOUT, 1);
      else
	WriteGPIObit (devc, CS8_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, CS8_CLK, 1);
    }

  /* De-assert chip select. */
  /* */
  WriteGPIObit (devc, CS8_CS, CS_0);
}

static void
ewx2496_iic_write (envy24_devc * devc, int addr,
		   unsigned char bRegIdx, unsigned char bRegData);

static void
write_ewx2496_spdif_reg (envy24_devc * devc, int bRegister, int bData)
{
  ewx2496_iic_write (devc, EWX_DIG_ADDR, bRegister, bData);

}

#define write_cs8427_spdif_reg(d, r, b) devc->model_data->auxdrv->spdif_write_reg(d, r, b)

#define BIT0		0x01
#define BIT1		0x02
#define BIT2		0x04
#define BIT3		0x08
#define BIT4		0x10
#define BIT5		0x20
#define BIT6		0x40
#define BIT7		0x80

static void
lock_cs8427_spdif (envy24_devc * devc)
{
  write_cs8427_spdif_reg (devc, 18, read_cs8427_spdif_reg (devc, 18) & ~BIT5);
  write_cs8427_spdif_reg (devc, 18, read_cs8427_spdif_reg (devc, 18) | BIT2);
}

static void
unlock_cs8427_spdif (envy24_devc * devc)
{
  write_cs8427_spdif_reg (devc, 18, read_cs8427_spdif_reg (devc, 18) & ~BIT2);
}

static unsigned char
bitswap (unsigned char bIn)
/*
*****************************************************************************
*  Endian reversing routine.
****************************************************************************/
{
  unsigned char bOut = 0;
  unsigned char bImask = 0x01;
  unsigned char bOmask = 0x80;

  while (bImask)
    {
      if (bIn & bImask)
	bOut |= bOmask;
      bImask = bImask << 1;
      bOmask = (bOmask >> 1) & 0x7F;
    }

  return bOut;
}


static unsigned char
ReadCsByte (envy24_devc * devc, unsigned char bByteNum)
/*
*****************************************************************************
*  Reads a byte from Channel Status block buffer in CS8427.
*
*  bByteNum is in the range (0..23)
*
*  This routine assumes that CS8427 register 18 bit 5 is cleared so that the
*  CS buffer is windowed, and that register 18 bit 2 is set so that CS output
*  transfers are currently disabled.
****************************************************************************/
{
  unsigned char bTemp;

  /* CS block window starts at reg #32... */
  bTemp = read_cs8427_spdif_reg (devc, bByteNum + 32);

  /* CS block access is reverse endian. */
  return bitswap (bTemp);
}

static void
WriteCsByte (envy24_devc * devc, unsigned char bByteNum, unsigned char bData)
/*
*****************************************************************************
*  Writes a byte to Channel Status block buffer in CS8427.
*
*  bByteNum is in the range (0..23)
*
*  This routine assumes that CS8427 register 18 bit 5 is cleared so that the
*  CS buffer is windowed, and that register 18 bit 2 is set so that CS output
*  transfers are currently disabled.
****************************************************************************/
{
  /* CS block access is reverse endian. */
  bData = bitswap (bData);

  /* CS Window starts at reg #32... */
  write_cs8427_spdif_reg (devc, bByteNum + 32, bData);
}

void
InitConsumerModeCS (envy24_devc * devc)
{
  int i;

  /* Set CS8427 registers 32-55 to window CS block, and disable CS output. */
  lock_cs8427_spdif (devc);

  /* Zero all the general CS bits. */
  /* */
  for (i = 0; i < 24; i++)
    WriteCsByte (devc, i, 0x00);
  /* */
  /* Consumer (usually SPDIF or AC3) mode static bit settings. */
  /* */
  WriteCsByte (devc, 0, 0x00);	/* Consumer format (bit0 = 0). */
  WriteCsByte (devc, 1, 0x02);	/* Category = PCM encoder/decoder. */

  unlock_cs8427_spdif (devc);
}

void
init_cs8427_spdif (envy24_devc * devc)
{
  int tmp;

  /* Select iunternal sync */
  write_cs8427_spdif_reg (devc, 4, read_cs8427_spdif_reg (devc, 4) & (~BIT0));
/*
*****************************************************************************
*  Initializes core (mainly static) registers of the CS8427.
*  Returns 1 if initialization OK, otherwise 0.
****************************************************************************/
  /* Assumes Envy24 GPIO's have been initialized.  They should be just fine */
  /* in the Windows driver as they are initialized from EEPROM info. */

  /* Verify device ID register.  Must be 0x71. */
  if ((tmp = read_cs8427_spdif_reg (devc, 127)) != 0x71 && tmp != 0)
    {
      cmn_err (CE_CONT, "Envy24: Unrecognized S/PDIF chip ID %02x\n",
	       read_cs8427_spdif_reg (devc, 127));
      cmn_err (CE_CONT,
	       "        Hardware stalled. Please reboot and try again.\n");
      return;
    }

  /* Turn off RUN bit while making changes to configuration. */
  write_cs8427_spdif_reg (devc, 4, read_cs8427_spdif_reg (devc, 4) & (~BIT6));

  /* RMCK default function, set Validity, disable mutes, TCBL=output. */
  write_cs8427_spdif_reg (devc, 1, 0x01);	/* validity* is BIT6. */

  /* Hold last valid audio sample, RMCK=256*Fs, normal stereo operation. */
  write_cs8427_spdif_reg (devc, 2, 0x00);
  /* Output drivers normal operation, Tx <== serial audio port, */
  /* Rx ==> serial audio port. */
  write_cs8427_spdif_reg (devc, 3, 0x0C);

  /* RUN off, OMCK=256xFs, output time base = OMCK, input time base = */
  /* recovered input clock, recovered input clock source is Envy24. */
  write_cs8427_spdif_reg (devc, 4, 0x00);

  /* Serial audio input port data format = I2S. */
  write_cs8427_spdif_reg (devc, 5, BIT2 | BIT0);	/* SIDEL=1, SILRPOL=1. */

  /* Serial audio output port data format = I2S. */
  write_cs8427_spdif_reg (devc, 6, BIT2 | BIT0);	/* SODEL=1, SOLRPOL=1. */

  /* Turn off CS8427 interrupt stuff that we don't implement in our hardware. */
  write_cs8427_spdif_reg (devc, 9, 0x00);
  write_cs8427_spdif_reg (devc, 10, 0x00);
  write_cs8427_spdif_reg (devc, 11, 0x00);
  write_cs8427_spdif_reg (devc, 12, 0x00);
  write_cs8427_spdif_reg (devc, 13, 0x00);
  write_cs8427_spdif_reg (devc, 14, 0x00);

  /* Unmask the input PLL lock, V, confidence, biphase, parity status bits. */
  write_cs8427_spdif_reg (devc, 17,
			  (unsigned char) BIT4 | BIT3 | BIT2 | BIT1 | BIT0);

  /* Registers 32-55 window to CS buffer. */
  /* Inhibit D->E transfers from overwriting first 5 bytes of CS data. */
  /* Inhibit D->E transfers (all) of CS data. */
  /* Allow E->F transfers of CS data. */
  /* One-byte mode: both A/B channels get same written CS data. */
  /* A channel info is output to chip's EMPH* pin. */
  /* */
  write_cs8427_spdif_reg (devc, 18, 0x18);

  /* Use internal buffer to transmit User (U) data.    */
  /* Chip's U pin is an output. */
  /* Transmit all 0's for user data. */
  /* */
  write_cs8427_spdif_reg (devc, 19, 0x10);

  /* Turn on chip's RUN bit, rock and roll! */
  /* */
  write_cs8427_spdif_reg (devc, 4, read_cs8427_spdif_reg (devc, 4) | BIT6);
  InitConsumerModeCS (devc);

}

#if 0
static void
WriteCsField (envy24_devc * devc, unsigned char bByteNum,
	      unsigned short bMask, unsigned short bBits)
/*
*****************************************************************************
*  Writes a specific field within the Channel Status block buffer.
*		bByteNum is CS byte number to access (0..23).
*		bMask is the field to be written (set bits define field).
*		bBits is the value to write into that field.
*
*  Assumes: Reg 18 Bit 5 is cleared so CS buffer is accessible.
*			Reg 18 Bit 2 is set so E->F buffer transfer is stopped.
****************************************************************************/
{
  /* Get current reg value. */
  unsigned char bTemp = ReadCsByte (devc, bByteNum);

  /* Clear field to be written. */
  bTemp &= ~(bMask);

  /* Set new values. */
  WriteCsByte (devc, bByteNum, (unsigned char) (bTemp | (bBits & bMask)));
}
#endif

/*ARGSUSED*/ 
void
write_cs8427_spdif (envy24_devc * devc, int d)
{
  int i;

  if (devc->syncsource == SYNC_INTERNAL)
    {
      write_cs8427_spdif_reg (devc, 4,
			      read_cs8427_spdif_reg (devc, 4) & ~BIT0);
    }
  else
    {
      write_cs8427_spdif_reg (devc, 4,
			      read_cs8427_spdif_reg (devc, 4) | BIT0);
    }

  lock_cs8427_spdif (devc);
  for (i = 0; i < 24; i++)
    {
      WriteCsByte (devc, i, devc->spdif_cbits[i]);
    }
  unlock_cs8427_spdif (devc);
}

/*
 * Terratec stuff
 */

/*=============================================================================
  Function    : IIC_GetSDA
-------------------------------------------------------------------------------
  Description : 
  Returns     : unsigned char  -> 
  Parameters  : unsigned long dwPortAddr -> 
-------------------------------------------------------------------------------
  Notes       : 
=============================================================================*/
/*ARGSUSED*/ 
static unsigned char
IIC_GetSDA (envy24_devc * devc, unsigned long dwPortAddr)
{

  unsigned char bReg, bSDA;

  /* FMB TEST: RW line */
  /* set write mask */
  bReg = ~(0x08);		/* writeable */
  envy24_write_cci (devc, 0x21, bReg);

  /* set RW line LOW */
  bReg = 0;			/* writeable */
  envy24_write_cci (devc, 0x20, bReg);

  /* Set direction: SDA to input and SCL to output */
  bReg = envy24_read_cci (devc, 0x22);
  bReg |= 0x20;			/* SCL output = 1 */
  bReg &= ~0x10;		/* SDA input  = 0 */
  envy24_write_cci (devc, 0x22, bReg);

  /* Get SDA line state */
  bSDA = envy24_read_cci (devc, 0x20);


  /* set RW line HIGH */
  bReg = 0x08;			/* writeable */
  envy24_write_cci (devc, 0x20, bReg);

  return 1;
  /* return ((bSDA & 0x10) == 0x10); */

}

/*=============================================================================
  Function    : IIC_SetIic
-------------------------------------------------------------------------------
  Description : 
  Returns     : void  -> 
  Parameters  : unsigned int dwPortAddr -> 
              : unsigned char fSDA -> 
              : unsigned char fSCL -> 
-------------------------------------------------------------------------------
  Notes       : 
=============================================================================*/
/*ARGSUSED*/ 
static void
IIC_SetIic (envy24_devc * devc, unsigned int dwPortAddr, unsigned char fSDA,
	    unsigned char fSCL)
{
  unsigned char bReg;

  /* Set direction: SDA and SCL to output */
  bReg = envy24_read_cci (devc, 0x22);
  bReg |= (0x20 | 0x10);	/* 1 -> output */
  envy24_write_cci (devc, 0x22, bReg);

  /* set write mask */
  bReg = ~(0x20 | 0x10);	/* writeable */
  envy24_write_cci (devc, 0x21, bReg);

  /* Set line state */
  /* FMB TEST: RW line */
  bReg = 0x08;
/*	bReg = 0; */
  if (fSDA)
    bReg += 0x10;
  if (fSCL)
    bReg += 0x20;
  envy24_write_cci (devc, 0x20, bReg);

}

/*=============================================================================
  Function    : IIC_Start
-------------------------------------------------------------------------------
  Description : 
  Returns     : void  -> 
  Parameters  : unsigned int dwPortAddr -> 
-------------------------------------------------------------------------------
  Notes       : 
=============================================================================*/
/*__inline  */
static void
IIC_Start (envy24_devc * devc, unsigned int dwPortAddr)
{
  /* falling edge of SDA while SCL is HIGH */
  IIC_SetIic (devc, dwPortAddr, 1, 1);
  IIC_SetIic (devc, dwPortAddr, 0, 1);
}

/*=============================================================================
  Function    : IIC_Stop  
-------------------------------------------------------------------------------
  Description : 
  Returns     : void -> 
  Parameters  : unsigned int dwPortAddr -> 
-------------------------------------------------------------------------------
  Notes       : 
=============================================================================*/
/*__inline  */

static void
IIC_Stop (envy24_devc * devc, unsigned int dwPortAddr)
{
  /* rising edge of SDA while SCL is HIGH */
  IIC_SetIic (devc, dwPortAddr, 0, 1);
  IIC_SetIic (devc, dwPortAddr, 1, 1);
}


/*=============================================================================
  Function    : IIC_SendByte
-------------------------------------------------------------------------------
  Description : 
  Returns     : unsigned char -> 
  Parameters  : unsigned int dwPortAddr -> 
              : unsigned char bByte -> 
-------------------------------------------------------------------------------
  Notes       : 
=============================================================================*/
/*__inline  */
static unsigned char
IIC_SendByte (envy24_devc * devc, unsigned int dwPortAddr,
	      unsigned char bByte)
{
  unsigned char bDataBit, bAck;
  int i;

  for (i = 7; i >= 0; i--)	/* send byte (MSB first) */
    {
      bDataBit = (bByte >> i) & 0x01;

      IIC_SetIic (devc, dwPortAddr, bDataBit, 0);
      IIC_SetIic (devc, dwPortAddr, bDataBit, 1);
    }				/* end for i */

  IIC_SetIic (devc, dwPortAddr, 1, 0);

  /* Get acknowledge */
  IIC_SetIic (devc, dwPortAddr, 1, 1);
  bAck = IIC_GetSDA (devc, dwPortAddr);
  /* FMB this is a start condition but never mind */
  IIC_SetIic (devc, dwPortAddr, 0, 0);
  return 1;
  /* return (!bAck); *//* bAck = 0 --> success */
}

/*=============================================================================
  Function    : IIC_WriteByte
-------------------------------------------------------------------------------
  Description : 
  Returns     : unsigned char  -> 
  Parameters  : unsigned int dwPortAddr -> 
              : unsigned char bIicAddress -> 
              : unsigned char bByte -> 
-------------------------------------------------------------------------------
  Notes       : 
=============================================================================*/
static unsigned char
IIC_WriteByte (envy24_devc * devc, int dwPortAddr, unsigned char bIicAddress,
	       unsigned char bByte)
{
  IIC_Start (devc, dwPortAddr);

  /* send IIC address and data byte */
  if (!IIC_SendByte (devc, dwPortAddr, bIicAddress))
    {
      cmn_err (CE_CONT, "IIC_SendByte 1 failed\n");
      goto FAILED;
    }
  if (!IIC_SendByte (devc, dwPortAddr, bByte))
    {
      cmn_err (CE_CONT, "IIC_SendByte 2 failed\n");
      goto FAILED;
    }

  IIC_Stop (devc, dwPortAddr);
  return 1;

FAILED:
  IIC_Stop (devc, dwPortAddr);
  return 0;
}

/*=============================================================================
    FUNCTION:  HW_AK4525_SetChipSelect
-------------------------------------------------------------------------------
	PURPOSE:    
    PARAMETERS: PDEVICE_EXTENSION pDevExt - contains the port address 
	                                        and the OUT_SEL state
                unsigned char bCsMask	- AK4525 CS Mask: 1 means selected
	RETURNS:    int 
-------------------------------------------------------------------------------
	NOTES:       - We use the fact that the parameter bCsMask has the same layout
	               as the IIC command byte we have to send to the PCF8574
	               (wrong polarity, however)
				-  We also have to care that we don't change the output switch
				   setting. We remember the value in the Device Extension
				-  It is important that the CODECS are deselected (CS to HIGH)
				   after sue. Otherwise some IIC functions might accidentally
				   change our CODEC registers!!!
=============================================================================*/
static unsigned char
HW_AK4525_SetChipSelect (envy24_devc * devc, unsigned char bCsMask)
{
  unsigned char bDataByte;
  /* check validity */
  if (bCsMask > 0x0F)
    {
      cmn_err (CE_CONT, "Invalid bCsMask %02x\n", bCsMask);
      return 0;			/* invalid  */
    }

  /* Format data byte */
  bDataByte = (~bCsMask) & 0x0F;
  /* FMB PCF8574 HIGH means -10dbV,  */
  /*     OutputLevelSwitch is 1 for +4dbU */
  /* if (!pDevExt->Ews88mt.OutputLevelSwitch) */
  /*  bDataByte += 0x40; */

  /* Set Chip select to LOW for selected Codecs (via IIC) */
  if (!IIC_WriteByte (devc, devc->ccs_base, 0x48, bDataByte))
    {
      cmn_err (CE_CONT, "IIC_WriteByte failed\n");
      return 0;
    }

  return 1;
}

#define HW_AK4525_SetLines IIC_SetIic

/*=============================================================================
    FUNCTION:   HW_AK4525_WriteRegister 
-------------------------------------------------------------------------------
	PURPOSE:    
    PARAMETERS: unsigned int port      - port address for IIC port
                unsigned char bCsMask   - CoDec bitmask (bits 0..3)
		                  or 0xFF for all codecs together
		unsigned char bRegIdx   - offset to desired register
                unsigned char bReg      - new register value
	RETURNS:    
-------------------------------------------------------------------------------
	NOTES:      The Chip Selects must have been set prior to this call
				PCF8574_CODEC_IIC_ADDRESS    

	+-----------------------------------------------------------------+
	| B15 B14 B13 B12 B11 B10 B09 B08|B07 B06 B05 B04 B03 B02 B01 B00 |
	+--------+---+-------------------+--------------------------------+
	| C1  C0 |R/W|A4  A3  A2  A1  A0 |D7  D6  D5  D4  D3  D2  D1  D0  |
	| 1   0  |1  |        <= 7       |           data                 |
	+--------+---+-------------------+--------------------------------+

=============================================================================*/
static unsigned char
HW_AK4525_WriteRegister (envy24_devc * devc, unsigned char bCsMask,
			 unsigned char bRegIdx, unsigned char bRegData)
{

  unsigned short wCmd;
  unsigned char i;
  unsigned int port = devc->ccs_base;
  unsigned char fData;

  /* format buffer */
  wCmd = 0xA000 +		/* chip address + R/W */
    (((unsigned short) bRegIdx) << 8) +	/* register address */
    bRegData;

  /* start write cycle */
  if (!HW_AK4525_SetChipSelect (devc, bCsMask))
    {
      cmn_err (CE_CONT, "HW_AK4525_SetChipSelect failed\n");
      return 0;			/* = CS to LOW -> data latched */
    }


  for (i = 0; i < 16; i++)
    {
      fData = (wCmd & 0x8000) ? 1 : 0;
      HW_AK4525_SetLines (devc, port, fData, 0);
      HW_AK4525_SetLines (devc, port, fData, 1);	/* data is clocked in on rising edge of CCLK */
      wCmd <<= 1;
    }

  /* leave data line HIGH (= default for IIC) */
  HW_AK4525_SetLines (devc, port, fData, 0);	/* data is clocked in on rising edge of CCLK */

  /* end write cycle */
  if (!HW_AK4525_SetChipSelect (devc, 0x00))
    {
      cmn_err (CE_CONT, "HW_AK4525_SetChipSelect 2 failed\n");
      return 0;			/* = all CS to HIGH -> CS to default */
    }

  /* default */
  HW_AK4525_SetLines (devc, port, 1, 1);	/* data is clocked in on rising edge of CCLK */

  return 1;
}

#define EWX_CLK 0x20
#define EWX_DTA 0x10

static void
ewx2496_iic_write_byte (envy24_devc * devc, unsigned char data,
			unsigned char save)
{
  int i;
  unsigned char gpio;

  for (i = 0; i < 8; i++)
    {
      int fData = (data & 0x80) ? 1 : 0;

      gpio = 0x08;
      if (fData)
	gpio |= EWX_DTA;	/* DATA */
      envy24_write_cci (devc, 0x20, gpio | save);
      envy24_write_cci (devc, 0x20, gpio | EWX_CLK | save);	/*  Clock pulse */
      envy24_write_cci (devc, 0x20, gpio | save);

      data <<= 1;
    }

  envy24_write_cci (devc, 0x20, EWX_DTA | save);
  oss_udelay (3);
  envy24_write_cci (devc, 0x20, EWX_DTA | EWX_CLK | save);	/*  Clock pulse  (ACK) */
  oss_udelay (1);
  envy24_write_cci (devc, 0x20, EWX_DTA | save);
  oss_udelay (1);
}

static unsigned char
ewx2496_iic_read_byte (envy24_devc * devc, unsigned char save)
{
  int i;
  unsigned char data = 0;
  int b;

  PRT_STATUS (0x01);
  save |= EWX_DTA;
  envy24_write_cci (devc, 0x20, save);
  oss_udelay (10);
#if 0
  save &= ~8;			/* R/W bit */
  envy24_write_cci (devc, 0x22, ~EWX_DTA);	/* GPIO direction */
  envy24_write_cci (devc, 0x21, EWX_DTA);	/* GPIO write mask */
#endif

  for (i = 0; i < 8; i++)
    {

      envy24_write_cci (devc, 0x20, EWX_CLK | save);	/*  Clock pulse */
      b = envy24_read_cci (devc, 0x20);
      if (b & EWX_DTA)
	data |= 0x80;
      envy24_write_cci (devc, 0x20, save);

      data >>= 1;
    }

  envy24_write_cci (devc, 0x22, 0xff);	/* GPIO direction */
  envy24_write_cci (devc, 0x21, 0x00);	/* GPIO write mask */
  envy24_write_cci (devc, 0x20, (save & ~EWX_DTA) | 0x08);	/* Low for the ACK bit */
  envy24_write_cci (devc, 0x20, (save & ~EWX_DTA) | 0x08 | EWX_CLK);	/* Clock for the ACK bit */
  envy24_write_cci (devc, 0x20, save | 0x08);
  oss_udelay (10);
  return data;
}

static void
ewx2496_iic_write (envy24_devc * devc, int addr,
		   unsigned char bRegIdx, unsigned char bRegData)
{
  unsigned char save;

  /* start write cycle */
  envy24_write_cci (devc, 0x22, 0xff);	/* GPIO direction */
  envy24_write_cci (devc, 0x21, 0x00);	/* GPIO write mask */

  save = (envy24_read_cci (devc, 0x20) & ~(EWX_CLK | EWX_DTA)) | 0x08	/* R/W bit */
    ;
  /* Send start */
  oss_udelay (1);
  envy24_write_cci (devc, 0x20, save | EWX_CLK | EWX_DTA);
  oss_udelay (1);
  envy24_write_cci (devc, 0x20, save | EWX_CLK);
  oss_udelay (1);
  envy24_write_cci (devc, 0x20, save);
  oss_udelay (1);

  ewx2496_iic_write_byte (devc, addr, save);
  ewx2496_iic_write_byte (devc, bRegIdx, save);
  ewx2496_iic_write_byte (devc, bRegData, save);

  /* Send stop */
  oss_udelay (1);
  envy24_write_cci (devc, 0x20, save);
  oss_udelay (1);
  envy24_write_cci (devc, 0x20, save | EWX_CLK);
  oss_udelay (1);
  envy24_write_cci (devc, 0x20, save | EWX_CLK | EWX_DTA);
}

static unsigned char
ewx2496_iic_read (envy24_devc * devc, int addr, unsigned char bRegIdx)
{
  unsigned char save, data;

  /* ewx2496_iic_write(devc, addr, bRegIdx, 0x55); */
  PRT_STATUS (0x80);

  envy24_write_cci (devc, 0x22, 0xff);	/* GPIO direction */
  envy24_write_cci (devc, 0x21, 0x00);	/* GPIO write mask */

  save = (envy24_read_cci (devc, 0x20) & ~(EWX_DTA | EWX_CLK)) | 0x08	/* R/W bit */
    ;
  PRT_STATUS (0x02);
  /* Send start */
  oss_udelay (1);
  envy24_write_cci (devc, 0x20, save | EWX_CLK | EWX_DTA);
  oss_udelay (1);
  envy24_write_cci (devc, 0x20, save | EWX_CLK);
  oss_udelay (1);
  envy24_write_cci (devc, 0x20, save);
  oss_udelay (1);

  ewx2496_iic_write_byte (devc, addr | 0x01, save);
  oss_udelay (10);
  data = ewx2496_iic_read_byte (devc, save);

  /* Send stop */
  oss_udelay (1);
  envy24_write_cci (devc, 0x20, save);
  oss_udelay (1);
  envy24_write_cci (devc, 0x20, save | EWX_CLK);
  oss_udelay (1);
  envy24_write_cci (devc, 0x20, save | EWX_CLK | EWX_DTA);
  oss_udelay (1);
  PRT_STATUS (0x00);
  return data;
}

static unsigned char
write_ewx2496_codec (envy24_devc * devc,
		     unsigned char bRegIdx, unsigned char bRegData)
{

  unsigned short wCmd;
  unsigned char i;
  unsigned char fData;
  unsigned char gpio, save;

  /* format buffer */
  wCmd = 0xA000 +		/* chip address + R/W */
    (((unsigned short) bRegIdx) << 8) +	/* register address */
    bRegData;

  /* start write cycle */
  envy24_write_cci (devc, 0x22, 0xff);	/* GPIO direction */
  envy24_write_cci (devc, 0x21, 0x00);	/* GPIO write mask */

  save = envy24_read_cci (devc, 0x20) & ~0x39;
  envy24_write_cci (devc, 0x20, save | 0x09);	/* CS inactive */
  oss_udelay (200);
  envy24_write_cci (devc, 0x20, 0x08);	/* CS active */
  oss_udelay (200);

  gpio = 0x08;

  for (i = 0; i < 16; i++)
    {
      fData = (wCmd & 0x8000) ? 1 : 0;

      gpio = 0x08;
      if (fData)
	gpio |= 0x10;		/* DATA */
      envy24_write_cci (devc, 0x20, gpio | save);
      oss_udelay (20);
      envy24_write_cci (devc, 0x20, gpio | 0x20 | save);	/*  Clock pulse */
      oss_udelay (200);
      envy24_write_cci (devc, 0x20, gpio | save);

      wCmd <<= 1;
    }

  oss_udelay (50);
  envy24_write_cci (devc, 0x20, 0x01);	/* CS inactive */
  oss_udelay (20);

  return 1;
}

/* Register offsets */
enum
{
  AK4525_REG_POWER,
  AK4525_REG_RESET,
  AK4525_REG_FORMAT,
  AK4525_REG_DEEMPHASIS,
  AK4525_REG_LEFT_INPUT,
  AK4525_REG_RIGHT_INPUT,
  AK4525_REG_LEFT_OUTPUT,
  AK4525_REG_RIGHT_OUTPUT
};


/*=============================================================================
  Function    : HW_AK4525_Reset
-------------------------------------------------------------------------------
  Description : 
  Returns     : unsigned char 
  Parameters  : PCUST_HW_INSTANCE_DATA  devc - 
                int nCodecIdx -> 0..3 index of Codec ,-1 -> all
-------------------------------------------------------------------------------
  Notes       : 
=============================================================================*/
static unsigned char
HW_AK4525_Reset (envy24_devc * devc, int nCodecIdx)
{
  unsigned char bCodecMask = 0;
  if (nCodecIdx == -1)
    bCodecMask = 0x0F;
  else
    bCodecMask = 0x01 << (nCodecIdx);

  if (!HW_AK4525_WriteRegister (devc, bCodecMask,	/* 0x0F -> set mode for all codecs */
				AK4525_REG_RESET, 0x00))	/* DACs,ADCs -> reset */
    {
      cmn_err (CE_CONT, "REG_RESET failed\n");
      return 0;
    }

  if (!HW_AK4525_WriteRegister (devc, bCodecMask,	/* 0x0F -> set mode for all codecs */
				AK4525_REG_FORMAT, 0x60))	/* IIS, 256 fsn, normal speed */
    return 0;

  if (!HW_AK4525_WriteRegister (devc, bCodecMask,	/* 0x0F -> set mode for all codecs */
				AK4525_REG_RESET, 0x03))	/* DACs,ADCs -> normal operation */
    return 0;

  if (!HW_AK4525_WriteRegister (devc, bCodecMask,	/* 0x0F -> set mode for all codecs */
				AK4525_REG_POWER, 0x07))	/* power on */
    return 0;

  if (!HW_AK4525_WriteRegister (devc, bCodecMask,	/* soft mute timeout --> short */
				AK4525_REG_DEEMPHASIS, 0x19))
    return 0;

  if (!HW_AK4525_WriteRegister (devc,
				bCodecMask, AK4525_REG_LEFT_INPUT, 0x7f))
    return 0;

  if (!HW_AK4525_WriteRegister (devc,
				bCodecMask, AK4525_REG_RIGHT_INPUT, 0x7f))
    return 0;

  if (!HW_AK4525_WriteRegister (devc,
				bCodecMask, AK4525_REG_LEFT_OUTPUT, 0x7f))
    return 0;

  if (!HW_AK4525_WriteRegister (devc,
				bCodecMask, AK4525_REG_RIGHT_OUTPUT, 0x7f))
    return 0;

  return 1;
}

unsigned char
ewx2496_AK4525_Reset (envy24_devc * devc, int nCodecIdx)
{
  unsigned char bCodecMask = 0;
  if (nCodecIdx == -1)
    bCodecMask = 0x0F;
  else
    bCodecMask = 0x01 << (nCodecIdx);
  envy24_write_cci (devc, 0x20, 0x03);	/* Default value */

  if (!write_ewx2496_codec (devc, AK4525_REG_RESET, 0x00))	/* DACs,ADCs -> reset */
    {
      cmn_err (CE_CONT, "REG_RESET failed\n");
      return 0;
    }

  if (!write_ewx2496_codec (devc, AK4525_REG_FORMAT, 0x60))	/* IIS, 256 fsn, normal speed */
    return 0;

  if (!write_ewx2496_codec (devc, AK4525_REG_RESET, 0x03))	/* DACs,ADCs -> normal operation */
    return 0;

  if (!write_ewx2496_codec (devc, AK4525_REG_POWER, 0x07))	/* power on */
    return 0;

  if (!write_ewx2496_codec (devc, AK4525_REG_DEEMPHASIS, 0x19))
    return 0;

  if (!write_ewx2496_codec (devc, AK4525_REG_LEFT_INPUT, 0x7f))
    return 0;

  if (!write_ewx2496_codec (devc, AK4525_REG_RIGHT_INPUT, 0x7f))
    return 0;

  if (!write_ewx2496_codec (devc, AK4525_REG_LEFT_OUTPUT, 0x7f))
    return 0;

  if (!write_ewx2496_codec (devc, AK4525_REG_RIGHT_OUTPUT, 0x7f))
    return 0;

  return 1;
}

static void
ews88_init_codecs (envy24_devc * devc)
{
  if (!HW_AK4525_Reset (devc, -1))
    cmn_err (CE_CONT, "Envy24: EWS88MT reset failed\n");;
}

static void
ewx2496_init_codecs (envy24_devc * devc)
{
  if (!ewx2496_AK4525_Reset (devc, -1))
    cmn_err (CE_CONT, "Envy24: EWS88MT reset failed\n");;
}

static int
envy24_set_akm (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;
  int codec, level, i;
  static unsigned char levels[] = { 0x7f, 0x8c, 0x98 };

  if (ctrl >= 0xff)
    return OSS_EINVAL;

  if (cmd == SNDCTL_MIX_READ)
    {
      if (envy24_gain_sliders)
	return devc->akm_gains[ctrl];
      return devc->akm_gains[ctrl];
    }
  else if (cmd == SNDCTL_MIX_WRITE)
    {

      if (envy24_gain_sliders)
	level = value & 0xff;
      else
	{
	  if (value > 2)
	    return OSS_EINVAL;
	  level = levels[value];
	}

      codec = ctrl & 0xf0;

      for (i = 0; i < 4; i++)
	{
	  if (!(ctrl & (1 << i)))
	    continue;

	  write_codec (devc, codec, 4 + i, level);
	}
      return devc->akm_gains[ctrl] = value;
    }
  return OSS_EINVAL;
}

static int
envy24_set_ap (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;
  int level;
  static unsigned char levels[] = { 0x60, 0x6f, 0x7f };

  if (ctrl >= 0xff)
    return OSS_EINVAL;

  if (cmd == SNDCTL_MIX_READ)
    {
      if (envy24_gain_sliders)
	return devc->akm_gains[ctrl];
      return devc->akm_gains[ctrl];
    }
  else if (cmd == SNDCTL_MIX_WRITE)
    {

      if (envy24_gain_sliders)
	level = value & 0xff;
      else
	{
	  if (value > 2)
	    return OSS_EINVAL;
	  level = levels[value];
	}

      write_ap_codec (devc, 4, level);
      write_ap_codec (devc, 5, level);
      return devc->akm_gains[ctrl] = value;
    }
  return OSS_EINVAL;
}

static int
envy24_set_d410 (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;
  int level, n;
  static unsigned char levels[] = { 0x00, 0x00, 0x0c };

  if (ctrl >= 8)
    return OSS_EINVAL;

  if (cmd == SNDCTL_MIX_READ)
    {
      if (envy24_gain_sliders)
	return devc->akm_gains[ctrl];
      return devc->akm_gains[ctrl];
    }
  else if (cmd == SNDCTL_MIX_WRITE)
    {

      if (envy24_gain_sliders)
	level = 255 - value * 2;
      else
	{
	  if (value > 2)
	    return OSS_EINVAL;
	  level = levels[value];
	}

      if (ctrl < 6)
	n = ctrl + 2;
      else
	n = ctrl + 5;
      write_ap_codec (devc, n, level);
      if (devc->skipdevs == 2)
	write_ap_codec (devc, n + 1, level);
      return devc->akm_gains[ctrl] = value;
    }
  return OSS_EINVAL;
}

static unsigned char
write_ewx2496_codec (envy24_devc * devc,
		     unsigned char bRegIdx, unsigned char bRegData);

static int
envy24_set_ewx2496 (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;
  int level;
  static unsigned char levels[] = { 0x60, 0x6f, 0x7f };

  if (ctrl >= 0xff)
    return OSS_EINVAL;

  if (cmd == SNDCTL_MIX_READ)
    {
      if (envy24_gain_sliders)
	return devc->akm_gains[ctrl];
      return devc->akm_gains[ctrl];
    }
  else if (cmd == SNDCTL_MIX_WRITE)
    {

      if (envy24_gain_sliders)
	level = value & 0xff;
      else
	{
	  if (value > 2)
	    return OSS_EINVAL;
	  level = levels[value];
	}

      write_ewx2496_codec (devc, 6, level);
      write_ewx2496_codec (devc, 7, level);
      return devc->akm_gains[ctrl] = value;
    }
  return OSS_EINVAL;
}

static unsigned char
HW_AK4525_WriteRegister (envy24_devc * devc, unsigned char bCsMask,
			 unsigned char bRegIdx, unsigned char bRegData);

static int
envy24_set_ews88 (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;
  int codec, level, i;
  static unsigned char levels[] = { 0x7f, 0x8c, 0x98 };

  if (ctrl >= 0xff)
    return OSS_EINVAL;

  if (cmd == SNDCTL_MIX_READ)
    {
      if (envy24_gain_sliders)
	return devc->akm_gains[ctrl];
      return devc->akm_gains[ctrl];
    }
  else if (cmd == SNDCTL_MIX_WRITE)
    {

      if (envy24_gain_sliders)
	level = value & 0xff;
      else
	{
	  if (value > 2)
	    return OSS_EINVAL;
	  level = levels[value];
	}

      codec = ctrl & 0xf0;

      for (i = 0; i < 4; i++)
	{
	  if (!(ctrl & (1 << i)))
	    continue;

	  if (!HW_AK4525_WriteRegister (devc, codec >> 4, 4 + i, level))
	    return OSS_EIO;
	}
      return devc->akm_gains[ctrl] = value;
    }
  return OSS_EINVAL;
}

/*ARGSUSED*/ 
static int
create_akm_mixer (int dev, envy24_devc * devc, int root)
{
  int i, mask = devc->outportmask, group, err, skip, codec, ports;
  int typ = MIXT_ENUM, range = 3;
  char tmp[64];

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24_GAIN")) < 0)
    return group;

  skip = devc->skipdevs;
  if (skip != 2)
    skip = 1;

  if (envy24_gain_sliders)
    {
      typ = MIXT_MONOSLIDER;
      range = 144;

      for (i = 0; i < 0xff; i++)
	devc->akm_gains[i] = 0x7f;
    }
  else
    for (i = 0; i < 0xff; i++)
      devc->akm_gains[i] = 0;

  for (i = 0; i < 8; i += skip)
    {

      if (!(mask & (1 << i)))
	continue;		/* Not present */

      if (devc->skipdevs == 2)
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDOUT");
	  else
	    sprintf (tmp, "ENVY24_OUT%d/%d", i + 1, i + 2);

	  codec = (i > 1) ? AKM_B : AKM_A;
	  ports = 0x0c;		/* Both output ports */
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports, envy24_set_akm,
					       typ,
					       tmp, range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
      else
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDOUTL");
	  else if (i == 9)
	    strcpy (tmp, "ENVY24_SPDOUTR");
	  else
	    sprintf (tmp, "ENVY24_OUT%d", i + 1);

	  codec = (i > 1) ? AKM_B : AKM_A;
	  ports = (i & 1) ? 0x08 : 0x04;
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports, envy24_set_akm,
					       typ,
					       tmp, range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
    }

  mask = devc->inportmask;
  for (i = 0; i < 8; i += skip)
    {

      if (!(mask & (1 << i)))
	continue;		/* Not present */

      if (devc->skipdevs == 2)
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDIN");
	  else
	    sprintf (tmp, "ENVY24_IN%d/%d", i + 1, i + 2);

	  codec = (i > 1) ? AKM_B : AKM_A;
	  ports = 0x03;		/* Both output ports */
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports, envy24_set_akm,
					       typ,
					       tmp, range,
					       MIXF_RECVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
      else
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDINL");
	  else if (i == 9)
	    strcpy (tmp, "ENVY24_SPDINR");
	  else
	    sprintf (tmp, "ENVY24_IN%d", i + 1);

	  codec = (i > 1) ? AKM_B : AKM_A;
	  ports = (i & 1) ? 0x02 : 0x01;
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports, envy24_set_akm,
					       typ,
					       tmp, range,
					       MIXF_RECVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
    }

  return 0;
}

/*ARGSUSED*/ 
static int
create_ap_mixer (int dev, envy24_devc * devc, int root)
{
  int i, mask = devc->outportmask, group, err, skip, codec, ports;
  int typ = MIXT_ENUM, range = 3;
  char tmp[64];

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24_GAIN")) < 0)
    return group;

  skip = devc->skipdevs;
  if (skip != 2)
    skip = 1;

  if (envy24_gain_sliders)
    {
      typ = MIXT_MONOSLIDER;
      range = 164;

      for (i = 0; i < 0xff; i++)
	devc->akm_gains[i] = 0x7f;
    }
  else
    for (i = 0; i < 0xff; i++)
      devc->akm_gains[i] = 0;

  for (i = 0; i < 8; i += skip)
    {

      if (!(mask & (1 << i)))
	continue;		/* Not present */

      if (devc->skipdevs == 2)
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDOUT");
	  else
	    sprintf (tmp, "ENVY24_OUT%d/%d", i + 1, i + 2);

	  codec = (i > 1) ? AKM_B : AKM_A;
	  ports = 0x0c;		/* Both output ports */
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports, envy24_set_ap,
					       typ,
					       tmp, range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
      else
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDOUTL");
	  else if (i == 9)
	    strcpy (tmp, "ENVY24_SPDOUTR");
	  else
	    sprintf (tmp, "ENVY24_OUT%d", i + 1);

	  codec = (i > 1) ? AKM_B : AKM_A;
	  ports = (i & 1) ? 0x08 : 0x04;
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports, envy24_set_ap,
					       typ,
					       tmp, range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
    }

  return 0;
}

/*ARGSUSED*/ 
static int
create_d410_mixer (int dev, envy24_devc * devc, int root)
{
  int i, mask = devc->outportmask, group, err, skip;
  int typ = MIXT_ENUM, range = 3;
  int enum_mask = 0xffffff;
  char tmp[64];

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24_GAIN")) < 0)
    return group;

  skip = devc->skipdevs;
  if (skip != 2)
    skip = 1;

  if (envy24_gain_sliders)
    {
      typ = MIXT_MONOSLIDER;
      range = 127;

      for (i = 0; i < 0xff; i++)
	devc->akm_gains[i] = 127;
    }
  else
    {
      for (i = 0; i < 0xff; i++)
	devc->akm_gains[i] = 1;
      enum_mask = 0x6;
    }

  for (i = 0; i < 8; i += skip)
    {

      if (!(mask & (1 << i)))
	continue;		/* Not present */

      if (devc->skipdevs == 2)
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDOUT");
	  else
	    sprintf (tmp, "ENVY24_OUT%d/%d", i + 1, i + 2);

	  if ((err = mixer_ext_create_control (dev, group,
					       i, envy24_set_d410,
					       typ,
					       tmp, range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
      else
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDOUTL");
	  else if (i == 9)
	    strcpy (tmp, "ENVY24_SPDOUTR");
	  else
	    sprintf (tmp, "ENVY24_OUT%d", i + 1);

	  if ((err = mixer_ext_create_control (dev, group,
					       i, envy24_set_d410,
					       typ,
					       tmp, range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
      envy24_set_enum_mask (dev, err, enum_mask);
    }

  return 0;
}

/*ARGSUSED*/ 
static int
create_ewx2496_mixer (int dev, envy24_devc * devc, int root)
{
  int i, mask = devc->outportmask, group, err, skip, codec, ports;
  int typ = MIXT_ENUM, range = 3;
  char tmp[64];

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24_GAIN")) < 0)
    return group;

  skip = devc->skipdevs;
  if (skip != 2)
    skip = 1;

  if (envy24_gain_sliders)
    {
      typ = MIXT_MONOSLIDER;
      range = 164;

      for (i = 0; i < 0xff; i++)
	devc->akm_gains[i] = 0x7f;
    }
  else
    for (i = 0; i < 0xff; i++)
      devc->akm_gains[i] = 0;

  for (i = 0; i < 8; i += skip)
    {

      if (!(mask & (1 << i)))
	continue;		/* Not present */

      if (devc->skipdevs == 2)
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDOUT");
	  else
	    sprintf (tmp, "ENVY24_OUT%d/%d", i + 1, i + 2);

	  codec = (i > 1) ? AKM_B : AKM_A;
	  ports = 0x0c;		/* Both output ports */
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports,
					       envy24_set_ewx2496, typ, tmp,
					       range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
      else
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDOUTL");
	  else if (i == 9)
	    strcpy (tmp, "ENVY24_SPDOUTR");
	  else
	    sprintf (tmp, "ENVY24_OUT%d", i + 1);

	  codec = (i > 1) ? AKM_B : AKM_A;
	  ports = (i & 1) ? 0x08 : 0x04;
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports,
					       envy24_set_ewx2496, typ, tmp,
					       range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
    }

  return 0;
}

/*ARGSUSED*/ 
static int
create_ews88_mixer (int dev, envy24_devc * devc, int root)
{
  int i, mask = devc->outportmask, group, err, skip, codec, ports;
  int typ = MIXT_ENUM, range = 3;
  char tmp[64];

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24_GAIN")) < 0)
    return group;

  skip = devc->skipdevs;
  if (skip != 2)
    skip = 1;

  if (envy24_gain_sliders)
    {
      typ = MIXT_MONOSLIDER;
      range = 164;

      for (i = 0; i < 0xff; i++)
	devc->akm_gains[i] = 0x7f;
    }
  else
    for (i = 0; i < 0xff; i++)
      devc->akm_gains[i] = 0;

  for (i = 0; i < 8; i += skip)
    {

      if (!(mask & (1 << i)))
	continue;		/* Not present */

      if (devc->skipdevs == 2)
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDOUT");
	  else
	    sprintf (tmp, "ENVY24_OUT%d/%d", i + 1, i + 2);

	  codec = 0x10 << (i / 2);
	  ports = 0x0c;		/* Both output ports */
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports,
					       envy24_set_ews88, typ, tmp,
					       range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
      else
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDOUTL");
	  else if (i == 9)
	    strcpy (tmp, "ENVY24_SPDOUTR");
	  else
	    sprintf (tmp, "ENVY24_OUT%d", i + 1);

	  codec = 0x10 << (i / 2);
	  ports = (i & 1) ? 0x08 : 0x04;
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports,
					       envy24_set_ews88, typ, tmp,
					       range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
    }

  mask = devc->inportmask;
  for (i = 0; i < 8; i += skip)
    {

      if (!(mask & (1 << i)))
	continue;		/* Not present */

      if (devc->skipdevs == 2)
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDIN");
	  else
	    sprintf (tmp, "ENVY24_IN%d/%d", i + 1, i + 2);

	  codec = 0x10 << (i / 2);
	  ports = 0x03;		/* Both output ports */
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports,
					       envy24_set_ews88, typ, tmp,
					       range,
					       MIXF_RECVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
      else
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDINL");
	  else if (i == 9)
	    strcpy (tmp, "ENVY24_SPDINR");
	  else
	    sprintf (tmp, "ENVY24_IN%d", i + 1);

	  codec = 0x10 << (i / 2);
	  ports = (i & 1) ? 0x02 : 0x01;
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports,
					       envy24_set_ews88, typ, tmp,
					       range,
					       MIXF_RECVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
    }

  return 0;
}

/*
 * Hoontech DSP24 family
 */


typedef union
{
  struct
  {
    unsigned int data:5;
    unsigned int clock:1;
    unsigned int res0:2;
  }
  b;

  unsigned int dwVal;
}
CLBYTE;

#define HOONTECH_CLOCK	0x20

static void
hoontech_write_gpio (envy24_devc * devc, unsigned char data)
{
  envy24_write_cci (devc, 0x22, 0xff);	/* GPIO direction */
  envy24_write_cci (devc, 0x21, 0xc0);	/* GPIO write mask */

  envy24_write_cci (devc, 0x20, data | HOONTECH_CLOCK);	/* GPIO data */
  envy24_write_cci (devc, 0x20, data & ~HOONTECH_CLOCK);	/* GPIO data */
  envy24_write_cci (devc, 0x20, data | HOONTECH_CLOCK);	/* GPIO data */
}

static void
hoontech_init (envy24_devc * devc)
{
  devc->adsp.b.box = 0;
  devc->adsp.b.darear = 0;
  devc->adsp.b.id0 = 0;
  devc->adsp.b.clock0 = 1;
  devc->adsp.b.res0 = 0x0;

  devc->adsp.b.ch1 = 0x01;
  devc->adsp.b.ch2 = 0x01;
  devc->adsp.b.ch3 = 0x01;
  devc->adsp.b.id1 = 1;
  devc->adsp.b.clock1 = 1;
  devc->adsp.b.res1 = 0x0;

  devc->adsp.b.ch4 = 0x01;
  devc->adsp.b.midiin = 0x01;
  devc->adsp.b.midi1 = 0x0;
  devc->adsp.b.id2 = 2;
  devc->adsp.b.clock2 = 1;
  devc->adsp.b.res2 = 0x0;

  devc->adsp.b.midi2 = 0x0;
  devc->adsp.b.mute = 0x0;
  devc->adsp.b.insel = 0x1;
  devc->adsp.b.id3 = 3;
  devc->adsp.b.clock3 = 1;
  devc->adsp.b.res3 = 0x0;

  hoontech_write_gpio (devc, devc->adsp.w.b0);
  hoontech_write_gpio (devc, devc->adsp.w.b1);
  hoontech_write_gpio (devc, devc->adsp.w.b2);
  hoontech_write_gpio (devc, devc->adsp.w.b3);
}

/**************************************************/

static void
default_card_init (envy24_devc * devc)
{

  if (devc->model_data->flags & MF_AKMCODEC)
    {
      write_codec (devc, AKM_A | AKM_B, 0, 0x07);
      write_codec (devc, AKM_A | AKM_B, 1, 0x03);
      write_codec (devc, AKM_A | AKM_B, 2, 0x60);
      write_codec (devc, AKM_A | AKM_B, 3, 0x19);
      write_codec (devc, AKM_A | AKM_B, 4, 0x7f);
      write_codec (devc, AKM_A | AKM_B, 5, 0x7f);
      write_codec (devc, AKM_A | AKM_B, 6, 0x7f);
      write_codec (devc, AKM_A | AKM_B, 7, 0x7f);
    }

  if (devc->model_data->flags & MF_AP)
    {				/* Init the AK4528 codec of Audiophile 2496 */
      write_ap_codec (devc, 0, 0x07);
      write_ap_codec (devc, 1, 0x00);
      write_ap_codec (devc, 2, 0x60);
      write_ap_codec (devc, 3, 0x0d);
      write_ap_codec (devc, 4, 0x7f);
      write_ap_codec (devc, 1, 0x03);
      write_ap_codec (devc, 5, 0x7f);
    }

  if (devc->model_data->flags & MF_D410)
    {				/* Init the AK4529 codec of Delta 410 */
      OUTB (devc->osdev,
	    INB (devc->osdev, devc->mt_base + 0x02) & ~0x04,
	    devc->mt_base + 0x02);

      write_ap_codec (devc, 0x00, 0x0c);
      write_ap_codec (devc, 0x01, 0x02);
      write_ap_codec (devc, 0x02, 0x00);
      write_ap_codec (devc, 0x03, 0x00);
      write_ap_codec (devc, 0x04, 0x00);
      write_ap_codec (devc, 0x05, 0x00);
      write_ap_codec (devc, 0x06, 0x00);
      write_ap_codec (devc, 0x07, 0x00);
      write_ap_codec (devc, 0x0b, 0x00);
      write_ap_codec (devc, 0x0c, 0x00);
      write_ap_codec (devc, 0x08, 0x15);
      write_ap_codec (devc, 0x0a, 0x3f);
    }

  if (devc->model_data->flags & MF_EWS88)
    ews88_init_codecs (devc);

  if (devc->model_data->flags & MF_EWX2496)
    ewx2496_init_codecs (devc);

  if (devc->model_data->flags & MF_HOONTECH)
    hoontech_init (devc);
}

static int
default_mix_init (envy24_devc * devc, int dev, int group)
{
  int err;

  if (devc->model_data->flags & MF_AKMCODEC)
    {
      if ((err = create_akm_mixer (dev, devc, group)) < 0)
	return err;
    }
  else if (devc->model_data->flags & MF_AP)
    {
      if ((err = create_ap_mixer (dev, devc, group)) < 0)
	return err;
    }
  else if (devc->model_data->flags & MF_D410)
    {
      if ((err = create_d410_mixer (dev, devc, group)) < 0)
	return err;
    }
  else if (devc->model_data->flags & MF_EWS88)
    {
      if ((err = create_ews88_mixer (dev, devc, group)) < 0)
	return err;
    }
  else if (devc->model_data->flags & MF_EWX2496)
    {
      if ((err = create_ewx2496_mixer (dev, devc, group)) < 0)
	return err;
    }
  return 0;
}

/*ARGSUSED*/ 
int
cs8427_spdif_ioctl (envy24_devc * devc, int dev, unsigned int cmd,
		    ioctl_arg arg)
{
  oss_digital_control *ctrl = (oss_digital_control *) arg;

  if (arg == NULL)
    return OSS_EINVAL;

  switch (cmd)
    {
    case SNDCTL_DSP_READCTL:
      {
	int i, rq;
	unsigned char status;

	rq = ctrl->valid;
	memset (ctrl, 0, sizeof (*ctrl));

	ctrl->caps = DIG_CBITIN_FULL | DIG_CBITOUT_FULL;

	if (rq & VAL_CBITIN)
	  {
	    for (i = 0; i < 24; i++)
	      ctrl->cbitin[i] = ReadCsByte (devc, i);
	    ctrl->valid |= VAL_CBITIN;
	  }

	if (rq & VAL_CBITOUT)
	  {
	    for (i = 0; i < 24; i++)
	      ctrl->cbitin[i] = devc->spdif_cbits[i];
	    ctrl->valid |= VAL_CBITOUT;
	  }


	if (rq & VAL_ISTATUS)
	  {
	    ctrl->valid |= VAL_ISTATUS;
	    status = read_cs8427_spdif_reg (devc, 15);
	    if (status & 0x04)
	      ctrl->in_data = IND_DATA;
	    else
	      ctrl->in_data = IND_AUDIO;

	    status = read_cs8427_spdif_reg (devc, 16);
	    if (status & 0x40)
	      ctrl->in_errors |= INERR_QCODE_CRC;
	    if (status & 0x20)
	      ctrl->in_errors |= INERR_CRC;

	    if (status & 0x10)
	      ctrl->in_locked = LOCK_UNLOCKED;
	    else
	      ctrl->in_locked = LOCK_LOCKED;

	    if (status & 0x08)
	      ctrl->in_vbit = VBIT_ON;
	    else
	      ctrl->in_vbit = VBIT_OFF;

	    if (status & 0x04)
	      ctrl->in_quality = IN_QUAL_POOR;
	    else
	      ctrl->in_quality = IN_QUAL_GOOD;

	    if (status & 0x02)
	      ctrl->in_errors |= INERR_BIPHASE;

	    if (status & 0x01)
	      ctrl->in_errors |= INERR_PARITY;

#if 1
	    /* TODO: Better handling required */
	    write_cs8427_spdif_reg (devc, 18, 0x00);
#endif
	  }
      }
      return 0;
      break;

    case SNDCTL_DSP_WRITECTL:
      {
	int i, rq;

	rq = ctrl->valid;
	memset (ctrl, 0, sizeof (*ctrl));

	ctrl->caps = DIG_CBITOUT_FULL;

	if (rq & VAL_CBITOUT)
	  {
	    for (i = 0; i < 24; i++)
	      devc->spdif_cbits[i] = ctrl->cbitout[i];
	    ctrl->valid |= VAL_CBITOUT;

	    lock_cs8427_spdif (devc);
	    for (i = 0; i < 24; i++)
	      {
		WriteCsByte (devc, i, devc->spdif_cbits[i]);
	      }
	    unlock_cs8427_spdif (devc);
	  }
      }
      return 0;
      break;

    default:;
    }
  return OSS_EINVAL;
}

static int
set_spdif_control (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;

  if (cmd == SNDCTL_MIX_READ)
    {
      switch (ctrl)
	{
	case 1:
	  return devc->spdif_pro_mode;
	  break;
	}

      return OSS_EIO;
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (ctrl)
	{
	case 1:
	  return devc->spdif_pro_mode = !!value;
	  break;
	}

      return OSS_EIO;
    }

  return OSS_EIO;
}

/*ARGSUSED*/ 
int
cs8427_spdif_mixer_init (envy24_devc * devc, int dev, int group)
{
  int err;

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24_SPDIF")) < 0)
    return group;

  if ((err = mixer_ext_create_control (dev, group,
				       1, set_spdif_control,
				       MIXT_ENUM,
				       "SPDIF_MODE", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  return 0;
}

static void
set_ap_speed (envy24_devc * devc)
{
  unsigned char gpio, tmp;

  tmp = devc->speedbits;
  if (devc->syncsource != SYNC_INTERNAL)
    {
      tmp |= 0x10;		/* S/PDIF input clock select */
      if (devc->model_data->flags & MF_WCLOCK)	/* Has world clock too */
	{
	  int cmd = envy24_read_cci (devc, 0x20);
	  cmd |= 0x10;		/* S/PDIF */
	  if (devc->syncsource == SYNC_WCLOCK)
	    cmd &= ~0x10;	/* World clock */
	  envy24_write_cci (devc, 0x20, cmd);
	}
    }
  OUTB (devc->osdev, tmp, devc->mt_base + 0x01);

  if (devc->speed > 48000)
    {
      write_ap_codec (devc, 0x01, 0x00);
      gpio = envy24_read_cci (devc, 0x20);
      gpio |= 0x01;		/* Turn DFS ON */
      envy24_write_cci (devc, 0x20, gpio);
      write_ap_codec (devc, 0x02, 0x65);
      write_ap_codec (devc, 0x01, 0x03);
    }
  else
    {
      write_ap_codec (devc, 0x01, 0x00);
      gpio = envy24_read_cci (devc, 0x20);
      gpio &= ~0x01;		/* Turn DFS OFF */
      envy24_write_cci (devc, 0x20, gpio);
      write_ap_codec (devc, 0x02, 0x60);
      write_ap_codec (devc, 0x01, 0x03);
    }
}

envy24_auxdrv_t default_auxdrv = {
  default_card_init,
  default_mix_init,
  NULL,
  write_spdif
};

envy24_auxdrv_t ap2496_auxdrv = {
  default_card_init,
  default_mix_init,
  init_cs8427_spdif,
  write_cs8427_spdif,
  cs8427_spdif_ioctl,
  read_ap_spdif_reg,
  write_ap_spdif_reg,
  set_ap_speed,
  NULL,
  cs8427_spdif_mixer_init
};

envy24_auxdrv_t d410_auxdrv = {
  default_card_init,
  default_mix_init,
  init_cs8427_spdif,
  write_cs8427_spdif,
  cs8427_spdif_ioctl,
  read_ap_spdif_reg,
  write_ap_spdif_reg,
  NULL,
  NULL,
  cs8427_spdif_mixer_init
};

envy24_auxdrv_t ewx2496_auxdrv = {
  default_card_init,
  default_mix_init,
  init_cs8427_spdif,
  write_cs8427_spdif,
  NULL,
  read_ewx2496_spdif_reg,
  write_ewx2496_spdif_reg,
  NULL,
  NULL,
  cs8427_spdif_mixer_init
};
