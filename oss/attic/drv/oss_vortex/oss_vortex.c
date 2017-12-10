/*
 * Purpose: Driver for Aureal Semiconductor Vortex PCI audio controller.
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

#define AUREAL_VENDOR_ID	0x12eb
#define AUREAL_VORTEX		0x0001
#define AUREAL_VORTEX2		0x0002

#define global_base	(0x12800)
#	define			GIRQSTAT	global_base+0x0
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
#	define			GIRQCTL		global_base+0x4
#	define			GSTAT		global_base+0x8
#	define			GCTL		global_base+0xc
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

#define dma_base	(0x10000)

#define midi_base	(0x11000)
#	define			MIDIDAT		midi_base+0x0
#	define			MIDICMD		midi_base+0x4
#	define			MIDISTAT	MIDICMD
#		define			MIDIVAL		0x00000080
#		define			CMDOK		0x00000040
#		define			MPUMODE		0x00000001
#	define			GAMECTL		midi_base+0xc
#		define			JOYMODE		0x00000040
#		define			MIDITXFULL	0x00000008
#		define			MIDIRXINIT	0x00000004
#		define			MIDIRXOVFL	0x00000002
#		define			MIDIDATVAL	0x00000001

#define fifo_base	(0xe000)
#define adb_destinations		103
#define adbarb_block_base		(0x10800)
#define serial_block_base		(0x11800)
#	define			CODSMPLTMR	serial_block_base+0x19c
#define adbarb_wtdram_base_addr		(adbarb_block_base + 0x000)
#define adbarb_wtdram_srhdr_base_addr 	(adbarb_block_base + (adb_destinations * 4))
#define adbarb_sr_active_addr		(adbarb_block_base + 0x200)
#define adbarb_srb_last_addr		(adbarb_block_base + 0x204)
#define codec_control_reg_addr		(serial_block_base + 0x0184)
#define cmd_status_reg_addr		(serial_block_base + 0x0188)
#define channel_enable_reg_addr		(serial_block_base + 0x0190)
#define serial_ram_reg_addr		(serial_block_base + 0x00)

#define fifo_chan0_src_addr		0x00
#define fifo_chan1_src_addr		0x01
#define fifo_chan2_dst_addr		0x02
#define fifo_chan2a_dst_addr         0x022
#define fifo_chan3_dst_addr		0x03
#define fifo_chan3a_dst_addr         0x023
#define fifo_chan4_dst_addr		0x04
#define fifo_chan5_dst_addr		0x05
#define fifo_chan6_dst_addr		0x06
#define fifo_chan7_dst_addr		0x07
#define fifo_chan8_dst_addr		0x08
#define fifo_chan9_dst_addr		0x09
#define fifo_chana_dst_addr		0x0a
#define fifo_chanb_dst_addr		0x0b
#define src_chan0_src_addr		0x10
#define src_chan1_src_addr		0x11
#define src_chan0_dst_addr		0x10
#define src_chan1_dst_addr		0x11
#define codec_chan0_src_addr	0x48
#define codec_chan1_src_addr	0x49
#define codec_chan0_dst_addr	0x58
#define codec_chan1_dst_addr	0x59

#define src_base	(0xc000)
#define src_input_fifo				(src_base + 0x000)
#define src_output_double_buffer		(src_base + 0x800)
#define src_next_channel			(src_base + 0xc00)
#define src_sr_header				(src_base + 0xc40)
#define src_active_sample_rate			(src_base + 0xcc0)
#define src_throttle_source			(src_base + 0xcc4)
#define src_throttle_count_size			(src_base + 0xcc8)
#define src_channel_params			(src_base + 0xe00)

/* static int dst_routed[256] = { 0 }; */


static unsigned int
ReadReg (vortex_devc * devc, oss_native_word addr)
{
  return READL (addr);
}

static void
WriteReg (vortex_devc * devc, oss_native_word addr, unsigned int data)
{
  WRITEL (addr, data);
}

static void
ReadCodecRegister (vortex_devc * devc, int cIndex, int *pdwData)
{
  int dwCmdStRegAddr;
  int dwCmdStRegData;
  int nCnt = 0;

  dwCmdStRegAddr = cIndex << 16;
  WriteReg (devc, cmd_status_reg_addr, dwCmdStRegAddr);
  do
    {
      dwCmdStRegData = ReadReg (devc, cmd_status_reg_addr);
      if (nCnt++ > 10)
	oss_udelay (10);
    }
  while ((dwCmdStRegData & 0x00FF0000) != ((1 << 23) | (dwCmdStRegAddr))
	 && (nCnt < 100));
  if (nCnt >= 100)
    {
      *pdwData = dwCmdStRegData;
      cmn_err (CE_WARN,
	       "timeout waiting for Status Valid bit in the Command Status Reg, index %x\n",
	       cIndex);
    }
  else
    *pdwData = dwCmdStRegData & 0x0000ffff;
}

static void
WriteCodecCommand (vortex_devc * devc, int cIndex, int wData)
{
  int dwData;
  int nCnt = 0;

  do
    {
      dwData = ReadReg (devc, codec_control_reg_addr);
      if (nCnt++ > 10)
	oss_udelay (100);
    }
  while (!(dwData & 0x0100) && (nCnt < 100));
  if (nCnt >= 100)
    cmn_err (CE_WARN, "timeout waiting for Codec Command Write OK bit.\n");


  do
    {
      dwData = ReadReg (devc, codec_control_reg_addr);
      if (nCnt++ > 1)
	oss_udelay (100);
    }
  while (!(dwData & 0x0100) && (nCnt < 100));
  if (nCnt >= 100)
    cmn_err (CE_WARN, "timeout waiting for Codec Command Write OK bit.\n");

  dwData = (cIndex << 16) | (1 << 23) | wData;
  WriteReg (devc, cmd_status_reg_addr, dwData);
  oss_udelay (1000);

  /* Read it back to make sure it got there */
  do
    {
      dwData = ReadReg (devc, codec_control_reg_addr);
      if (nCnt++ > 1)
	oss_udelay (1000);
    }
  while (!(dwData & 0x0100) && (nCnt < 100));
  if (nCnt >= 100)
    cmn_err (CE_WARN, "timeout waiting for Codec Command Write OK bit.\n");
  ReadCodecRegister (devc, cIndex, &dwData);
  if (dwData != (int) wData)
    {
      do
	{
	  dwData = ReadReg (devc, codec_control_reg_addr);
	  if (nCnt++ > 1)
	    oss_udelay (1000);
	}
      while (!(dwData & 0x0100) && (nCnt < 100));
      if (nCnt >= 1000)
	cmn_err (CE_WARN,
		 "timeout waiting for Codec Command Write OK bit.\n");
      dwData = (cIndex << 16) | (1 << 23) | wData;
      WriteReg (devc, cmd_status_reg_addr, dwData);
      do
	{
	  dwData = ReadReg (devc, codec_control_reg_addr);
	  if (nCnt++ > 1)
	    oss_udelay (100);
	}
      while (!(dwData & 0x0100) && (nCnt < 50));
      if (nCnt >= 10)
	cmn_err (CE_WARN,
		 "timeout waiting for Codec Command Write OK bit.\n");
#if 0
      ReadCodecRegister (devc, cIndex, &dwData);
      if (dwData != (int) wData)
	{
	  cmn_err (CE_WARN,
		   "Vortex ERROR: Write to index %x failed (exp %04x, got %04x)\n",
		   cIndex, wData, dwData);
	}
#endif
    }
}

static int
ac97_read (void *devc_, int addr)
{
  vortex_devc *devc = devc_;
  int data;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  ReadCodecRegister (devc, addr, &data);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return data & 0xffff;
}

static int
ac97_write (void *devc_, int addr, int data)
{
  vortex_devc *devc = devc_;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  WriteCodecCommand (devc, addr, data);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;
}

static void
ClearDataFifo (vortex_devc * devc, int nChannel)
{
  int j;
  /* Clear out FIFO data */
  for (j = 0; j < 32; j++)
    WriteReg (devc, fifo_base + (0x80 * nChannel) + (0x4 * j), 0x0);
}

static int
vortexintr (oss_device_t * osdev)
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
  status = ReadReg (devc, GIRQSTAT);

#if 0
  if (status & MFATERRST)
    cmn_err (CE_WARN, "Master fatal error interrupt\n");
  if (status & MPARERRST)
    cmn_err (CE_WARN, "Master parity error interrupt\n");
#endif

  if (status & TIMIRQST)	/* Timer interrupt */
    {
      ReadReg (devc, CODSMPLTMR);	/* Clear the interrupt */
      WriteReg (devc, CODSMPLTMR, 0x00001000);
      serviced = 1;
    }

  if (status & DMAENDIRQST)	/* DMA end interrupt */
    {
      for (i = 0; i < MAX_PORTC; i++)
	{
	  vortex_portc *portc = &devc->portc[i];

	  if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	    {
	      dmap_t *dmap = audio_engines[portc->audiodev]->dmap_out;

	      int pos = 0, n;
	      unsigned int dmastat;

	      dmastat = ReadReg (devc, dma_base + 0x5c0);
	      pos = ((dmastat >> 12) & 0x03) * 4096 + (dmastat & 4095);

	      pos /= dmap->fragment_size - 1;
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

	      dmastat = ReadReg (devc, dma_base + 0x5c8);

	      pos = ((dmastat >> 12) & 0x03) * 4096 + (dmastat & 4095);

	      pos /= dmap->fragment_size;
	      if (pos < 0 || pos >= dmap->nfrags)
		pos = 0;

	      n = 0;
	      while (dmap_get_qtail (dmap) != pos && n++ < dmap->nfrags)
		oss_audio_inputintr (portc->audiodev, 0);
	    }
	  ReadReg (devc, dma_base + 0x600);	/* Read Dma Status0 */
	  ReadReg (devc, dma_base + 0x604);	/* Read Dma Status1 */
	  ReadReg (devc, dma_base + 0x608);	/* Read Dma Status2 */
	  serviced = 1;
	}
    }

  if (status & MIDIRQST)	/* MIDI interrupt */
    {
      int uart_stat = ReadReg (devc, MIDISTAT);
      int n = 10;

      while (!(uart_stat & MIDIVAL) && n--)
	{
	  int d;
	  d = ReadReg (devc, MIDIDAT) & 0xff;

	  if (devc->midi_opened & OPEN_READ && devc->midi_input_intr)
	    devc->midi_input_intr (devc->midi_dev, d);
	  uart_stat = ReadReg (devc, MIDISTAT);
	}
      serviced = 1;
    }
  if (status != 0)
    {
      WriteReg (devc, GIRQSTAT, status & 0x7ff);	/* Ack pulse interrupts */
      serviced = 1;
    }

  /* MUTEX_EXIT (devc->mutex, flags); */
  return serviced;
}

static void
cold_reset (vortex_devc * devc)
{
  int i;
  int bSigmatelCodec = 0;

  for (i = 0; i < 32; i = i + 1)
    {
      WriteReg (devc, serial_ram_reg_addr + 0x80 + (i * 4), 0);
      oss_udelay (100);
    }
  if (bSigmatelCodec)
    {
      /* Disable clock */
      WriteReg (devc, codec_control_reg_addr, 0x00a8);
      oss_udelay (100);
      /* Set Sync High     */
      /*    WriteReg(devc, codec_control_reg_addr, 0x40a8); */
      /*    delay(100);    */
      /* Place CODEC into reset */
      WriteReg (devc, codec_control_reg_addr, 0x80a8);
      oss_udelay (100);
      /* Give CODEC some Clocks with reset asserted */
      WriteReg (devc, codec_control_reg_addr, 0x80e8);
      oss_udelay (100);
      /* Turn off clocks */
      WriteReg (devc, codec_control_reg_addr, 0x80a8);
      oss_udelay (100);
      /* Take out of reset */
      /*  WriteReg(devc, codec_control_reg_addr, 0x40a8); */
      /*  oss_udelay(100);    */
      /* Release reset     */
      WriteReg (devc, codec_control_reg_addr, 0x00a8);
      oss_udelay (100);
      /* Turn on clocks        */
      WriteReg (devc, codec_control_reg_addr, 0x00e8);
      oss_udelay (100);
    }
  else
    {
      WriteReg (devc, codec_control_reg_addr, 0x8068);
      oss_udelay (10);
      WriteReg (devc, codec_control_reg_addr, 0x00e8);
      oss_udelay (10);
    }
}

static void
InitCodec (vortex_devc * devc)
{
  int i;
  cold_reset (devc);
  for (i = 0; i < 32; i = i + 1)
    {
      WriteReg (devc, serial_ram_reg_addr + 0x80 + (i * 4), 0);
      oss_udelay (10);
    }
  /* Set up the codec in AC97 mode */
  WriteReg (devc, codec_control_reg_addr, 0x00e8);
  oss_udelay (10);
  /* Clear the channel enable register */
  WriteReg (devc, channel_enable_reg_addr, 0);
}

static void
SetupCodec (vortex_devc * devc)
{
  int dwData;
  int count = 0;
  int dwBit26 = 1 << 26;

  /* do the following only for ac97 codecs */
  /* Wait for Codec Ready (bit 26) */
  do
    {
      dwData = ReadReg (devc, codec_control_reg_addr);
      if (count++ > 1)
	oss_udelay (10);
    }
  while ((count < 100) && !(dwData & dwBit26));
  if (count >= 100)
    {
      cmn_err (CE_WARN, "Error: timeout waiting for Codec Ready bit.\n");
      cmn_err (CE_WARN, "Codec Interface Control Register is %08x\n", dwData);
    }
  /* Write interesting data to the Codec 97 Mixer registers */
  /* Master Volume 0dB Attunuation, Not muted. */
  WriteCodecCommand (devc, 0x02, 0x0f0f);
  oss_udelay (10);
  /* Master Volume mono muted. */
  WriteCodecCommand (devc, 0x06, 0x8000);
  oss_udelay (10);
  /* Mic Volume muted. */
  WriteCodecCommand (devc, 0x0e, 0x8000);
  oss_udelay (10);
  /* Line In Volume muted. */
  WriteCodecCommand (devc, 0x10, 0x8000);
  oss_udelay (10);
  /* CD Volume muted. */
  WriteCodecCommand (devc, 0x12, 0x8000);
  oss_udelay (10);
  /* Aux Volume muted. */
  WriteCodecCommand (devc, 0x16, 0x8000);
  oss_udelay (10);
  /* PCM out Volume 0 dB Gain, Not muted. */
  WriteCodecCommand (devc, 0x18, 0x0f0f);
  oss_udelay (10);
  /* Record select, select Mic for recording */
  WriteCodecCommand (devc, 0x1a, 0x0404);
  oss_udelay (10);
  /* Record Gain, 0dB */
  WriteCodecCommand (devc, 0x1c, 0x8000);
  oss_udelay (10);

  /* Poll the Section Ready bits in the Status Register (index 0x26) */
  count = 0;
  do
    {
      ReadCodecRegister (devc, 0x26, &dwData);
      if (count++ > 1)
	oss_udelay (10);
    }
  while (!(dwData & 0x02) && (count < 10));
  if (!(dwData & 0x02))
    cmn_err (CE_WARN, "DAC section ready bit is not set.\n");

  /* Read and confirm the data in the Codec 97 Mixer registers. */
  /* just the PCM reg, as a sanity check */
  ReadCodecRegister (devc, 0x18, &dwData);
  if ((dwData & 0x0000ffff) != 0xf0f)
    {
      cmn_err (CE_WARN, "PCM volume reg is %x, sb 0xf0f.\n", dwData);
    }
}

static void
InitAdb (vortex_devc * devc)
{
/*  parameter  values for write_op  */
#define op_none	  0		/* dst_op = x */
#define op_tail 	  1	/* dst_op = x */
#define op_add 	  2		/* dst_op = dst_addr being added */
#define op_adds 	  3	/* dst_op = dst_addr being added */
#define op_del		  4	/* dst_op = dst_addr being deleted */
#define op_dels	  5		/* dst_op = dst_addr being deleted */
#define op_inval	  6	/* dst_op = x */

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
  write_op = op_none;
  dst_op = 0x7f;		/* ~('b0); */
  src_op = 0x7f;		/* ~('b0); */
  sr_op = 0x1f;			/* ~('b0); */
#endif
  /* Disable any active sample rate */
  WriteReg (devc, 0x10800 + 0x200, 0);
  /* Null out all the linked lists */
  for (i = 0; i < 0x1f0; i = i + 4)
    {
      WriteReg (devc, 0x10800 + i, 0xffffffff);
    }
}

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
  WriteReg (devc, ram_dst_addr, 0xffffffff);

  /* check if this sr has a list started yet */
  if (devc->tail_index[sr] == (adb_destinations + sr))
    {
      /* current tail for this sample rate indicates that list is empty, */
      /* thus this route will be head of list */
      WriteReg (devc, ram_hdr_addr, ((src_addr << 7) | dst_addr));
      devc->dst_index[dst_addr] = sr_ram_index;
    }
  else
    {
      /* add to end of list */
      WriteReg (devc, ram_tail_addr, ((src_addr << 7) | dst_addr));
      devc->dst_index[dst_addr] = devc->tail_index[sr];
    }

  /* keep track of the new tail */
  devc->tail_index[sr] = dst_addr;

  /* keep track of which sample rate list this dst_addr now belongs to */
  devc->sr_list[dst_addr] = sr;

  /* mark dst_addr as routed */
  /* dst_routed[dst_addr] = 1; */
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
  data = ReadReg (devc, ram_dst_addr);

  /* now update the list by writing what was linked to us to where we  */
  /* were once at in the list  */
  WriteReg (devc, ram_rtd_addr, data);

  /* update the devc->dst_index for this reroute */
  /* devc->dst_index[data[6:0]] = devc->dst_index[dst_addr]; */
  devc->dst_index[data & 0x7f] = devc->dst_index[dst_addr];

  /* Invalidate the now deleted route.  Data for dst_addr = 7e; */
  /* NOTE: the adbarb_monitor needs this write to track properly! */
  WriteReg (devc, ram_dst_addr, 0xfffffffe);

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
SetupRoutes (vortex_devc * devc)
{
  /* First add the record routes */
  add_route (devc, 17, codec_chan0_src_addr, fifo_chan2_dst_addr, 0);
  add_route (devc, 17, codec_chan1_src_addr, fifo_chan3_dst_addr, 0);
  add_route (devc, 17, codec_chan0_src_addr, fifo_chan2a_dst_addr, 0);
  add_route (devc, 17, codec_chan1_src_addr, fifo_chan3a_dst_addr, 0);

  /* Now add the playback routes */
  add_route (devc, 17, fifo_chan0_src_addr, codec_chan0_dst_addr, 0);
  add_route (devc, 17, fifo_chan0_src_addr, codec_chan1_dst_addr, 0);
}

static void
EnableCodecChannel (vortex_devc * devc, unsigned char channel)
{
  unsigned int data;

  data = ReadReg (devc, channel_enable_reg_addr);
  data = data | (1 << (8 + channel));	/*(1'b1 << (8+channel)); */
  WriteReg (devc, channel_enable_reg_addr, data);
}

static void
DisableCodecChannel (vortex_devc * devc, unsigned char channel)
{
  unsigned int data;

  data = ReadReg (devc, channel_enable_reg_addr);
  data = data & ~(1 << (8 + channel));	/*~(1'b1 << (8+channel)); */
  WriteReg (devc, channel_enable_reg_addr, data);
}

static void
EnableAdbWtd (vortex_devc * devc, int sr)
{
  unsigned int dwData;

  dwData = ReadReg (devc, adbarb_sr_active_addr);
  dwData |= (1 << sr);
  WriteReg (devc, adbarb_sr_active_addr, dwData);
  devc->sr_active |= (1 << sr);
}

static void
DisableAdbWtd (vortex_devc * devc, int sr)
{
  unsigned int dwData;

  dwData = ReadReg (devc, adbarb_sr_active_addr);
  dwData &= ~(1 << sr);
  WriteReg (devc, adbarb_sr_active_addr, dwData);
  devc->sr_active &= ~(1 << sr);
}

/*ARGSUSED*/
static int
vortex_midi_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
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
      int tmp = ReadReg (devc, GIRQCTL);
      WriteReg (devc, GIRQCTL, tmp | MIDIRQST);	/* Enable MIDI interrupts */
      tmp = ReadReg (devc, GIRQCTL);
    }

  WriteReg (devc, MIDICMD, 0x000000ff);	/* Reset MIDI */
  WriteReg (devc, MIDICMD, 0x0000003f);	/* Enter UART mode */
  if ((ReadReg (devc, MIDIDAT) & 0xff) != 0xfe)
    cmn_err (CE_WARN, "Vortex warning! MIDI init not acknowledged\n");
  return 0;
}

/*ARGSUSED*/
static void
vortex_midi_close (int dev, int mode)
{
  vortex_devc *devc = (vortex_devc *) midi_devs[dev]->devc;

  int tmp = ReadReg (devc, GIRQCTL);
  WriteReg (devc, GIRQCTL, tmp & ~MIDIRQST);	/* Disable MIDI interrupts */
  WriteReg (devc, MIDICMD, 0x000000ff);	/* Reset MIDI */

  devc->midi_opened = 0;
}

static int
vortex_midi_out (int dev, unsigned char midi_byte)
{
  int n = 10;
  vortex_devc *devc = (vortex_devc *) midi_devs[dev]->devc;

  while ((ReadReg (devc, MIDISTAT) & CMDOK) && n--);
  if (ReadReg (devc, MIDISTAT) & CMDOK)
    return 0;
  WriteReg (devc, MIDIDAT, midi_byte);
  return 1;
}

/*ARGSUSED*/
static int
vortex_midi_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t vortex_midi_driver = {
  vortex_midi_open,
  vortex_midi_close,
  vortex_midi_ioctl,
  vortex_midi_out
};

static void
vortex_midi_init (vortex_devc * devc)
{
  /* Derive the MIDI baud rate from 49.152 MHz clock */
  WriteReg (devc, GAMECTL, 0x00006100);
  WriteReg (devc, MIDICMD, 0x000000ff);	/* Reset MIDI */
  WriteReg (devc, MIDICMD, 0x0000003f);	/* Enter UART mode */

  /* All commands should return 0xfe as an acknowledgement */
  if ((ReadReg (devc, MIDIDAT) & 0xff) != 0xfe)
    cmn_err (CE_WARN, "Vortex warning! MIDI init not acknowledged\n");
  WriteReg (devc, MIDICMD, 0x000000ff);	/* Reset MIDI */
}

/***********************************
 * Audio routines 
 ***********************************/

static int
vortex_set_rate (int dev, int arg)
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
vortex_set_channels (int dev, short arg)
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
vortex_set_format (int dev, unsigned int arg)
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
vortex_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

#ifdef USE_SRC
static void cleanup_src (int dev);
#endif

static void vortex_trigger (int dev, int state);

static void
vortex_reset (int dev)
{
  vortex_trigger (dev, 0);

#ifdef USE_SRC
  cleanup_src (dev);
  del_route (devc, src_chan0_dst_addr, 0);
  del_route (devc, src_chan1_dst_addr, 0);
#endif
}

static void
vortex_reset_input (int dev)
{
  vortex_portc *portc = audio_engines[dev]->portc;
  vortex_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
vortex_reset_output (int dev)
{
  vortex_portc *portc = audio_engines[dev]->portc;
  vortex_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
vortex_open (int dev, int mode, int open_flags)
{
  oss_native_word flags;
  vortex_portc *portc = audio_engines[dev]->portc;
  vortex_devc *devc = audio_engines[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode != 0)
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
vortex_close (int dev, int mode)
{
  vortex_portc *portc = audio_engines[dev]->portc;
  vortex_devc *devc = audio_engines[dev]->devc;

  vortex_reset (dev);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  portc->audio_enabled &= ~mode;
}

/*
 * InitFifos
 */
static void
init_fifos (vortex_devc * devc)
{
  int i;

  /* Zero Out the FIFO Pointers */
  for (i = 0; i < 48; i++)
    {
      WriteReg (devc, fifo_base + 0x1800 + (4 * i), 0x00000020);
    }
  for (i = 2; i < 6; i++)
    {
      WriteReg (devc, fifo_base + 0x1800 + (4 * i), 0x00000000);
    }
  WriteReg (devc, fifo_base + 0x18c0, 0x00000843);
  /* Clear out FIFO data */
  for (i = 0; i < 4; i++)
    ClearDataFifo (devc, i);

  /* Set up the DMA engine to grab DMA memory */
  WriteReg (devc, dma_base + 0x61c, 0);	/* Clear Dma Status0 */
  WriteReg (devc, dma_base + 0x620, 0);	/* Clear Dma Status1 */
  WriteReg (devc, dma_base + 0x624, 0);	/* Clear Dma Status2 */
}

#ifdef USE_SRC
static void
setup_src (int dev)
{
  vortex_portc *portc = audio_engines[dev]->portc;
  vortex_devc *devc = audio_engines[dev]->devc;

  int i, j, chn;

  for (j = 0; j < 2; j++)
    {
      unsigned int tmp, ratio, link;
      chn = portc->voice_chn + j;

      for (i = 0; i < 128; i += 4)
	WriteReg (devc, src_input_fifo + (128 * chn) + i, 0);
      for (i = 0; i < 8; i += 4)
	WriteReg (devc, src_output_double_buffer + (chn * 8) + i, 0);

      ratio = 48000 / portc->speed;
      tmp = 0;
      tmp |= chn & 0xf;		/* Correlated channel */
      if (ratio > 4)
	tmp |= ((17 - ratio - 1) << 4);
      else
	tmp |= (12 << 4);	/* Zero crossing */
      WriteReg (devc, src_channel_params + 0xe00 + 4 * chn, tmp);	/* [0] */

      ratio = (48000 << 14) / portc->speed;
      WriteReg (devc, src_channel_params + 0xe40 * 4 * chn, ratio);	/* [1] */

      WriteReg (devc, src_channel_params + 0xe80 + 4 * chn, 0);	/* [2] */
      WriteReg (devc, src_channel_params + 0xec0 + 4 * chn, 0);	/* [3] */
      WriteReg (devc, src_channel_params + 0xf00 + 4 * chn, 0);	/* [4] */
      WriteReg (devc, src_channel_params + 0xf40 + 4 * chn, 1);	/* [5] */

      ratio = 48000 / portc->speed;
      tmp = 0x3000f;		/* Throttle in, FIFO depth=15 */
      WriteReg (devc, src_channel_params + 0xf80 + 4 * chn, tmp);	/* [6] */

      link = ReadReg (devc, src_sr_header + 0);
      WriteReg (devc, src_next_channel + chn * 4, link);
      WriteReg (devc, src_sr_header + 0, 0x10 | chn);

      link = ReadReg (devc, src_throttle_source);
      link |= (1 << chn);
      WriteReg (devc, src_throttle_source, link);

      link = ReadReg (devc, src_active_sample_rate);
      link |= (1 << 0);
      WriteReg (devc, src_active_sample_rate, link);

    }
}

static void
cleanup_src (int dev)
{
  vortex_portc *portc = audio_engines[dev]->portc;
  vortex_devc *devc = audio_engines[dev]->devc;

  int i, j, chn;

  for (j = 0; j < 2; j++)
    {
      chn = portc->voice_chn + j;

      for (i = 0; i < 128; i += 4)
	WriteReg (devc, src_input_fifo + (128 * chn) + i, 0);
      for (i = 0; i < 8; i += 4)
	WriteReg (devc, src_output_double_buffer + (chn * 8) + i, 0);
      WriteReg (devc, src_next_channel + chn * 4, 0);
      WriteReg (devc, src_sr_header + 0, 0);

      WriteReg (devc, src_active_sample_rate, 0);

    }
}
#endif

/*ARGSUSED*/
static void
vortex_output_block (int dev, oss_native_word buf, int count, int fragsize,
		     int intrflag)
{
  vortex_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
vortex_start_input (int dev, oss_native_word buf, int count, int fragsize,
		    int intrflag)
{
  vortex_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
vortex_trigger (int dev, int state)
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
	      /* Start the FIFOs */
	      fifo_mode = 0x30030;
	      if (portc->channels == 2)
		{
		  fifo_mode |= 0x00000002;
		  WriteReg (devc, fifo_base + 0x1800, fifo_mode);	/* Left Pb */
		}
	      else
		{
		  WriteReg (devc, fifo_base + 0x1800, fifo_mode);	/* Left Pb */
		  WriteReg (devc, fifo_base + 0x1804, fifo_mode);	/* Right Pb */
		}

	      WriteReg (devc, CODSMPLTMR, 0x1000);	/* start timer */
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

	      WriteReg (devc, CODSMPLTMR, 0x0);	/* stop timer */
	      WriteReg (devc, fifo_base + 0x1800, 0);	/* Left Playback */
	      WriteReg (devc, fifo_base + 0x1804, 0);	/* Right Playback */

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
	      /* Start the FIFOs */
	      fifo_mode = 0x30010;
	      if (portc->channels == 2)
		fifo_mode |= 0x00000002;
	      WriteReg (devc, fifo_base + 0x1808, fifo_mode);	/* Left Rec */
	      WriteReg (devc, fifo_base + 0x180c, fifo_mode);	/* Right Rec */
	      WriteReg (devc, CODSMPLTMR, 0x1000);	/* start timer */
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

	      WriteReg (devc, CODSMPLTMR, 0x0);	/* stop timer */

	      WriteReg (devc, fifo_base + 0x1808, 0);	/* Left Record */
	      WriteReg (devc, fifo_base + 0x180c, 0);	/* Right Record */

	      ClearDataFifo (devc, 2);
	      ClearDataFifo (devc, 3);
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
vortex_prepare_for_input (int dev, int bsize, int bcount)
{
  unsigned int nBufSize, ChSizeGotoReg0, ChSizeGotoReg1, ch_mode;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  vortex_devc *devc = audio_engines[dev]->devc;
  vortex_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;
  int SAMPLES = 1024;


  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

#ifdef USE_SRC
  setup_src (dev);
  add_route (devc, 17, src_chan0_src_addr, codec_chan0_dst_addr, 0);
  add_route (devc, 17, src_chan1_src_addr, codec_chan1_dst_addr, 0);

  add_route (devc, 0, fifo_chan0_src_addr, src_chan0_dst_addr, 0);
  if (portc->channels == 2)
    add_route (devc, 0, fifo_chan0_src_addr, src_chan1_dst_addr, 0);
  else
    add_route (devc, 0, fifo_chan1_src_addr, src_chan1_dst_addr, 0);
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

    case AFMT_MU_LAW:
      ch_mode |= 0x00008000;
      break;

    case AFMT_A_LAW:
      ch_mode |= 0x0000c000;
      break;

    }

  /* Left Record Channel VDB Chan 2  */
  WriteReg (devc, dma_base + 0x220, dmap->dmabuf_phys);	/* Set Chan0 Address */
  WriteReg (devc, dma_base + 0x224, dmap->dmabuf_phys + 4096);	/* Set Chan0 Address */
  WriteReg (devc, dma_base + 0x228, dmap->dmabuf_phys + 2 * 4096);	/* Set Chan0 Address */
  WriteReg (devc, dma_base + 0x22c, dmap->dmabuf_phys + 3 * 4096);	/* Set Chan0 Address */

  ChSizeGotoReg0 = (0xde000000) | (nBufSize << 12) | (nBufSize);
  ChSizeGotoReg1 = (0xfc000000) | (nBufSize << 12) | (nBufSize);
  WriteReg (devc, dma_base + 0x410, ChSizeGotoReg0);	/* Set Chan0 Size to SAMPLES*2*2, Loop over 0 */
  WriteReg (devc, dma_base + 0x414, ChSizeGotoReg1);	/* Set Chan0 Size to SAMPLES*2*2, Loop over 0 */
  WriteReg (devc, dma_base + 0x588, ch_mode);	/* Set Chan0 Mode */

  /* Right Record Channel VDB Chan 3 */
  WriteReg (devc, dma_base + 0x230, dmap->dmabuf_phys);	/* Set Chan1 Address */
  WriteReg (devc, dma_base + 0x234, dmap->dmabuf_phys + 4096);	/* Set Chan0 Address */
  WriteReg (devc, dma_base + 0x238, dmap->dmabuf_phys + 2 * 4096);	/* Set Chan0 Address */
  WriteReg (devc, dma_base + 0x23c, dmap->dmabuf_phys + 3 * 4096);	/* Set Chan0 Address */

  ChSizeGotoReg0 = (0x56000000) | (nBufSize << 12) | (nBufSize);
  ChSizeGotoReg1 = (0x74000000) | (nBufSize << 12) | (nBufSize);
  WriteReg (devc, dma_base + 0x418, ChSizeGotoReg0);
  WriteReg (devc, dma_base + 0x41c, ChSizeGotoReg1);
  WriteReg (devc, dma_base + 0x58c, ch_mode);	/* Set Chan1 Mode */

  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
vortex_prepare_for_output (int dev, int bsize, int bcount)
{
  unsigned int nBufSize, ChSizeGotoReg0, ChSizeGotoReg1, ch_mode;
  int SAMPLES = 1024;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  vortex_devc *devc = audio_engines[dev]->devc;
  vortex_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

#ifdef USE_SRC
  setup_src (dev);
  add_route (devc, 17, src_chan0_src_addr, codec_chan0_dst_addr, 0);
  add_route (devc, 17, src_chan1_src_addr, codec_chan1_dst_addr, 0);

  add_route (devc, 0, fifo_chan0_src_addr, src_chan0_dst_addr, 0);
  if (portc->channels == 2)
    add_route (devc, 0, fifo_chan0_src_addr, src_chan1_dst_addr, 0);
  else
    add_route (devc, 0, fifo_chan1_src_addr, src_chan1_dst_addr, 0);
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

    case AFMT_MU_LAW:
      ch_mode |= 0x00008000;
      break;

    case AFMT_A_LAW:
      ch_mode |= 0x0000c000;
      break;

    }

  /* Left Playback Channel VDB Chan 0 */
  WriteReg (devc, dma_base + 0x200, dmap->dmabuf_phys);	/* Set Chan0 Address */
  WriteReg (devc, dma_base + 0x204, dmap->dmabuf_phys + 4096);	/* Set Chan0 Address */
  WriteReg (devc, dma_base + 0x208, dmap->dmabuf_phys + 2 * 4096);	/* Set Chan0 Address */
  WriteReg (devc, dma_base + 0x20c, dmap->dmabuf_phys + 3 * 4096);	/* Set Chan0 Address */

  ChSizeGotoReg0 = (0xde000000) | (nBufSize << 12) | (nBufSize);
  ChSizeGotoReg1 = (0xfc000000) | (nBufSize << 12) | (nBufSize);
  WriteReg (devc, dma_base + 0x400, ChSizeGotoReg0);	/* Set Chan0 Size to SAMPLES*2*2, Loop over 0 */
  WriteReg (devc, dma_base + 0x404, ChSizeGotoReg1);	/* Set Chan0 Size to SAMPLES*2*2, Loop over 0 */
  WriteReg (devc, dma_base + 0x580, ch_mode);	/* Set Chan0 Mode */

  /* Right Playback Channel VDB Chan 1 */
  WriteReg (devc, dma_base + 0x210, dmap->dmabuf_phys);	/* Set Chan1 Address */
  WriteReg (devc, dma_base + 0x214, dmap->dmabuf_phys + 4096);	/* Set Chan1 Address */
  WriteReg (devc, dma_base + 0x218, dmap->dmabuf_phys + 2 * 4096);	/* Set Chan1 Address */
  WriteReg (devc, dma_base + 0x21c, dmap->dmabuf_phys + 3 * 4096);	/* Set Chan1 Address */

  ChSizeGotoReg0 = (0x56000000) | (nBufSize << 12) | (nBufSize);
  ChSizeGotoReg1 = (0x74000000) | (nBufSize << 12) | (nBufSize);
  WriteReg (devc, dma_base + 0x408, ChSizeGotoReg0);
  WriteReg (devc, dma_base + 0x40c, ChSizeGotoReg1);

  WriteReg (devc, dma_base + 0x584, ch_mode);	/* Set Chan1 Mode */

  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
vortex_free_buffer (int dev, dmap_t * dmap, int direction)
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
  dmap->buffsize += 4096;	/* Return the stolen page back */
  oss_free_dmabuf (dev, dmap);
#endif

  dmap->dmabuf = NULL;
  return 0;
}

/*ARGSUSED*/
static int
vortex_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  vortex_devc *devc = audio_engines[dev]->devc;
  oss_native_word phaddr;

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

  /* Reserve the last page for internal use (channel buffers) */
  p = dmap->buffsize - 4096;
  dmap->buffsize = p;

  if (dmap->buffsize < (4 * 1024))
    {
      cmn_err (CE_WARN, "Allocated DMA buffer smaller than 8k\n");
      vortex_free_buffer (dev, dmap, direction);
      return OSS_ENOSPC;
    }
#endif

  return 0;
}

/*ARGSUSED*/
static int
vortex_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  vortex_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags, dmastat = 0;
  int ptr = 0;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (direction == PCM_ENABLE_OUTPUT)
    {
      dmastat = ReadReg (devc, dma_base + 0x5c0);
    }
  if (direction == PCM_ENABLE_INPUT)
    {
      dmastat = ReadReg (devc, dma_base + 0x5c8);
    }
  ptr = ((dmastat >> 12) & 0x03) * 4096 + (dmastat & 4095);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return ptr;
}

static audiodrv_t vortex_driver = {
  vortex_open,
  vortex_close,
  vortex_output_block,
  vortex_start_input,
  vortex_ioctl,
  vortex_prepare_for_input,
  vortex_prepare_for_output,
  vortex_reset,
  NULL,
  NULL,
  vortex_reset_input,
  vortex_reset_output,
  vortex_trigger,
  vortex_set_rate,
  vortex_set_format,
  vortex_set_channels,
  NULL,
  NULL,
  NULL,				/* vortex_check_input, */
  NULL,				/* vortex_check_output, */
  vortex_alloc_buffer,
  vortex_free_buffer,
  NULL,
  NULL,
  vortex_get_buffer_pointer
};

static void
attach_channel (vortex_devc * devc, int my_mixer)
{
  int adev, i;
  int first_dev = 0;

  for (i = 0; i < MAX_PORTC; i++)
    {
      char tmp_name[100];
      vortex_portc *portc = &devc->portc[i];
      int caps =
	ADEV_FIXEDRATE | ADEV_AUTOMODE | ADEV_STEREOONLY | ADEV_16BITONLY;

      if (i == 0)
	{
	  sprintf (tmp_name, "Aureal Vortex (%s)", devc->name);
	  caps |= ADEV_DUPLEX;
	}
      else
	{
	  sprintf (tmp_name, "Aureal Vortex (%s) (shadow)", devc->name);
	  caps |= ADEV_DUPLEX | ADEV_SHADOW;
	}
      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					tmp_name,
					&vortex_driver,
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
	  audio_engines[adev]->vmix_flags = VMIX_MULTIFRAG;
	  audio_engines[adev]->rate_source = first_dev;
	  audio_engines[adev]->fixed_rate = 48000;
	  audio_engines[adev]->min_rate = 48000;
	  audio_engines[adev]->max_rate = 48000;
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

#ifdef USE_SRC
static void
init_src (vortex_devc * devc)
{
  int i;

  for (i = 0; i < 0x800; i += 4)
    WriteReg (devc, src_input_fifo + i, 0);
  for (i = 0; i < 0x080; i += 4)
    WriteReg (devc, src_output_double_buffer + i, 0);
  for (i = 0; i < 0x040; i += 4)
    WriteReg (devc, src_next_channel + i, 0);
  for (i = 0; i < 0x054; i += 4)
    WriteReg (devc, src_sr_header + i, 0);

  WriteReg (devc, src_active_sample_rate, 0);
  WriteReg (devc, src_throttle_source, 0);
  WriteReg (devc, src_throttle_count_size, 511);

  for (i = 0; i < 0x1bf; i += 4)
    WriteReg (devc, src_channel_params + i, 0);
}
#endif

int init_vortex2 (vortex_devc * devc, int is_mx300);
int vortex2intr (oss_device_t * osdev);

static int
init_vortex (vortex_devc * devc)
{
  int my_mixer;
  int i;

/*
 * Reset Vortex
 */
  WriteReg (devc, GCTL, 0xffffffff);
  oss_udelay (1000);

  InitCodec (devc);
  SetupCodec (devc);
  InitAdb (devc);
  EnableCodecChannel (devc, 0);
  EnableCodecChannel (devc, 1);
  init_fifos (devc);
  EnableAdbWtd (devc, 17);
  SetupRoutes (devc);
#ifdef USE_SRC
  init_src (devc);
#endif

  /*
   * DMA controller memory is supposed to contain 0xdeadbeef after
   * reset.
   */
  if (ReadReg (devc, 0x10000 + 0x7c0) != 0xdeadbeef)
    cmn_err (CE_WARN,
	     "DMA memory check returned unexpected result %08x\n",
	     ReadReg (devc, 0x10000 + 0x7c0));

  my_mixer = ac97_install (&devc->ac97devc, "AC97 Mixer", ac97_read,
			   ac97_write, devc, devc->osdev);
  if (my_mixer >= 0)
    devc->mixer_dev = my_mixer;
  else
    return 0;

  WriteReg (devc, GCTL, ReadReg (devc, GCTL) | GIRQEN);	/* Enable IRQ */
  WriteReg (devc, CODSMPLTMR, 0x0);
  WriteReg (devc, GIRQCTL, DMAENDIRQST | DMABERRST | TIMIRQST);

  for (i = 0; i < 2; i++)
    devc->dst_routed[i] = 0;

  /* Turn on vortex serial interface outputs to codec */
  EnableCodecChannel (devc, 0);
  EnableCodecChannel (devc, 1);

  /* Zero Out the FIFO Pointers */
  for (i = 0; i < 48; i++)
    {
      WriteReg (devc, fifo_base + 0x1800 + (4 * i), 0x00000020);
    }
  for (i = 2; i < 6; i++)
    {
      WriteReg (devc, fifo_base + 0x1800 + (4 * i), 0x00000000);
    }
  WriteReg (devc, fifo_base + 0x18c0, 0x00000843);
  /* Clear out FIFO data */
  for (i = 0; i < 4; i++)
    ClearDataFifo (devc, i);

  /* Set up the DMA engine to grab DMA memory */
  WriteReg (devc, dma_base + 0x61c, 0);	/* Clear Dma Status0 */
  WriteReg (devc, dma_base + 0x620, 0);	/* Clear Dma Status1 */
  WriteReg (devc, dma_base + 0x624, 0);	/* Clear Dma Status2 */

  attach_channel (devc, my_mixer);

  devc->midi_dev = oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "VORTEX",
					"Aureal Vortex UART",
					&vortex_midi_driver,
					sizeof (midi_driver_t),
					/* &std_midi_synth, */ NULL,
					0, devc, devc->osdev);
  devc->midi_opened = 0;
  vortex_midi_init (devc);

  return 1;
}

int
oss_vortex_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision /*, pci_latency */ ;
  unsigned short pci_command, vendor, device;
  unsigned short subsystemid, subvendor;
  int is_mx300 = 0;
  vortex_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered Aureal Vortex probe routine\n"));

  oss_pci_byteswap (osdev, 1);

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != AUREAL_VENDOR_ID || (device != AUREAL_VORTEX &&
				     device != AUREAL_VORTEX2))
    return 0;

  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);

  pci_read_config_word (osdev, PCI_SUBSYSTEM_ID, &subsystemid);
  pci_read_config_word (osdev, PCI_SUBSYSTEM_VENDOR_ID, &subvendor);

  if (pci_irq_line == 0)
    {
      cmn_err (CE_WARN, "IRQ not assigned by BIOS (%d).\n", pci_irq_line);
      return 0;
    }

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;

  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_0, &devc->bar0addr);

  if (device == AUREAL_VORTEX2)
    {
      devc->name = "Aureal Vortex AU8830";
      devc->id = AUREAL_VORTEX2;
      devc->bar0_size = 256 * 1024;
    }
  else
    {
      devc->name = "Aureal Vortex AU8820";
      devc->id = AUREAL_VORTEX;
      devc->bar0_size = 128 * 1024;
    }

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY;
  pci_command &= ~(PCI_COMMAND_SERR | PCI_COMMAND_PARITY);
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  if (subvendor == 0x1092 && subsystemid == 0x3001)
    is_mx300 = 1;

  /* Prgram IORDY for VIA chipset failure */
  pci_write_config_byte (osdev, 0x40, 0xff);

  /* Map the shared memory area */
  devc->bar0virt =
    (unsigned int *) MAP_PCI_MEM (devc->osdev, 0, devc->bar0addr,
				  devc->bar0_size);
  devc->dwRegister = devc->bar0virt;

  /* Disable all interrupts */
  WriteReg (devc, GIRQCTL, 0x00000000);

  oss_register_device (osdev, devc->name);

  if (devc->id == AUREAL_VORTEX)
    if (oss_register_interrupts (devc->osdev, 0, vortexintr, NULL) < 0)
      {
	cmn_err (CE_WARN, "Can't allocate IRQ%d\n", pci_irq_line);
	return 0;
      }

  if (devc->id == AUREAL_VORTEX2)
    if (oss_register_interrupts (devc->osdev, 0, vortex2intr, NULL) < 0)
      {
	cmn_err (CE_WARN, "Can't allocate IRQ%d\n", pci_irq_line);
	return 0;
      }


  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  if (devc->id == AUREAL_VORTEX2)
    return init_vortex2 (devc, is_mx300);
  else
    return init_vortex (devc);	/* Detected */
}


extern void unload_vortex2 (oss_device_t * osdev);

int
oss_vortex_detach (oss_device_t * osdev)
{
  vortex_devc *devc = (vortex_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  if (devc->id == AUREAL_VORTEX2)
    unload_vortex2 (osdev);
  else
    {
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
      WriteReg (devc, GIRQCTL, 0x00000000);
    }

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);

  UNMAP_PCI_MEM (devc->osdev, 0, devc->bar0addr, devc->bar0virt,
		 devc->bar0_size);

  oss_unregister_device (devc->osdev);
  return 1;
}
