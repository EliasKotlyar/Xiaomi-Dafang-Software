/*
 * Purpose: Driver for Aureal Semiconductor Vortex 2 PCI audio controller.
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

#include "vortex.h"

#undef USE_SRC
#undef USE_SPDIF

#define AUREAL_VENDOR_ID	0x12eb
#define AUREAL_VORTEX2		0x0002

#	define			ISR	(devc->global_base+0x0)
#		define			CODIRQST	0x00008000
#		define			MODIRQST	0x00004000
#		define			MIDIRQST	0x00002000
#		define			TIMIRQST	0x00001000
#		define			COIRQLST	0x00000800
#		define			COIRQPST	0x00000400
#		define			FRCIRQST	0x00000200
#		define			SBIRQST 	0x00000100
#		define			SBMIRQST	0x00000080
#		define			FMIRQST		0x00000040
#		define			DMAENDIRQST	0x00000020
#		define			DMABERRST	0x00000010
#		define			FIFOERRST	0x00000008
#		define			REGERRST	0x00000004
#		define			MPARERRST	0x00000002
#		define			MFATERRST	0x00000001
#	define			ICR		devc->global_base+0x4
#	define			GSR		devc->global_base+0x8
#	define			GCR		devc->global_base+0xc
#		define			SFTRST		0x00800000
#		define			ARBRST		0x00400000
#		define			EXTRST		0x00200000
#		define			RBTOEN		0x00100000
#		define			AINCEN		0x00080000
#		define			TTXFLUSH	0x00040000
#		define			TRVFLUSH	0x00020000
#		define			MFIFOFLUSH	0x00010000
#		define			FRCIRQ		0x00008000
#		define			GIRQEN		0x00004000
#	define			MIDIDAT		devc->midi_base+0x0
#	define			MIDICMD		devc->midi_base+0x4
#	define			MIDISTAT	MIDICMD
#		define			MIDIVAL		0x00000080
#		define			CMDOK		0x00000040
#		define			MPUMODE		0x00000001
#	define			GAMECTL		devc->midi_base+0xc
#		define			JOYMODE		0x00000040
#		define			MIDITXFULL	0x00000008
#		define			MIDIRXINIT	0x00000004
#		define			MIDIRXOVFL	0x00000002
#		define			MIDIDATVAL	0x00000001

#define adb_destinations		173
#	define			CODSMPLTMR	(devc->serial_block_base+0x19c)
#define adbarb_base	devc->adbarb_block_base
#define adbarb_wtdram_base_addr		(devc->adbarb_block_base + 0x000)
#define adbarb_wtdram_srhdr_base_addr 	(devc->adbarb_block_base + (adb_destinations * 4))
#define adbarb_sr_active_addr		(devc->adbarb_block_base + 0x400)

#define codec_control_reg_addr		(devc->serial_block_base + 0x184)
#define cmd_status_reg_addr		(devc->serial_block_base + 0x188)
#define channel_enable_reg_addr		(devc->serial_block_base + 0x190)
#define serial_ram_reg_addr		(devc->serial_block_base + 0x00)

#define         spdif_ctrl_reg  (devc->serial_block_base + 0x0194)	/* offset for spdif control register */
#define         codec_sample_counter (devc->serial_block_base + 0x0198)	/* offset for spdif control register */
#define         spdif_cfg_dword         0x86	/* enable port, enable CRC, set clock toinput (48kHz) */
#define         spdif_ch_status_reg0    0x0	/* Set to consumer, digital audio, */
#define     spdif_ch_status_reg_base (devc->serial_block_base + 0x1D0)	/* Set to the first address location */

#define fifo_chan0_src_addr         0x00
#define fifo_chan1_src_addr         0x01
#define fifo_chan6_src_addr         0x06
#define fifo_chan7_src_addr         0x07
#define fifo_chan8_src_addr         0x08
#define fifo_chan9_src_addr         0x09
#define fifo_chan2_dst_addr         0x02
#define fifo_chan2a_dst_addr         0x022
#define fifo_chan3_dst_addr         0x03
#define fifo_chan3a_dst_addr         0x023
#define fifo_chan4_dst_addr         0x04
#define fifo_chan5_dst_addr         0x05
#define fifo_chan6_dst_addr         0x06
#define fifo_chan7_dst_addr         0x07
#define fifo_chan8_dst_addr         0x08
#define fifo_chan9_dst_addr         0x09
#define fifo_chana_dst_addr         0x0a
#define fifo_chanb_dst_addr         0x0b

#define codec_chan0_src_addr        0x70
#define codec_chan1_src_addr        0x71
#define codec_chan0_dst_addr        0x88
#define codec_chan1_dst_addr        0x89
#define codec_chan4_dst_addr        0x8c
#define codec_chan5_dst_addr        0x8d

#define spdif_chan0_dst_addr                0x92
#define spdif_chan1_dst_addr                0x93

#define src_chan0_dst_addr                  0x40
#define src_chan1_dst_addr                  0x41
#define src_chan0_src_addr                  0x20
#define src_chan1_src_addr                  0x21

#define src_base_offset				(devc->src_base)
#define src_input_fifo_base			(devc->src_base + 0x000)
#define src_output_fifo_base			(devc->src_base + 0x800)
#define src_next_ch_base			(devc->src_base + 0xc00)
#define src_sr_header_base			(devc->src_base + 0xc40)
#define src_active_sample_rate			(devc->src_base + 0xcc0)
#define src_throttle_source			(devc->src_base + 0xcc4)
#define src_throttle_count_size			(devc->src_base + 0xcc8)
#define src_ch_params_base			(devc->src_base + 0xe00)
#define src_ch_param0				(devc->src_base + 0xe00)
#define src_ch_param1				(devc->src_base + 0xe40)
#define src_ch_param2				(devc->src_base + 0xe80)
#define src_ch_param3				(devc->src_base + 0xec0)
#define src_ch_param4				(devc->src_base + 0xf00)
#define src_ch_param5				(devc->src_base + 0xf40)
#define src_ch_param6				(devc->src_base + 0xf80)

#define pif_gpio_control			(devc->parallel_base + 0x05c)

/************************************************************
 *                          Vortex2 Routines                *
 ************************************************************/


static unsigned int
V2ReadReg (vortex_devc * devc, oss_native_word addr)
{
  return READL (addr);
}

static void
V2WriteReg (vortex_devc * devc, oss_native_word addr, oss_native_word data)
{
  WRITEL (addr, data);
}

static void
V2ReadCodecRegister (vortex_devc * devc, int cIndex, int *pdwData)
{
  int dwCmdStRegAddr;
  int dwCmdStRegData;
  int nCnt = 0;

  dwCmdStRegAddr = cIndex << 16;
  V2WriteReg (devc, cmd_status_reg_addr, dwCmdStRegAddr);
  do
    {
      dwCmdStRegData = V2ReadReg (devc, cmd_status_reg_addr);
      if (nCnt++ > 1)
	oss_udelay (10);
    }
  while ((dwCmdStRegData & 0x00FF0000) != ((1 << 23) | (dwCmdStRegAddr))
	 && (nCnt < 10));
  if (nCnt >= 50)
    {
      *pdwData = -1;
      cmn_err (CE_WARN, "AC97 Timeout\n");
    }
  else
    *pdwData = dwCmdStRegData & 0x0000ffff;
}

static void
V2WriteCodecCommand (vortex_devc * devc, int cIndex, int wData)
{
  int dwData;
  int nCnt = 0;


  do
    {
      dwData = V2ReadReg (devc, codec_control_reg_addr);
      if (nCnt++ > 1)
	oss_udelay (10);
    }
  while (!(dwData & 0x0100) && (nCnt < 50));
  if (nCnt >= 10)
    cmn_err (CE_WARN, "AC97 write timeout.\n");


  do
    {
      dwData = V2ReadReg (devc, codec_control_reg_addr);
      if (nCnt++ > 1)
	oss_udelay (10);
    }
  while (!(dwData & 0x0100) && (nCnt < 100));
  if (nCnt >= 100)
    cmn_err (CE_WARN, "AC97 timeout(2)\n");

  dwData = (cIndex << 16) | (1 << 23) | wData;
  V2WriteReg (devc, cmd_status_reg_addr, dwData);
  oss_udelay (20);
  /* Read it back to make sure it got there */
  do
    {
      dwData = V2ReadReg (devc, codec_control_reg_addr);
      if (nCnt++ > 1)
	oss_udelay (10);
    }
  while (!(dwData & 0x0100) && (nCnt < 100));
  if (nCnt >= 100)
    cmn_err (CE_WARN, "AC97 timeout(3)\n");
  V2ReadCodecRegister (devc, cIndex, &dwData);
  if (dwData != (int) wData)
    {
      do
	{
	  dwData = V2ReadReg (devc, codec_control_reg_addr);
	  if (nCnt++ > 1)
	    oss_udelay (10);
	}
      while (!(dwData & 0x0100) && (nCnt < 10));
      if (nCnt >= 10)
	cmn_err (CE_WARN, "AC97 Timeout(4).\n");
      dwData = (cIndex << 16) | (1 << 23) | wData;
      V2WriteReg (devc, cmd_status_reg_addr, dwData);
      do
	{
	  dwData = V2ReadReg (devc, codec_control_reg_addr);
	  if (nCnt++ > 1)
	    oss_udelay (10);
	}
      while (!(dwData & 0x0100) && (nCnt < 100));
      if (nCnt >= 100)
	cmn_err (CE_WARN, "AC97 timeout(5).\n");
#if 0
      V2ReadCodecRegister (devc, cIndex, &dwData);
      if (dwData != (int) wData)
	{
	  cmn_err (CE_WARN,
		   "Vortex ERROR: Write to index %x failed (exp %04x, got %04x)\n",
		   cIndex, wData, dwData);
	}
#endif
    }
}

static void
ClearDataFifo (vortex_devc * devc, int nChannel)
{
  int j;

  /* Clear out FIFO data */
  for (j = 0; j < 64; j++)
    V2WriteReg (devc,
		devc->fifo_base + 0x4000 + (0x100 * nChannel) + (0x4 * j),
		0x0);
}

static void
cold_reset (vortex_devc * devc)
{
  int i, reg;
  int bSigmatelCodec = 0;

  V2ReadCodecRegister (devc, 0x7c, &reg);
  if (reg == 0x8384)
    {
      DDB (cmn_err (CE_WARN, "Sigmatel codec detected\n"));
      bSigmatelCodec = 1;
    }

  for (i = 0; i < 32; i = i + 1)
    {
      V2WriteReg (devc, serial_ram_reg_addr + 0x80 + (i * 4), 0);
      oss_udelay (10);
    }
  if (bSigmatelCodec)
    {
      /* Disable clock */
      V2WriteReg (devc, codec_control_reg_addr, 0x00a8);
      oss_udelay (100);
      /* Set Sync High     */
      /*    V2WriteReg(devc, codec_control_reg_addr, 0x40a8); */
      /*    delay(100);    */
      /* Place CODEC into reset */
      V2WriteReg (devc, codec_control_reg_addr, 0x80a8);
      oss_udelay (100);
      /* Give CODEC some Clocks with reset asserted */
      V2WriteReg (devc, codec_control_reg_addr, 0x80e8);
      oss_udelay (100);
      /* Turn off clocks */
      V2WriteReg (devc, codec_control_reg_addr, 0x80a8);
      oss_udelay (100);
      /* Take out of reset */
      /*  V2WriteReg(devc, codec_control_reg_addr, 0x40a8); */
      /*  oss_udelay(100);    */
      /* Release reset     */
      V2WriteReg (devc, codec_control_reg_addr, 0x00a8);
      oss_udelay (100);
      /* Turn on clocks        */
      V2WriteReg (devc, codec_control_reg_addr, 0x00e8);
      oss_udelay (100);
    }
  else
    {
      V2WriteReg (devc, codec_control_reg_addr, 0x8068);
      oss_udelay (10);
      V2WriteReg (devc, codec_control_reg_addr, 0x00e8);
      oss_udelay (10);
    }
}

static void
V2InitCodec (vortex_devc * devc)
{
  int i;
  cold_reset (devc);
  for (i = 0; i < 32; i = i + 1)
    {
      V2WriteReg (devc, serial_ram_reg_addr + 0x80 + (i * 4), 0);
      oss_udelay (10);
    }

  /* Set up the codec in AC97 mode */
  V2WriteReg (devc, codec_control_reg_addr, 0x00e8);
  oss_udelay (10);
  /* Clear the channel enable register */
  V2WriteReg (devc, channel_enable_reg_addr, 0);

  /* Set up Sigmatel STAC9708 Codec with initialization routine  rev. 0.50 */

  V2WriteCodecCommand (devc, 0x26, 0x800f);	/* set EAPD to unmute */
  oss_udelay (10);

  V2WriteCodecCommand (devc, 0x76, 0xabba);
  oss_udelay (10);		/* Turn on secondary output DACs */
  V2WriteCodecCommand (devc, 0x78, 0x1000);
  oss_udelay (10);

  V2WriteCodecCommand (devc, 0x70, 0xabba);
  oss_udelay (10);		/* Turn on extra current to reduce THD */
  V2WriteCodecCommand (devc, 0x72, 0x07);
  oss_udelay (10);
}

static void
V2SetupCodec (vortex_devc * devc)
{
  int dwData;
  int count = 0;
  int dwBit28 = 1 << 28;

  /* do the following only for ac97 codecs */
  /* Wait for Codec Ready (bit 28) */
  do
    {
      dwData = V2ReadReg (devc, codec_control_reg_addr);
      if (count++ > 1)
	oss_udelay (10);
    }
  while ((count < 100) && !(dwData & dwBit28));
  if (count >= 100)
    {
#if 1
      cmn_err (CE_WARN, "Error: timeout waiting for Codec Ready bit.\n");
      cmn_err (CE_WARN, "Codec Interface Control Register is %08x\n", dwData);
#endif
    }
  /* Write interesting data to the Codec 97 Mixer registers */
  /* Master Volume 0dB Attunuation, Not muted. */
  V2WriteCodecCommand (devc, 0x02, 0x0a0a);
  oss_udelay (10);
  /* Master Volume mono muted. */
  V2WriteCodecCommand (devc, 0x06, 0x8000);
  oss_udelay (10);
  /* Mic Volume muted. */
  V2WriteCodecCommand (devc, 0x0e, 0x8000);
  oss_udelay (10);
  /* Line In Volume muted. */
  V2WriteCodecCommand (devc, 0x10, 0x8000);
  oss_udelay (10);
  /* CD Volume muted. */
  V2WriteCodecCommand (devc, 0x12, 0x8000);
  oss_udelay (10);
  /* Aux Volume muted. */
  V2WriteCodecCommand (devc, 0x16, 0x8000);
  oss_udelay (10);
  /* PCM out Volume 0 dB Gain, Not muted. */
  V2WriteCodecCommand (devc, 0x18, 0x0f0f);
  oss_udelay (10);
  /* Record select, select Mic for recording */
  V2WriteCodecCommand (devc, 0x1a, 0x0404);
  oss_udelay (10);
  /* Record Gain, 0dB */
  V2WriteCodecCommand (devc, 0x1c, 0x8000);
  oss_udelay (10);

  /* Poll the Section Ready bits in the Status Register (index 0x26) */
  count = 0;
  do
    {
      V2ReadCodecRegister (devc, 0x26, &dwData);
      if (count++ > 1)
	oss_udelay (10);
    }
  while (!(dwData & 0x02) && (count < 10));
  if (!(dwData & 0x02))
    cmn_err (CE_WARN, "DAC section ready bit is not set.\n");

  /* Read and confirm the data in the Codec 97 Mixer registers. */
  /* just the PCM reg, as a sanity check */
  V2ReadCodecRegister (devc, 0x18, &dwData);
  if ((dwData & 0x0000ffff) != 0xf0f)
    {
      cmn_err (CE_WARN, "PCM volume reg is %x, sb 0xf0f.\n", dwData);
    }
}

static void
V2InitAdb (vortex_devc * devc)
{
/*  parameter  values for write_op  */
#define none	  0		/* dst_op = x */
#define tail 	  1		/* dst_op = x */
#define add 	  2		/* dst_op = dst_addr being added */
#define adds 	  3		/* dst_op = dst_addr being added */
#define del		  4	/* dst_op = dst_addr being deleted */
#define dels	  5		/* dst_op = dst_addr being deleted */
#define inval	  6		/* dst_op = x */

  int i;

#if 0
  unsigned char /*reg   [3:0] */ write_op;
  unsigned char /*reg   [6:0] */ dst_op;
  unsigned char /*reg   [6:0] */ src_op;
  unsigned char /*reg   [SS:0] */ sr_op;
#endif
  /* misc */

  devc->sr_active = 0;
  /* the initial tail for each list is the header location */
  for (i = 0; i <= 31; i = i + 1)
    devc->tail_index[i] = adb_destinations + i;
  for (i = 0; i <= 127; i = i + 1)
    devc->dst_index[i] = 0x7f;	/*~('b0); */
  for (i = 0; i <= 127; i = i + 1)
    devc->sr_list[i] = 0x1f;	/*~('b0); */

#if 0
  write_op = none;
  dst_op = 0x7f;		/* ~('b0); */
  src_op = 0x7f;		/* ~('b0); */
  sr_op = 0x1f;			/* ~('b0); */
#endif
  /* Disable any active sample rate */
  V2WriteReg (devc, adbarb_base + 0x400, 0);
  /* Null out all the linked lists */
  for (i = 0; i < 0x1f0; i = i + 4)
    {
      V2WriteReg (devc, adbarb_base + i, 0xffffffff);
    }
}

#ifdef USE_SRC
static void
V2DisableSrc (vortex_devc * devc)
{
  V2WriteReg (devc, (oss_native_word) (src_active_sample_rate), (oss_native_word) (0x0));	/* activate 0 and 1 */
  return;
}


static void
V2EnableSrc (vortex_devc * devc)
{
  int i, j;

  for (i = 0; i < 16; i++)
    {
      V2WriteReg (devc, (unsigned long) (src_next_ch_base + (0x4 * i)), (unsigned long) (0x0));	/* clear next ch list */
    }

  for (i = 0; i < 22; i++)
    {
      V2WriteReg (devc, (unsigned long) (src_sr_header_base + (0x4 * i)), (unsigned long) (0x0));	/* Clear header list */
    }

  for (i = 0; i < 16; i++)
    {
      for (j = 0; j < 32; j++)
	{
	  V2WriteReg (devc, (unsigned long) (src_input_fifo_base + (0x4 * ((0x20 * i) + j))), (unsigned long) (0xdeadbabe));	/* clear input fifo */
	}
    }

  for (i = 0; i < 16; i++)
    {
      for (j = 0; j < 2; j++)
	{
	  V2WriteReg (devc, (unsigned long) (src_output_fifo_base + (0x4 * ((0x2 * i) + j))), (unsigned long) (0x5555aaaa));	/* clear input fifo */
	}
    }

  for (i = 0; i < 16; i++)
    {
      V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * i)), (unsigned long) (0xc0));	/* samples per wing */
      V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * (0x10 + i))), (unsigned long) (0x45a9));	/* conversion ratio */
      V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * (0x20 + i))), (unsigned long) (0x0));	/* Drift error = 0 */
      V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * (0x30 + i))), (unsigned long) (0x0));	/* Drift error = 0  */
      V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * (0x40 + i))), (unsigned long) (0x0));	/* fraction = 0 */
      V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * (0x50 + i))), (unsigned long) (0x1));	/* drift out count = 1 */
      V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * (0x60 + i))), (unsigned long) (0x30f00));	/* conversion ratio */
    }


  V2WriteReg (devc, (unsigned long) (src_next_ch_base), (unsigned long) (0x1));	/* point to SRC1 as last in list */
  V2WriteReg (devc, (unsigned long) (src_sr_header_base + (0x04 * 20)), (unsigned long) (0x10));	/* Using spdif sr (20) point to ch 0 */
  V2WriteReg (devc, (unsigned long) (src_ch_params_base), (unsigned long) (0xc0));	/* samples per wing */
  V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04)), (unsigned long) (0xc1));	/* samples per wing */
  V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * 0x10)), (unsigned long) (0x45a9));	/* conversion ratio */
  V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * 0x11)), (unsigned long) (0x45a9));	/* conversion ratio */
  V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * 0x20)), (unsigned long) (0x0));	/* Drift error = 0 */
  V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * 0x21)), (unsigned long) (0x0));	/* Drift error = 0 */
  V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * 0x30)), (unsigned long) (0x0));	/* Drift error = 0  */
  V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * 0x31)), (unsigned long) (0x0));	/*  Drift error = 0 */

  V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * 0x40)), (unsigned long) (0x0));	/* fraction = 0  */
  V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * 0x41)), (unsigned long) (0x0));	/* fraction = 0 */
  V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * 0x50)), (unsigned long) (0x1));	/* drift out count = 1  */
  V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * 0x51)), (unsigned long) (0x1));	/* drift out count = 1 */
  V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * 0x60)), (unsigned long) (0x30f00));	/* pointers, throttle in */
  V2WriteReg (devc, (unsigned long) (src_ch_params_base + (0x04 * 0x61)), (unsigned long) (0x30f00));	/* pointers, throttle in */
  V2WriteReg (devc, (unsigned long) (src_throttle_source), (unsigned long) (0x3));	/* choose counter for ch 0 & 1 */
  V2WriteReg (devc, (unsigned long) (src_throttle_count_size), (unsigned long) (0x1ff));	/* counter size = 511 */
  V2WriteReg (devc, (unsigned long) (src_active_sample_rate), (unsigned long) (0x100000));	/* activate sr 20 for codec */

}
#endif

static void
SetBit (unsigned int owData[], int nBit, unsigned char cVal)
{
  if (cVal)
    owData[nBit / 32] |= (1 << (nBit % 32));
  else
    owData[nBit / 32] &= ~(1 << (nBit % 32));
}

/*ARGSUSED*/
static int
add_route (vortex_devc * devc, unsigned int sr,
	   unsigned int src_addr, unsigned int dst_addr, int verify)
{

  unsigned int ram_hdr_addr;
  unsigned int ram_dst_addr;
  unsigned int ram_tail_addr;
  unsigned int sr_ram_index;

  sr_ram_index = adb_destinations + sr;
  ram_hdr_addr = adbarb_wtdram_srhdr_base_addr + (sr * 4);	/* VHRDxx[sr] */
  ram_dst_addr = adbarb_wtdram_base_addr + (dst_addr * 4);	/* VDSTxx[dst_addr] */
  ram_tail_addr = adbarb_wtdram_base_addr + (devc->tail_index[sr] * 4);

  /* since always add to end of list, ram[dst_addr] will be the new tail. */
  /* and, since we could be adding to an active list, the tail needs */
  /* to be NULLed before the new link is inserted  */
  /* (since we need to check the current tail next, devc->tail_index is  */
  /* updated a bit later below.) */
  V2WriteReg (devc, ram_dst_addr, 0xffffffff);

  /* check if this sr has a list started yet */
  if (devc->tail_index[sr] == (adb_destinations + sr))
    {
      /* current tail for this sample rate indicates that list is empty, */
      /* thus this route will be head of list */
      V2WriteReg (devc, ram_hdr_addr, ((src_addr << 8) | dst_addr));
      devc->dst_index[dst_addr] = sr_ram_index;
    }
  else
    {
      /* add to end of list */
      V2WriteReg (devc, ram_tail_addr, ((src_addr << 8) | dst_addr));
      devc->dst_index[dst_addr] = devc->tail_index[sr];
    }

  /* keep track of the new tail */
  devc->tail_index[sr] = dst_addr;

  /* keep track of which sample rate list this dst_addr now belongs to */
  devc->sr_list[dst_addr] = sr;

  /* mark dst_addr as routed */
  /* devc->dst_routed[dst_addr] = 1; */
  SetBit ((unsigned int *) &devc->dst_routed, dst_addr, 1);

  return 0;
}

/*ARGSUSED*/
static void
del_route (vortex_devc * devc, unsigned char dst_addr, int verify)
{

  unsigned int data, ram_dst_addr, ram_rtd_addr;
  unsigned char sr;


  ram_dst_addr = adbarb_wtdram_base_addr + (dst_addr * 4);
  ram_rtd_addr = adbarb_wtdram_base_addr + (devc->dst_index[dst_addr] * 4);

  /* get the sr list that this dst_addr belongs to */
  sr = devc->sr_list[dst_addr];

  /* mark dst as no longer routed  */
  SetBit ((unsigned int *) &devc->dst_routed, dst_addr, 0);

  /* remove the dst from the sr_list then check to see if this was the */
  /* last route in the list.  if so, disable wtd for this sample rate */
  devc->sr_list[dst_addr] = 0x1f;	/*~('b0); */

  /* find out what's linked to us so we can reroute it.  if we are the  */
  /* tail, this will be NULL and will get relinked as such */
  data = V2ReadReg (devc, ram_dst_addr);

  /* now update the list by writing what was linked to us to where we  */
  /* were once at in the list  */
  V2WriteReg (devc, ram_rtd_addr, data);

  /* update the devc->dst_index for this reroute */
  /* devc->dst_index[data[6:0]] = devc->dst_index[dst_addr]; */
  devc->dst_index[data & 0x7f] = devc->dst_index[dst_addr];

  /* Invalidate the now deleted route.  Data for dst_addr = 7e; */
  /* NOTE: the adbarb_monitor needs this write to track properly! */
  V2WriteReg (devc, ram_dst_addr, 0xfffffffe);

  /* if we are removing the tail, reset the tail pointer */
  if (devc->tail_index[sr] == dst_addr)
    {
      devc->tail_index[sr] = devc->dst_index[dst_addr];
    }

  /* clean up all data elements used to track this dst_addr */
  /* XXX check field size below */
  devc->dst_index[dst_addr] = 0x7f;	/* ~('b0); */
}

static void
EnableCodecChannel (vortex_devc * devc, unsigned char channel)
{
  unsigned int data;

  data = V2ReadReg (devc, channel_enable_reg_addr);
  data = data | (1 << (8 + channel));	/*(1'b1 << (8+channel)); */
  V2WriteReg (devc, channel_enable_reg_addr, data);
}

static void
DisableCodecChannel (vortex_devc * devc, unsigned char channel)
{
  unsigned int data;

  data = V2ReadReg (devc, channel_enable_reg_addr);
  data = data & ~(1 << (8 + channel));	/*~(1'b1 << (8+channel)); */
  V2WriteReg (devc, channel_enable_reg_addr, data);
}

static void
EnableAdbWtd (vortex_devc * devc, int sr)
{
  unsigned int dwData;

  dwData = V2ReadReg (devc, adbarb_sr_active_addr);
  dwData |= (1 << sr);
  V2WriteReg (devc, adbarb_sr_active_addr, dwData);
  devc->sr_active |= (1 << sr);
}

static void
DisableAdbWtd (vortex_devc * devc, int sr)
{
  unsigned int dwData;

  dwData = V2ReadReg (devc, adbarb_sr_active_addr);
  dwData &= ~(1 << sr);
  V2WriteReg (devc, adbarb_sr_active_addr, dwData);
  devc->sr_active &= ~(1 << sr);
}

static void
V2SetupRoutes (vortex_devc * devc)
{

  /* Add the record routes */
  add_route (devc, 17, codec_chan0_src_addr, fifo_chan2_dst_addr, 0);
  add_route (devc, 17, codec_chan1_src_addr, fifo_chan3_dst_addr, 0);
  add_route (devc, 17, codec_chan0_src_addr, fifo_chan2a_dst_addr, 0);
  add_route (devc, 17, codec_chan1_src_addr, fifo_chan3a_dst_addr, 0);

  /* Add the playback routes */
  add_route (devc, 17, fifo_chan0_src_addr, codec_chan0_dst_addr, 0);
  add_route (devc, 17, fifo_chan0_src_addr, codec_chan1_dst_addr, 0);
}

static void
init_fifos (vortex_devc * devc)
/* */
/* Frequency to use.  1..256 which translates to nFreq/48000 */
/* if 0 then use 12 for left and 24 for right. */
/*  */
/* inputs: */
/*    nFreq -- frequency to use. */
/* outputs: */
/*    none. */
{
  int i;

  /* Zero Out all the FIFO Pointers, WT(64) & VDB(32) */
  for (i = 0; i < 96; i++)
    {
      V2WriteReg (devc, devc->fifo_base + 0x6000 + (4 * i), 0x00000020);
    }
  /* Program Channels 2,3,4,5 as record channels */
  for (i = 2; i < 6; i++)
    {
      V2WriteReg (devc, devc->fifo_base + 0x6100 + (4 * i), 0x00000000);
    }
  /* Set Trigger Levels */
  V2WriteReg (devc, devc->fifo_base + 0x7008, 0x00000843);


  /* Clear out FIFO data for channels 0-9 */
  for (i = 0; i < 10; i++)
    ClearDataFifo (devc, i);

  /* Set up the DMA engine to grab DMA memory */
  V2WriteReg (devc, devc->dma_base + 0xa80, 0);	/* Clear Dma Status0 */
  V2WriteReg (devc, devc->dma_base + 0xa84, 0);	/* Clear Dma Status1 */
  V2WriteReg (devc, devc->dma_base + 0xa88, 0);	/* Clear Dma Status2 */
  V2WriteReg (devc, devc->dma_base + 0xa8c, 0);	/* Clear Dma Status3 */
  V2WriteReg (devc, devc->dma_base + 0xa90, 0);	/* Clear Dma Status4 */
  V2WriteReg (devc, devc->dma_base + 0xa94, 0);	/* Clear Dma Status5 */
}

#ifdef USE_SRC
static void
v2setup_src (int dev)
{
  vortex_portc *portc = audio_engines[dev]->portc;
  vortex_devc *devc = audio_engines[dev]->devc;

  int i, j, chn;

  for (j = 0; j < 2; j++)
    {
      unsigned int tmp, ratio, link;
      chn = portc->voice_chn + j;

      for (i = 0; i < 128; i += 4)
	V2WriteReg (devc, src_input_fifo_base + (128 * chn) + i, 0);
      for (i = 0; i < 8; i += 4)
	V2WriteReg (devc, src_output_fifo_base + (chn * 8) + i, 0);

      ratio = 48000 / portc->speed;
      tmp = 0;
      tmp |= chn & 0xf;		/* Correlated channel */
      if (ratio > 4)
	tmp |= ((17 - ratio - 1) << 4);
      else
	tmp |= (12 << 4);	/* Zero crossing */
      V2WriteReg (devc, src_ch_params_base + 0xe00 + 4 * chn, tmp);	/* [0] */

      ratio = (48000 << 14) / portc->speed;
      V2WriteReg (devc, src_ch_params_base + 0xe40 * 4 * chn, ratio);	/* [1] */

      V2WriteReg (devc, src_ch_params_base + 0xe80 + 4 * chn, 0);	/* [2] */
      V2WriteReg (devc, src_ch_params_base + 0xec0 + 4 * chn, 0);	/* [3] */
      V2WriteReg (devc, src_ch_params_base + 0xf00 + 4 * chn, 0);	/* [4] */
      V2WriteReg (devc, src_ch_params_base + 0xf40 + 4 * chn, 1);	/* [5] */

      ratio = 48000 / portc->speed;
      tmp = 0x3000f;		/* Throttle in, FIFO depth=15 */
      V2WriteReg (devc, src_ch_params_base + 0xf80 + 4 * chn, tmp);	/* [6] */

      link = V2ReadReg (devc, src_sr_header_base + 0);
      V2WriteReg (devc, src_next_ch_base + chn * 4, link);
      V2WriteReg (devc, src_sr_header_base + 0, 0x10 | chn);

      link = V2ReadReg (devc, src_throttle_source);
      link |= (1 << chn);
      V2WriteReg (devc, src_throttle_source, link);

      link = V2ReadReg (devc, src_active_sample_rate);
      link |= (1 << 0);
      V2WriteReg (devc, src_active_sample_rate, link);

    }
}

static void
v2cleanup_src (int dev)
{
  vortex_portc *portc = audio_engines[dev]->portc;
  vortex_devc *devc = audio_engines[dev]->devc;

  int i, j, chn;

  for (j = 0; j < 2; j++)
    {
      unsigned int link;

      chn = portc->voice_chn + j;

      for (i = 0; i < 128; i += 4)
	V2WriteReg (devc, src_input_fifo_base + (128 * chn) + i, 0);
      for (i = 0; i < 8; i += 4)
	V2WriteReg (devc, src_output_fifo_base + (chn * 8) + i, 0);
      V2WriteReg (devc, src_next_ch_base + chn * 4, 0);
      V2WriteReg (devc, src_sr_header_base + 0, 0);

      link = V2ReadReg (devc, src_active_sample_rate);
      link &= ~(1 << chn);
      V2WriteReg (devc, src_active_sample_rate, link);

    }
}
#endif

/********************************************************
 *	Vortex2 MIDI Routines				*
 ********************************************************/
/*ARGSUSED*/
static int
vortex2_midi_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		   oss_midi_inputbuf_t inputbuf,
		   oss_midi_outputintr_t outputintr)
{
  vortex_devc *devc = (vortex_devc *) midi_devs[dev]->devc;

  if (devc->midi_opened)
    {
      return OSS_EBUSY;
    }

  devc->midi_input_intr = inputbyte;
  devc->midi_opened = mode;

  if (mode & OPEN_READ)
    {
      int tmp = V2ReadReg (devc, ICR);
      V2WriteReg (devc, ICR, tmp | MIDIRQST);	/* Enable MIDI interrupts */
      tmp = V2ReadReg (devc, ICR);
    }

  V2WriteReg (devc, MIDICMD, 0x000000ff);	/* Reset MIDI */
  V2WriteReg (devc, MIDICMD, 0x0000003f);	/* Enter UART mode */
  if ((V2ReadReg (devc, MIDIDAT) & 0xff) != 0xfe)
    cmn_err (CE_NOTE, "MIDI init not acknowledged\n");
  return 0;
}

/*ARGSUSED*/
static void
vortex2_midi_close (int dev, int mode)
{
  vortex_devc *devc = (vortex_devc *) midi_devs[dev]->devc;

  int tmp = V2ReadReg (devc, ICR);
  V2WriteReg (devc, ICR, tmp & ~MIDIRQST);	/* Disable MIDI interrupts */
  V2WriteReg (devc, MIDICMD, 0x000000ff);	/* Reset MIDI */

  devc->midi_opened = 0;
}

static int
vortex2_midi_out (int dev, unsigned char midi_byte)
{
  int n = 10;
  vortex_devc *devc = (vortex_devc *) midi_devs[dev]->devc;

  while ((V2ReadReg (devc, MIDISTAT) & CMDOK) && n--);
  if (V2ReadReg (devc, MIDISTAT) & CMDOK)
    return 0;
  V2WriteReg (devc, MIDIDAT, midi_byte);
  return 1;
}

/*ARGSUSED*/
static int
vortex2_midi_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t vortex2_midi_driver = {
  vortex2_midi_open,
  vortex2_midi_close,
  vortex2_midi_ioctl,
  vortex2_midi_out
};

static void
vortex2_midi_init (vortex_devc * devc)
{
  /* Derive the MIDI baud rate from 49.152 MHz clock */
  V2WriteReg (devc, GAMECTL, 0x00006100);
  V2WriteReg (devc, MIDICMD, 0x000000ff);	/* Reset MIDI */
  V2WriteReg (devc, MIDICMD, 0x0000003f);	/* Enter UART mode */

  /* All commands should return 0xfe as an acknowledgement */
  if ((V2ReadReg (devc, MIDIDAT) & 0xff) != 0xfe)
    cmn_err (CE_NOTE, "MIDI init not acknowledged\n");
  V2WriteReg (devc, MIDICMD, 0x000000ff);	/* Reset MIDI */
}

/****************************************************
 * 				OSS Audio routines 					*
 ****************************************************/

static int
ac97_read (void *devc_, int addr)
{
  vortex_devc *devc = devc_;
  int data;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  V2ReadCodecRegister (devc, addr, &data);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return data & 0xffff;
}

static int
ac97_write (void *devc_, int addr, int data)
{
  vortex_devc *devc = devc_;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  V2WriteCodecCommand (devc, addr, data);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;
}

int
vortex2intr (oss_device_t * osdev)
{
  vortex_devc *devc = (vortex_devc *) osdev->devc;
  int status;
  int i;
  int serviced = 0;

  /*
   * TODO: Fix mutexes and move the inputintr/outputintr calls outside the
   * mutex block.
   */
  /* MUTEX_ENTER (devc->mutex, flags); */
  status = V2ReadReg (devc, ISR);

  if (status & MFATERRST)
    cmn_err (CE_WARN, "Aureal Master fatal error interrupt\n");

  if (status & MPARERRST)
    cmn_err (CE_WARN, "Aureal Master parity error interrupt\n");

  if (status & TIMIRQST)	/* Timer interrupt */
    {
      V2ReadReg (devc, CODSMPLTMR);	/* Clear the interrupt */
      V2WriteReg (devc, CODSMPLTMR, 0x1000);
      serviced = 1;
    }

  if (status & (DMAENDIRQST | DMABERRST))	/* DMA end interrupt */
    {
      for (i = 0; i < MAX_PORTC; i++)
	{
	  vortex_portc *portc = &devc->portc[i];

	  if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	    {
	      dmap_t *dmap = audio_engines[portc->audiodev]->dmap_out;

	      int pos = 0, n;
	      unsigned int dmastat;

	      dmastat = V2ReadReg (devc, devc->dma_base + 0xd00 + (64 * 4));
	      pos = ((dmastat >> 12) & 0x03) * 4096 + (dmastat & 4095);
	      pos /= dmap->fragment_size;
	      if (pos < 0 || pos >= dmap->nfrags)
		pos = 0;

	      n = 0;
	      while (dmap_get_qhead (dmap) != pos && n++ < dmap->nfrags)
		oss_audio_outputintr (portc->audiodev, 0);
	    }

	  if (portc->trigger_bits & PCM_ENABLE_INPUT)
	    {
	      dmap_t *dmap = audio_engines[portc->audiodev]->dmap_in;

	      int pos = 0, n;
	      unsigned int dmastat;

	      dmastat = V2ReadReg (devc, devc->dma_base + 0xd08 + (64 * 4));

	      pos = ((dmastat >> 12) & 0x03) * 4096 + (dmastat & 4095);
	      pos /= dmap->fragment_size;
	      if (pos < 0 || pos >= dmap->nfrags)
		pos = 0;

	      n = 0;
	      while (dmap_get_qtail (dmap) != pos && n++ < dmap->nfrags)
		oss_audio_inputintr (portc->audiodev, 0);
	    }
	  V2ReadReg (devc, devc->dma_base + 0xa80);	/* Read Dma Status0 */
	  V2ReadReg (devc, devc->dma_base + 0xa84);	/* Read Dma Status1 */
	  V2ReadReg (devc, devc->dma_base + 0xa88);	/* Read Dma Status2 */
	  V2ReadReg (devc, devc->dma_base + 0xa8c);	/* Read Dma Status3 */
	  V2ReadReg (devc, devc->dma_base + 0xa90);	/* Read Dma Status4 */
	  V2ReadReg (devc, devc->dma_base + 0xa94);	/* Read Dma Status5 */
	  serviced = 1;
	}
    }

  if (status & MIDIRQST)	/* MIDI interrupt */
    {
      int uart_stat = V2ReadReg (devc, MIDISTAT);
      int n = 10;

      while (!(uart_stat & MIDIVAL) && n--)
	{
	  int d;
	  d = V2ReadReg (devc, MIDIDAT) & 0xff;

	  if (devc->midi_opened & OPEN_READ && devc->midi_input_intr)
	    devc->midi_input_intr (devc->midi_dev, d);
	  uart_stat = V2ReadReg (devc, MIDISTAT);
	}
      serviced = 1;
    }

  if (status != 0)
    {
      V2WriteReg (devc, ISR, status & 0x7ff);	/* Ack pulse interrupts */
      status = V2ReadReg (devc, ISR);
      serviced = 1;
    }
  /* MUTEX_EXIT (devc->mutex, flags); */
  return serviced;
}


static int
vortex2_set_rate (int dev, int arg)
{
  vortex_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->speed;

  if (audio_engines[dev]->flags & ADEV_FIXEDRATE)
    arg = 48000;

  if (arg > 48000)
    arg = 48000;
  if (arg < 5000)
    arg = 5000;
  portc->speed = arg;
  return portc->speed;
}

static short
vortex2_set_channels (int dev, short arg)
{
  vortex_portc *portc = audio_engines[dev]->portc;

  if (audio_engines[dev]->flags & ADEV_STEREOONLY)
    arg = 2;

  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
vortex2_set_format (int dev, unsigned int arg)
{
  vortex_portc *portc = audio_engines[dev]->portc;

  if (audio_engines[dev]->flags & ADEV_16BITONLY)
    arg = 16;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
vortex2_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void vortex2_trigger (int dev, int state);

static void
vortex2_reset (int dev)
{
  vortex2_trigger (dev, 0);

#ifdef USE_SRC
  v2cleanup_src (dev);
  del_route (devc, src_chan0_src_addr, 0);
  del_route (devc, src_chan1_src_addr, 0);
  del_route (devc, src_chan0_dst_addr, 0);
  del_route (devc, src_chan1_dst_addr, 0);
#endif

#ifdef USE_SPDIF
  del_route (devc, spdif_chan0_dst_addr, 0);
  del_route (devc, spdif_chan1_dst_addr, 0);
#endif
}

static void
vortex2_reset_input (int dev)
{
  vortex_portc *portc = audio_engines[dev]->portc;
  vortex2_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
vortex2_reset_output (int dev)
{
  vortex_portc *portc = audio_engines[dev]->portc;
  vortex2_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
vortex2_open (int dev, int mode, int open_flags)
{
  oss_native_word flags;
  vortex_portc *portc = audio_engines[dev]->portc;
  vortex_devc *devc = audio_engines[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  if (devc->open_mode & mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  devc->open_mode |= mode;

  portc->open_mode = mode;
  portc->audio_enabled &= ~mode;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static void
vortex2_close (int dev, int mode)
{
  vortex_portc *portc = audio_engines[dev]->portc;
  vortex_devc *devc = audio_engines[dev]->devc;

  vortex2_reset (dev);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  portc->audio_enabled = ~mode;
}

/*ARGSUSED*/
static void
vortex2_output_block (int dev, oss_native_word buf, int count, int fragsize,
		      int intrflag)
{
  vortex_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
vortex2_start_input (int dev, oss_native_word buf, int count, int fragsize,
		     int intrflag)
{
  vortex_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
vortex2_trigger (int dev, int state)
{
  vortex_devc *devc = audio_engines[dev]->devc;
  vortex_portc *portc = audio_engines[dev]->portc;
  unsigned int fifo_mode;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      /* Start the fifos */
	      fifo_mode = 0xc0030;
	      if (portc->channels == 2)
		{
		  fifo_mode |= 0x2;
		  V2WriteReg (devc, devc->fifo_base + 0x6100, fifo_mode);
		}
	      else
		{
		  V2WriteReg (devc, devc->fifo_base + 0x6100, fifo_mode);
		  V2WriteReg (devc, devc->fifo_base + 0x6104, fifo_mode);
		}
	      V2WriteReg (devc, CODSMPLTMR, 0x1000);	/* start timer */
	      portc->trigger_bits |= PCM_ENABLE_OUTPUT;
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
	      portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;

	      V2WriteReg (devc, CODSMPLTMR, 0x0);	/* stop timer */

	      V2WriteReg (devc, devc->fifo_base + 0x6100, 0);	/* Left Play */
	      V2WriteReg (devc, devc->fifo_base + 0x6104, 0);	/* Right Play */

	      ClearDataFifo (devc, 0);
	      ClearDataFifo (devc, 1);
	    }
	}
    }

  if (portc->open_mode & OPEN_READ)
    {
      if (state & PCM_ENABLE_INPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      /* Start the fifos */
	      fifo_mode = 0xc0010;
	      if (portc->channels == 2)
		fifo_mode |= 2;
	      V2WriteReg (devc, devc->fifo_base + 0x6108, fifo_mode);	/* LRecord  */
	      V2WriteReg (devc, devc->fifo_base + 0x610c, fifo_mode);	/* RRecord */
	      V2WriteReg (devc, CODSMPLTMR, 0x1000);	/* start timer */
	      portc->trigger_bits |= PCM_ENABLE_INPUT;
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      portc->trigger_bits &= ~PCM_ENABLE_INPUT;
	      portc->audio_enabled &= ~PCM_ENABLE_INPUT;

	      V2WriteReg (devc, CODSMPLTMR, 0x0);	/* stop timer */

	      V2WriteReg (devc, devc->fifo_base + 0x6108, 0);	/* LRecord */
	      V2WriteReg (devc, devc->fifo_base + 0x610c, 0);	/* RRecord */

	      ClearDataFifo (devc, 2);
	      ClearDataFifo (devc, 3);
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
vortex2_prepare_for_input (int dev, int bsize, int bcount)
{
  unsigned int nBufSize, ChSizeGotoReg0, ChSizeGotoReg1;
  unsigned int ch_mode, dma_base, dma_base4, dma_base8;

  dmap_t *dmap = audio_engines[dev]->dmap_in;
  vortex_devc *devc = audio_engines[dev]->devc;
  vortex_portc *portc = audio_engines[dev]->portc;
  unsigned int SAMPLES = 1024;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
#ifdef USE_SRC
  v2setup_src (dev);
#endif

#ifdef USE_SRC
  /* Add the routes */
  add_route (devc, 17, codec_chan0_src_addr, src_chan0_dst_addr, 0);
  add_route (devc, 17, codec_chan1_src_addr, src_chan1_dst_addr, 0);
  add_route (devc, 1, src_chan0_src_addr, fifo_chan2_dst_addr, 0);
  add_route (devc, 1, src_chan1_src_addr, fifo_chan3_dst_addr, 0);
#endif

  nBufSize = (SAMPLES * 2 * 2) - 1;

  ch_mode = 0x00001000;
  switch (portc->bits)
    {
    case AFMT_U8:
      ch_mode |= 0x00004000;
      break;
    case AFMT_S16_LE:
      ch_mode |= 0x00020000;
      break;
    }

  dma_base = devc->dma_base + 16 * portc->voice_chn;
  dma_base8 = devc->dma_base + 8 * portc->voice_chn;
  dma_base4 = devc->dma_base + 4 * portc->voice_chn;

  /* Left Record Channel VDB ch2 */
  V2WriteReg (devc, dma_base + 0x420, dmap->dmabuf_phys);
  V2WriteReg (devc, dma_base + 0x424, dmap->dmabuf_phys + 4096);
  V2WriteReg (devc, dma_base + 0x428, dmap->dmabuf_phys + 2 * 4096);
  V2WriteReg (devc, dma_base + 0x42c, dmap->dmabuf_phys + 3 * 4096);

  ChSizeGotoReg0 = (0xde000000) | (nBufSize << 12) | (nBufSize);
  ChSizeGotoReg1 = (0xfc000000) | (nBufSize << 12) | (nBufSize);

  V2WriteReg (devc, dma_base8 + 0x810, ChSizeGotoReg0);
  V2WriteReg (devc, dma_base8 + 0x814, ChSizeGotoReg1);
  V2WriteReg (devc, dma_base4 + 0xa08, ch_mode);

  /* Right Record Channel VDB ch3 */
  V2WriteReg (devc, dma_base + 0x430, dmap->dmabuf_phys);
  V2WriteReg (devc, dma_base + 0x434, dmap->dmabuf_phys + 4096);
  V2WriteReg (devc, dma_base + 0x438, dmap->dmabuf_phys + 2 * 4096);
  V2WriteReg (devc, dma_base + 0x43c, dmap->dmabuf_phys + 3 * 4096);

  ChSizeGotoReg0 = (0x56000000) | (nBufSize << 12) | (nBufSize);
  ChSizeGotoReg0 = (0x74000000) | (nBufSize << 12) | (nBufSize);
  V2WriteReg (devc, dma_base8 + 0x818, ChSizeGotoReg0);
  V2WriteReg (devc, dma_base8 + 0x81c, ChSizeGotoReg1);
  V2WriteReg (devc, dma_base4 + 0xa0c, ch_mode);

  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
vortex2_prepare_for_output (int dev, int bsize, int bcount)
{
  unsigned int nBufSize, ChSizeGotoReg0, ChSizeGotoReg1;
  unsigned int ch_mode, dma_base, dma_base4, dma_base8;

  dmap_t *dmap = audio_engines[dev]->dmap_out;
  vortex_devc *devc = audio_engines[dev]->devc;
  vortex_portc *portc = audio_engines[dev]->portc;
  unsigned int SAMPLES = 1024;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
#ifdef USE_SRC
  v2setup_src (dev);
  add_route (devc, 17, fifo_chan0_src_addr, src_chan0_dst_addr, 0);
  add_route (devc, 17, fifo_chan1_src_addr, src_chan1_dst_addr, 0);
  add_route (devc, 1, src_chan0_src_addr, codec_chan0_dst_addr, 0);
  add_route (devc, 1, src_chan1_src_addr, codec_chan1_dst_addr, 0);
#endif

  nBufSize = (SAMPLES * 2 * 2) - 1;

  ch_mode = 0x00003000;
  switch (portc->bits)
    {
    case AFMT_U8:
      ch_mode |= 0x00004000;
      break;

    case AFMT_S16_LE:
      ch_mode |= 0x00020000;
      break;
    }

  dma_base = devc->dma_base + 16 * portc->voice_chn;
  dma_base8 = devc->dma_base + 8 * portc->voice_chn;
  dma_base4 = devc->dma_base + 4 * portc->voice_chn;

  /* Left Playback Channel #0 */
  V2WriteReg (devc, dma_base + 0x400, dmap->dmabuf_phys);
  V2WriteReg (devc, dma_base + 0x404, dmap->dmabuf_phys + 4096);
  V2WriteReg (devc, dma_base + 0x408, dmap->dmabuf_phys + 2 * 4096);
  V2WriteReg (devc, dma_base + 0x40c, dmap->dmabuf_phys + 3 * 4096);

  ChSizeGotoReg0 = (0xde000000) | (nBufSize << 12) | (nBufSize);
  ChSizeGotoReg1 = (0xfc000000) | (nBufSize << 12) | (nBufSize);
  V2WriteReg (devc, dma_base8 + 0x800, ChSizeGotoReg0);
  V2WriteReg (devc, dma_base8 + 0x804, ChSizeGotoReg1);
  V2WriteReg (devc, dma_base4 + 0xa00, ch_mode);	/* Set Chan0 Mode */

  /* Right Playback Channel #1 */
  V2WriteReg (devc, dma_base + 0x410, dmap->dmabuf_phys);
  V2WriteReg (devc, dma_base + 0x414, dmap->dmabuf_phys + 4096);
  V2WriteReg (devc, dma_base + 0x418, dmap->dmabuf_phys + 2 * 4096);
  V2WriteReg (devc, dma_base + 0x41c, dmap->dmabuf_phys + 3 * 4096);
  ChSizeGotoReg0 = (0x56000000) | (nBufSize << 12) | (nBufSize);
  ChSizeGotoReg1 = (0x74000000) | (nBufSize << 12) | (nBufSize);
  V2WriteReg (devc, dma_base8 + 0x808, ChSizeGotoReg0);
  V2WriteReg (devc, dma_base8 + 0x80c, ChSizeGotoReg1);
  V2WriteReg (devc, dma_base4 + 0xa04, ch_mode);	/* Set Chan1 Mode */

  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
vortex2_free_buffer (int dev, dmap_t * dmap, int direction)
{
  vortex_devc *devc = audio_engines[dev]->devc;

  if (dmap->dmabuf == NULL)
    return 0;
#if 1
  CONTIG_FREE (devc->osdev, dmap->dmabuf, dmap->buffsize, TODO);
#ifdef linux
  oss_unreserve_pages ((oss_native_word) dmap->dmabuf,
		       (oss_native_word) dmap->dmabuf + 4 * 4096 - 1);
#endif

#else
  dmap->buffsize = devc->origbufsize;
  oss_free_dmabuf (dev, dmap);
#endif

  dmap->dmabuf = NULL;
  return 0;
}

/*ARGSUSED*/
static int
vortex2_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  vortex_devc *devc = audio_engines[dev]->devc;
  oss_native_word phaddr;
  /*int err; */

  if (dmap->dmabuf != NULL)
    return 0;
#if 1
  dmap->buffsize = 4 * 4096;	/* 4 subbuffers */
  dmap->dmabuf =
    CONTIG_MALLOC (devc->osdev, dmap->buffsize, MEMLIMIT_32BITS, &phaddr, TODO);
  dmap->dmabuf_phys = phaddr;
#ifdef linux
  oss_reserve_pages ((oss_native_word) dmap->dmabuf,
		     (oss_native_word) dmap->dmabuf + 4 * 4096 - 1);
#endif
#else
  if ((err = oss_alloc_dmabuf (dev, dmap, direction)) < 0)
    return err;
  devc->origbufsize = dmap->buffsize;
  dmap->buffsize = 4 * 4096;
#endif

  return 0;
}

/*ARGSUSED*/
static int
vortex2_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  vortex_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags, dmastat = 0;
  int ptr = 0;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (direction == PCM_ENABLE_OUTPUT)
    {
      dmastat = V2ReadReg (devc, devc->dma_base + 0xd00 + (64 * 4));
    }
  if (direction == PCM_ENABLE_INPUT)
    {
      dmastat = V2ReadReg (devc, devc->dma_base + 0xd08 + (64 * 4));
    }
  ptr = ((dmastat >> 12) & 0x03) * 4096 + (dmastat & 4095);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return ptr;
}

static audiodrv_t vortex2_driver = {
  vortex2_open,
  vortex2_close,
  vortex2_output_block,
  vortex2_start_input,
  vortex2_ioctl,
  vortex2_prepare_for_input,
  vortex2_prepare_for_output,
  vortex2_reset,
  NULL,
  NULL,
  vortex2_reset_input,
  vortex2_reset_output,
  vortex2_trigger,
  vortex2_set_rate,
  vortex2_set_format,
  vortex2_set_channels,
  NULL,
  NULL,
  NULL,				/* vortex2_check_input, */
  NULL,				/* vortex2_check_output, */
  vortex2_alloc_buffer,
  vortex2_free_buffer,
  NULL,
  NULL,
  vortex2_get_buffer_pointer
};

#ifdef USE_SPDIF
static void
V2EnableSpdif (vortex_devc * devc)
{
  unsigned short data;
  long n;

  for (n = 0; n <= 11; n++)
    {
      V2WriteReg (devc, spdif_ch_status_reg_base + (0x0004 * n), 0x0);
    }

  V2WriteReg (devc, spdif_ch_status_reg_base, spdif_ch_status_reg0);	/* first 4 bytes of channel status word */

  V2WriteReg (devc, spdif_ctrl_reg, spdif_cfg_dword);	/* set port to enable crc, input clock */

  data = V2ReadReg (devc, channel_enable_reg_addr);
  data = data | (1 << (18));	/*              set bits 18 and 19 to enable S/PDIF; */
  data = data | (1 << (19));	/*              set bits 18 and 19 to enable S/PDIF; */
  V2WriteReg (devc, channel_enable_reg_addr, data);
}
#endif

static void
attach_channel_vortex2 (vortex_devc * devc, int my_mixer)
{
  int adev;
  int i;
  int first_dev = 0;

  for (i = 0; i < MAX_PORTC; i++)
    {
      char tmp_name[100];
      vortex_portc *portc = &devc->portc[i];
      int caps =
	ADEV_FIXEDRATE | ADEV_AUTOMODE | ADEV_STEREOONLY | ADEV_16BITONLY;

      sprintf (tmp_name, "Aureal Vortex 2 (%s)", devc->name);
      if (i == 0)
	{
	  strcpy (tmp_name, devc->name);
	  caps |= ADEV_DUPLEX;
	}
      else
	{
	  sprintf (tmp_name, "%s (shadow)", devc->name);
	  caps |= ADEV_DUPLEX | ADEV_SHADOW;
	}

      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					tmp_name,
					&vortex2_driver,
					sizeof (audiodrv_t),
					caps,
					AFMT_U8 | AFMT_S16_LE, devc, -1)) < 0)
	{
	  adev = -1;
	  return;
	}
      else
	{
	  if (i == 0)
	    first_dev = adev;
	  audio_engines[adev]->portc = portc;
	  audio_engines[adev]->rate_source = first_dev;
	  audio_engines[adev]->fixed_rate = 48000;
	  audio_engines[adev]->min_rate = 48000;
	  audio_engines[adev]->max_rate = 48000;
	  audio_engines[adev]->vmix_flags = VMIX_MULTIFRAG;
#if 0
	  audio_engines[adev]->min_block = 4096;
	  audio_engines[adev]->max_block = 4096;
#endif
	  audio_engines[adev]->mixer_dev = my_mixer;
	  portc->voice_chn = 0;
	  portc->open_mode = 0;
	  portc->audiodev = adev;
	  portc->audio_enabled = 0;
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
	}
    }
  return;
}

int
init_vortex2 (vortex_devc * devc, int is_mx300)
{
  int my_mixer;
  int i;

  devc->global_base = (0x2a000);
  devc->dma_base = (0x27000);
  devc->midi_base = (0x28800);
  devc->fifo_base = (0x10000);
  devc->adbarb_block_base = (0x28000);
  devc->serial_block_base = (0x29000);
  devc->parallel_base = (0x22000);
  devc->src_base = (0x26000);

/*
 * Reset Vortex
 */
  V2WriteReg (devc, GCR, 0xffffffff);
  oss_udelay (1000);

  V2WriteReg (devc, GCR, V2ReadReg (devc, GCR) | GIRQEN);	/* Enable IRQ */
  V2WriteReg (devc, CODSMPLTMR, 0x0);
  V2WriteReg (devc, ICR, DMAENDIRQST | DMABERRST | TIMIRQST);
  oss_udelay (100);

  if (is_mx300)
    {
      unsigned int temp;

      temp = V2ReadReg (devc, pif_gpio_control);
      temp = 0x0c0 | temp;	/* set GPIO3 to stereo 2x mode */
      temp = 0x080 | temp;	/* enable GPIO3 */
      temp = 0xffffffbf & temp;	/* set GPIO3 to low quad mode */
      V2WriteReg (devc, pif_gpio_control, temp);
    }

  V2InitCodec (devc);
  V2SetupCodec (devc);
  V2InitAdb (devc);
  EnableCodecChannel (devc, 0);
  EnableCodecChannel (devc, 1);
  init_fifos (devc);
  EnableAdbWtd (devc, 17);
  V2SetupRoutes (devc);
#ifdef USE_SRC
  V2EnableSrc (devc);
#endif
#ifdef USE_SPDIF
  V2EnableSpdif
#endif
    /*
     * DMA controller memory is supposed to contain 0xdeadbeef after
     * reset.
     */
    if (V2ReadReg (devc, devc->dma_base + 0xcfc) != 0xdeadbeef)
    cmn_err (CE_WARN,
	     "DMA memory check returned unexpected result %08x\n",
	     V2ReadReg (devc, devc->dma_base + 0xcfc));

  my_mixer =
    ac97_install (&devc->ac97devc, "Vortex2 AC97 Mixer", ac97_read,
		  ac97_write, devc, devc->osdev);
  if (my_mixer >= 0)
    {
      devc->mixer_dev = my_mixer;
    }
  else
    return 0;


  for (i = 0; i < 2; i++)
    devc->dst_routed[i] = 0;

  attach_channel_vortex2 (devc, my_mixer);

  devc->midi_dev =
    oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "VORTEX",
			 "Aureal Vortex2 UART", &vortex2_midi_driver,
			 sizeof (midi_driver_t),
			 /*&std_midi_synth, */ NULL,
			 0, devc, devc->osdev);
  vortex2_midi_init (devc);
  devc->midi_opened = 0;
  return 1;
}

void
unload_vortex2 (oss_device_t * osdev)
{
  vortex_devc *devc = (vortex_devc *) osdev->devc;

#ifdef USE_SRC
  V2DisableSrc (devc);
#endif
  DisableCodecChannel (devc, 0);
  DisableCodecChannel (devc, 1);
  DisableAdbWtd (devc, 17);

  /* Disable routes */
  del_route (devc, codec_chan0_dst_addr, 0);
  del_route (devc, codec_chan1_dst_addr, 0);
  del_route (devc, fifo_chan2_dst_addr, 0);
  del_route (devc, fifo_chan3_dst_addr, 0);
  del_route (devc, fifo_chan2a_dst_addr, 0);
  del_route (devc, fifo_chan3a_dst_addr, 0);

  /* Disable all interrupts */
  V2WriteReg (devc, ICR, 0x00000000);
}
