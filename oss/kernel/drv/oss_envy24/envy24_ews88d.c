/*
 * Purpose: Card specific routines for Terratec EWS88D.
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

/* PCF8575 pin assignment  (default 0xffff) */
#define PCF_SPDIN_SELECT	0x0001	/* 0=optical 1=coax */
#define PCF_OPTOUT		0x0002	/* 0=SPDIF 1=ADAT */
#define PCF_CLOCKSRC		0x0004	/* 0=ADAT 1=SPDIF */
#define PCF_ADATEN		0x0008	/* 0=ADAT disabled 1=ADAT enabled */
#define PCF_ADATLOOP_		0x0010	/* 0=ADAT in->out loop, 1=no loop */
#define PCF_CS8414_ERF_		0x0020	/* Inverted CS8414 ERF */
#define PCF_SPDIF_ERR		0x0040	/* OR'ed from CS8414 E0..E2 */
#define PCF_ADAT_ERROR		0x0080	/* ADAT error input */

extern int envy24_gain_sliders;
extern int envy24_virtualout;
extern int envy24_zerolatency;	/* Testing in progress */
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
static __inline__ unsigned char
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
static __inline__ void
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
static __inline__ void
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
static __inline__ void
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

static unsigned char
IIC_WriteWord (envy24_devc * devc, int dwPortAddr, unsigned char bIicAddress,
	       unsigned short bByte)
{
  IIC_Start (devc, dwPortAddr);

  /* send IIC address and data byte */
  if (!IIC_SendByte (devc, dwPortAddr, bIicAddress))
    {
      cmn_err (CE_CONT, "IIC_SendByte 1 failed\n");
      goto FAILED;
    }
  if (!IIC_SendByte (devc, dwPortAddr, bByte & 0xff))
    {
      cmn_err (CE_CONT, "IIC_SendByte 3 failed\n");
      goto FAILED;
    }
  if (!IIC_SendByte (devc, dwPortAddr, (bByte >> 8) & 0xff))
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

static void
ews88d_set_pcf8575 (envy24_devc * devc, unsigned short d)
{
  if (!IIC_WriteWord (devc, devc->ccs_base, 0x40, d))
    {
      cmn_err (CE_CONT, "IIC_WriteWord failed\n");
    }
  devc->gpio_tmp = d;
}

static void
ews88d_card_init (envy24_devc * devc)
{
  ews88d_set_pcf8575 (devc, 0xffff);
}

static void
set_ews88d_speed (envy24_devc * devc)
{
  int tmp;

  tmp = devc->speedbits;

  switch (devc->syncsource)
    {
    case SYNC_INTERNAL:
      OUTB (devc->osdev, tmp, devc->mt_base + 0x01);
      break;

    case SYNC_SPDIF:
      tmp |= 0x10;
      OUTB (devc->osdev, tmp, devc->mt_base + 0x01);
      devc->gpio_tmp &= ~PCF_CLOCKSRC;
      ews88d_set_pcf8575 (devc, devc->gpio_tmp);
      break;

    case SYNC_WCLOCK:
      tmp |= 0x10;
      OUTB (devc->osdev, tmp, devc->mt_base + 0x01);
      devc->gpio_tmp |= PCF_CLOCKSRC;
      ews88d_set_pcf8575 (devc, devc->gpio_tmp);
      break;
    }

  if (devc->speed > 48000)
    {
      devc->gpio_tmp &= ~PCF_ADATEN;
      ews88d_set_pcf8575 (devc, devc->gpio_tmp);
    }
  else
    {
      devc->gpio_tmp |= PCF_ADATEN;
      ews88d_set_pcf8575 (devc, devc->gpio_tmp);
    }
}

static int
ews88d_mixer_set (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;

  if (cmd == SNDCTL_MIX_READ)
    switch (ctrl)
      {
      case 1:
	return !!(devc->gpio_tmp & PCF_SPDIN_SELECT);
	break;

      case 2:
	return !!(devc->gpio_tmp & PCF_OPTOUT);
	break;

      case 3:
	return !(devc->gpio_tmp & PCF_ADATLOOP_);
	break;
      }

  if (cmd == SNDCTL_MIX_WRITE)
    switch (ctrl)
      {
      case 1:
	devc->gpio_tmp &= ~PCF_SPDIN_SELECT;
	if (value)
	  devc->gpio_tmp |= PCF_SPDIN_SELECT;
	ews88d_set_pcf8575 (devc, devc->gpio_tmp);
	return !!(devc->gpio_tmp & PCF_SPDIN_SELECT);
	break;

      case 2:
	devc->gpio_tmp &= ~PCF_OPTOUT;
	if (value)
	  devc->gpio_tmp |= PCF_OPTOUT;
	ews88d_set_pcf8575 (devc, devc->gpio_tmp);
	return !!(devc->gpio_tmp & PCF_OPTOUT);
	break;

      case 3:
	devc->gpio_tmp &= ~PCF_ADATLOOP_;
	if (!value)
	  devc->gpio_tmp |= PCF_ADATLOOP_;
	ews88d_set_pcf8575 (devc, devc->gpio_tmp);
	return !(devc->gpio_tmp & PCF_ADATLOOP_);
	break;
      }

  return OSS_EINVAL;
}

static int
ews88d_mix_init (envy24_devc * devc, int dev, int group)
{
  int err;

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24_EWS88D")) < 0)
    return group;

  if ((err = mixer_ext_create_control (dev, group,
				       1, ews88d_mixer_set,
				       MIXT_ENUM,
				       "EWS88D_SPDIN", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       2, ews88d_mixer_set,
				       MIXT_ENUM,
				       "EWS88D_OPTOUT", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       3, ews88d_mixer_set,
				       MIXT_ONOFF,
				       "EWS88D_ADATLOOP", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  return 0;
}

envy24_auxdrv_t ews88d_auxdrv = {
  ews88d_card_init,
  ews88d_mix_init,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  set_ews88d_speed,
  NULL,
  cs8427_spdif_mixer_init
};
