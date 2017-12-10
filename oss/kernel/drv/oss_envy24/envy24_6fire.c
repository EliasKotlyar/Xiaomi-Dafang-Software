/*
 * Purpose: Card specific routines for Terratec DMX6fire.
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

#define DEFAULT_FRONTBOX_SETUP	0xfd

extern int envy24_gain_sliders;
extern int envy24_virtualout;

#define EWX_DIG_ADDR 0x24	/* IIC address of digital traceiver chip of 6fire */

/*=============================================================================
  Function    : IIC_GetSDA
-------------------------------------------------------------------------------
  Description : 
  Returns     : unsigned char  -> 
  Parameters  : unsigned long dwPortAddr -> 
-------------------------------------------------------------------------------
  Notes       : 
=============================================================================*/
static unsigned char
IIC_GetSDA (envy24_devc * devc)
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
static void
IIC_SetIic (envy24_devc * devc, unsigned char fSDA, unsigned char fSCL)
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
IIC_Start (envy24_devc * devc)
{
  /* falling edge of SDA while SCL is HIGH */
  IIC_SetIic (devc, 1, 1);
  oss_udelay (30);
  IIC_SetIic (devc, 0, 1);
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
IIC_Stop (envy24_devc * devc)
{
  /* rising edge of SDA while SCL is HIGH */
  IIC_SetIic (devc, 0, 1);
  IIC_SetIic (devc, 1, 1);
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
IIC_SendByte (envy24_devc * devc, unsigned char bByte)
{
  unsigned char bDataBit, bAck;
  int i;

  for (i = 7; i >= 0; i--)	/* send byte (MSB first) */
    {
      bDataBit = (bByte >> i) & 0x01;

      IIC_SetIic (devc, bDataBit, 0);
      IIC_SetIic (devc, bDataBit, 1);
      IIC_SetIic (devc, bDataBit, 0);
    }				/* end for i */

  IIC_SetIic (devc, 1, 0);

  /* Get acknowledge */
  IIC_SetIic (devc, 1, 1);
  bAck = IIC_GetSDA (devc);
  /* FMB this is a start condition but never mind */
  IIC_SetIic (devc, 0, 0);
#if 0
  if (bAck)
    cmn_err (CE_CONT, "SendByte failed %02x\n", bByte);
  else
    cmn_err (CE_CONT, "SendByte OK %02x\n", bByte);
#endif
  return 1;
  /* return (!bAck); *//* bAck = 0 --> success */
}

#if 0
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
IIC_WriteByte (envy24_devc * devc, unsigned char bIicAddress,
	       unsigned char bByte)
{
  IIC_Start (devc);

  /* send IIC address and data byte */
  if (!IIC_SendByte (devc, bIicAddress))
    {
      cmn_err (CE_CONT, "IIC_SendByte 1 failed\n");
      goto FAILED;
    }
  if (!IIC_SendByte (devc, bByte))
    {
      cmn_err (CE_CONT, "IIC_SendByte 2 failed\n");
      goto FAILED;
    }

  IIC_Stop (devc);
  return 1;

FAILED:
  IIC_Stop (devc);
  return 0;
}
#endif

static unsigned char
IIC_WriteWord (envy24_devc * devc, unsigned char bIicAddress,
	       unsigned char reg, unsigned char bByte)
{
  IIC_Start (devc);

  /* send IIC address and data byte */
  if (!IIC_SendByte (devc, bIicAddress))
    {
      cmn_err (CE_CONT, "IIC_SendByte 1 failed\n");
      goto FAILED;
    }
  if (!IIC_SendByte (devc, reg))
    {
      cmn_err (CE_CONT, "IIC_SendByte 2 failed\n");
      goto FAILED;
    }
  if (!IIC_SendByte (devc, bByte))
    {
      cmn_err (CE_CONT, "IIC_SendByte 3 failed\n");
      goto FAILED;
    }

  IIC_Stop (devc);
  return 1;

FAILED:
  IIC_Stop (devc);
  return 0;
}

static void
FrontboxControl (envy24_devc * devc, int b)
{
  IIC_WriteWord (devc, 0x40, 0x01, b);	/* Output register */
  devc->gpio_tmp = b;
}

static int
HW_AK4525_SetChipSelect (envy24_devc * devc, unsigned char bCsMask)
{
  unsigned char b;
  /* check validity */
  if (bCsMask > 0x07)
    {
      cmn_err (CE_CONT, "Invalid bCsMask %02x\n", bCsMask);
      return 0;			/* invalid  */
    }

  envy24_write_cci (devc, 0x21, 0x00);
  envy24_write_cci (devc, 0x22, 0xff);
  b = envy24_read_cci (devc, 0x20);
  b &= ~0x07;

  b |= bCsMask & 0x07;

  envy24_write_cci (devc, 0x20, b);

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
  unsigned char fData;

  /* format buffer */
  wCmd = 0xA000 +		/* chip address + R/W */
    (((unsigned short) bRegIdx) << 8) +	/* register address */
    bRegData;

  HW_AK4525_SetLines (devc, 0, 0);

  /* start write cycle */
  if (!HW_AK4525_SetChipSelect (devc, bCsMask))
    {
      cmn_err (CE_CONT, "HW_AK4525_SetChipSelect failed\n");
      return 0;			/* = CS to LOW -> data latched */
    }


  for (i = 0; i < 16; i++)
    {
      fData = (wCmd & 0x8000) ? 1 : 0;
      HW_AK4525_SetLines (devc, fData, 0);
      HW_AK4525_SetLines (devc, fData, 1);	/* data is clocked in on rising edge of CCLK */
      HW_AK4525_SetLines (devc, fData, 0);
      wCmd <<= 1;
    }

  /* leave data line HIGH (= default for IIC) */
  HW_AK4525_SetLines (devc, fData, 0);	/* data is clocked in on rising edge of CCLK */

  /* end write cycle */
  if (!HW_AK4525_SetChipSelect (devc, 0x00))
    {
      cmn_err (CE_CONT, "HW_AK4525_SetChipSelect 2 failed\n");
      return 0;			/* = all CS to HIGH -> CS to default */
    }

  /* default */
  HW_AK4525_SetLines (devc, 1, 1);	/* data is clocked in on rising edge of CCLK */

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
    bCodecMask = 0x07;
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

static void
set_dmx6fire_speed (envy24_devc * devc)
{
  unsigned char tmp;

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
}

#if 0
/*
 * S/PDIF register access doesn't work so this feature is not enabled.
 */

#define EWX_CLK 0x20
#define EWX_DTA 0x10

static void
dmx6fire_iic_write_byte (envy24_devc * devc, unsigned char data,
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
  envy24_write_cci (devc, 0x20, EWX_DTA | EWX_CLK | save);	/*  Clock pulse  (ACK) */
  envy24_write_cci (devc, 0x20, EWX_DTA | save);
}

static unsigned char
dmx6fire_iic_read_byte (envy24_devc * devc, unsigned char save)
{
  int i;
  unsigned char data = 0;
  int b;

  PRT_STATUS (0x01);
  save |= EWX_DTA;
  envy24_write_cci (devc, 0x20, save);
  oss_udelay (10);
#if 1
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

#if 0
static void
dmx6fire_iic_write (envy24_devc * devc, int addr,
		    unsigned char bRegIdx, unsigned char bRegData)
{
  unsigned char save;

  /* start write cycle */
  envy24_write_cci (devc, 0x22, 0xff);	/* GPIO direction */
  envy24_write_cci (devc, 0x21, 0x00);	/* GPIO write mask */

  save = (envy24_read_cci (devc, 0x20) & ~(EWX_CLK | EWX_DTA)) | 0x08	/* R/W bit */
    ;
  /* Send start */
  envy24_write_cci (devc, 0x20, save | EWX_CLK | EWX_DTA);
  envy24_write_cci (devc, 0x20, save | EWX_CLK);
  envy24_write_cci (devc, 0x20, save);

  dmx6fire_iic_write_byte (devc, addr, save);
  dmx6fire_iic_write_byte (devc, bRegIdx, save);
  dmx6fire_iic_write_byte (devc, bRegData, save);

  /* Send stop */
  envy24_write_cci (devc, 0x20, save);
  envy24_write_cci (devc, 0x20, save | EWX_CLK);
  envy24_write_cci (devc, 0x20, save | EWX_CLK | EWX_DTA);
}
#endif

static unsigned char
dmx6fire_iic_read (envy24_devc * devc, int addr, unsigned char bRegIdx)
{
  unsigned char save, data;

  /* dmx6fire_iic_write(devc, addr, bRegIdx, 0x55); */
  PRT_STATUS (0x80);

  envy24_write_cci (devc, 0x22, 0xff);	/* GPIO direction */
  envy24_write_cci (devc, 0x21, 0x00);	/* GPIO write mask */

  save = (envy24_read_cci (devc, 0x20) & ~(EWX_DTA | EWX_CLK)) | 0x08	/* R/W bit */
    ;
  PRT_STATUS (0x02);
  /* Send start */
  envy24_write_cci (devc, 0x20, save | EWX_CLK | EWX_DTA);
  envy24_write_cci (devc, 0x20, save | EWX_CLK);
  envy24_write_cci (devc, 0x20, save);

  dmx6fire_iic_write_byte (devc, addr | 0x01, save);
  data = dmx6fire_iic_read_byte (devc, save);

  /* Send stop */
  envy24_write_cci (devc, 0x20, save);
  envy24_write_cci (devc, 0x20, save | EWX_CLK);
  envy24_write_cci (devc, 0x20, save | EWX_CLK | EWX_DTA);
  PRT_STATUS (0x00);
  return data;
}

static int
read_dmx6fire_spdif_reg (envy24_devc * devc, int bRegister)
/*
*****************************************************************************
*  Reads a byte from a specific CS8427 register.
****************************************************************************/
{

  unsigned char ret;
  ret = dmx6fire_iic_read (devc, EWX_DIG_ADDR, bRegister);

  return ret;
}

static void
write_dmx6fire_spdif_reg (envy24_devc * devc, int bRegister, int bData)
{

  /*dmx6fire_iic_write(devc, EWX_DIG_ADDR, bRegister, bData); */
  IIC_WriteWord (devc, EWX_DIG_ADDR, bRegister, bData);
}
#else
static int
read_dmx6fire_spdif_reg (envy24_devc * devc, int bRegister)
{
  return 0;
}

static void
write_dmx6fire_spdif_reg (envy24_devc * devc, int bRegister, int bData)
{
}
#endif

static void
dmx6fire_card_init (envy24_devc * devc)
{
  envy24_write_cci (devc, 0x20, 0xff);
  envy24_write_cci (devc, 0x21, 0x00);
  envy24_write_cci (devc, 0x22, 0xff);
  envy24_write_cci (devc, 0x20, 0xff);
  HW_AK4525_SetChipSelect (devc, 0x00);
#if 1

  IIC_WriteWord (devc, 0x40, 0x02, 0x00);	/* Polarity register */
  IIC_WriteWord (devc, 0x40, 0x03, 0x00);	/* Direction register */

  FrontboxControl (devc, DEFAULT_FRONTBOX_SETUP);	/* Set default values */

  if (!HW_AK4525_Reset (devc, -1))
    cmn_err (CE_CONT, "Envy24: DMX6fire reset failed\n");;
#endif

}

static int
dmx6fire_mixer_set (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;

  if (cmd == SNDCTL_MIX_READ)
    switch (ctrl)
      {
      case 1:
	if (devc->skipdevs == 2)
	  return devc->rec_portc[2].riaa_filter;
	else
	  return devc->rec_portc[4].riaa_filter;
	break;

      }

  if (cmd == SNDCTL_MIX_WRITE)
    switch (ctrl)
      {
      case 1:
	if (devc->skipdevs == 2)
	  return devc->rec_portc[2].riaa_filter = !!value;
	else
	  return devc->rec_portc[4].riaa_filter = !!value;
	break;

      }

  return OSS_EINVAL;
}

static int
dmx6fire_mix_init (envy24_devc * devc, int dev, int group)
{
  int err;
#if 0
  int i, mask = devc->outportmask, skip, codec, ports;
  int range = 3;
  int typ = MIXT_ENUM;
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
      char tmp[32];

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
					       codec | ports, envy24_set_tdif,
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
					       codec | ports, envy24_set_tdif,
					       typ,
					       tmp, range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
    }
#endif

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24_6FIRE")) < 0)
    return group;

  if ((err = mixer_ext_create_control (dev, group,
				       1, dmx6fire_mixer_set,
				       MIXT_ONOFF,
				       "6FIRE_RIAA", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  return 0;
}

envy24_auxdrv_t dmx6fire_auxdrv = {
  dmx6fire_card_init,
  dmx6fire_mix_init,
  init_cs8427_spdif,
  write_cs8427_spdif,
  NULL,
  read_dmx6fire_spdif_reg,
  write_dmx6fire_spdif_reg,
  set_dmx6fire_speed,
  NULL,
  cs8427_spdif_mixer_init
};
