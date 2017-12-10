/*
 * Purpose: Internal definitions for RME MADI and AES32 audio interfaces
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


/*
 * DMA buffer size (for one direction)
 */
#define MAX_CHANNELS	64
#define CHBUF_SIZE	(64*1024)	// 64k / channel
#define CHBUF_PAGES	(CHBUF_SIZE/4096)
#define DMABUF_SIZE	(MAX_CHANNELS*CHBUF_SIZE)	// 4 Mbytes (total)


/*
 * Monitor mixer channel
 */
#define MONITOR_CH	64 // ???

/*
 * Registers
 */
#define MADI_control	     	64
#define MADI_interruptAck  	96
#define MADI_control2	     	256
#define MADI_freq                	256	/* AES32 only */
#define MADI_midiOut0  	     	352
#define MADI_midiOut1  	     	356
#define MADI_eeprom		     	384	/* AES32 only */
#define MADI_outputEnableStart       	512
#define MADI_inputEnableStart        	768
#define MADI_PlayPageTable  	8192
#define MADI_RecPageTable   	12288	/* 12k */
#define MADI_MATRIX_MIXER_SIZE  	8192
#define MADI_mixerStart    	32768

#define MADI_status    	0
#define MADI_status2  		192
#define MADI_timecode 		128
#define MADI_midiIn0     		360
#define MADI_midiIn1     		364
#define MADI_midiStatusOut0  		384
#define MADI_midiStatusOut1  		388
#define MADI_midiStatusIn0   		392
#define MADI_midiStatusIn1   		396
#define MADI_peakrmsStart		4096
#define		MADI_inpeaks		(1024*4)
#define		MADI_playpeaks		(MADI_inpeaks+64*4)
#define		MADI_outpeaks		(MADI_playpeaks+64*4)

/*
 * Control register bits
 */
#define MADI_Start            		(1<<0)
#define MADI_Latency0         		(1<<1)
#define MADI_Latency1         		(1<<2)
#define MADI_Latency2         		(1<<3)
#define MADI_ClockModeMaster  		(1<<4)
#define MADI_AudioInterruptEnable	(1<<5)

#define MADI_Freq0  		(1<<6)
#define MADI_Freq1  		(1<<7)
#define MADI_DblSpeed 		(1<<8)
#define MADI_QuadSpeed   		(1U<<31)

#define MADI_ProBit 		(1<<9)	// AES32

#define MADI_TX_64ch_mode     		(1<<10)	// MADI
#define MADI_Emphasis    		(1<<10)	// AES32

#define MADI_AutoInput     		(1<<11)	// MADI
#define MADI_DataBit       		(1<<11)	// AES32

#define MADI_InputSrc0 		(1<<14)	// MADI
#define MADI_InputSrc1 		(1<<15)
#define MADI_SyncSrc0     		(1<<16)
#define MADI_SyncSrc1     		(1<<17)	// AES32
#define MADI_SyncSrc2     		(1<<13)	// AES32
#define MADI_SyncSrc3     		(1<<25)	// AES32
#define MADI_SMUX         		(1<<18)	// MADI
#define MADI_clr_tms      		(1<<19)

#define MADI_taxi_reset   		(1<<20)	// MADI
#define MADI_WCK48        		(1<<20)	// AES32

#define MADI_Midi0IntrEna	(1<<22)
#define MADI_Midi1IntrEna 	(1<<23)

#define MADI_LineOut 			(1<<24)

#define MADI_DS_2Wire 		(1<<26)	// AES32
#define MADI_QS_2Wire 		(1<<27)	// AES32
#define MADI_QS_4Wire   		(1<<28)	// AES32

#define MADI_wclk_sel 			(1<<30)

/*
 * Control2 register bits
 */
#define MADI_BIGENDIAN_MODE		(1<<9)

/*
 * Helper macros for the command register
 */
#define MADI_FreqMask  (MADI_Freq0|MADI_Freq1|\
			      MADI_DblSpeed|MADI_QuadSpeed)

#define MADI_LatencyMask    (MADI_Latency0|MADI_Latency1|MADI_Latency2)

#define MADI_InputMask      (MADI_InputSrc0|MADI_InputSrc1)
#define MADI_InputOptical   0
#define MADI_InputCoax   (MADI_InputSrc0)
#define MADI_SyncRefMask    (MADI_SyncSrc0|MADI_SyncSrc1|\
			      MADI_SyncSrc2|MADI_SyncSrc3)
#define MADI_SyncRef_Word   0
#define MADI_SyncRef_MADI   (MADI_SyncSrc0)

/*
 * Status register bits
 */
#define MADI_audioIntPending    (1<<0)
#define MADI_RX_64ch_mode            (1<<1)
#define MADI_AB_int             (1<<2)
#define MADI_LockStatus           (1<<3)
#define MADI_BufferPosMask 0x000FFC0
#define MADI_madiSync          (1<<18)
#define MADI_DblSpeedStatus (1<<19)

#define MADI_Freq0_status         (1<<22)
#define MADI_Freq1_status         (1<<23)
#define MADI_Freq2_status         (1<<24)
#define MADI_Freq3_status         (1<<25)

#define MADI_BufferHalf          (1<<26)

#define MADI_midi0IRQStatus   (1<<30)
#define MADI_midi1IRQStatus   (1U<<31)

#define UNITY_GAIN		32768
#define MUTE_GAIN		0

typedef struct
{
  int open_mode;
  unsigned int trigger_bits;
  int direction;
#define DIR_IN	PCM_ENABLE_INPUT
#define DIR_OUT	PCM_ENABLE_OUTPUT
  int channel;			/* Index to the first channel */

  int audio_dev;

  int max_channels;
  int channels;			/* Number of channels */
} madi_portc_t;

typedef struct
{
  oss_device_t *osdev;
  oss_mutex_t mutex;
  char *name;

  int model;
#define MDL_MADI	0
#define MDL_AES32	1

  char *registers;
  oss_native_word physaddr;

  unsigned int cmd, cmd2;	// Cached control/control2 register values

  /* Sample rate, etc. */
  unsigned int rate, next_rate;
  unsigned long long active_inputs, active_outputs;	/* Bitmasks indexed by ch# */

  /* Mixer */
  int mixer_dev;

  /* Shadow of the hw mixer gain registers */
  unsigned short mixer_values[MAX_CHANNELS][2 * MAX_CHANNELS];

  /* Playback */
  unsigned char *playbuf;
  oss_native_word playbuf_phys;
  oss_dma_handle_t play_dma_handle;
  int num_outputs;
  madi_portc_t *out_portc[MAX_CHANNELS];
  unsigned long long busy_play_channels;

  /* Recording */
  unsigned char *recbuf;
  oss_native_word recbuf_phys;
  oss_dma_handle_t rec_dma_handle;
  int num_inputs;
  madi_portc_t *in_portc[MAX_CHANNELS];
  unsigned long long busy_rec_channels;

  volatile int open_audiodevs;	/* Number of audio input/output devices currently open */
  int first_audiodev;
} madi_devc_t;

static __inline__ void
madi_write (madi_devc_t * devc, int reg, unsigned int value)
{
  PCI_WRITEL (devc->osdev, devc->registers + reg, value);
}

static __inline__ unsigned int
madi_read (madi_devc_t * devc, int reg)
{
  return PCI_READL (devc->osdev, devc->registers + reg);
}

static __inline__ void
madi_control (madi_devc_t * devc, unsigned int value)
{
  madi_write (devc, MADI_control, value);
  devc->cmd = value;
}

static __inline__ void
madi_control2 (madi_devc_t * devc, unsigned int value)
{
  madi_write (devc, MADI_control2, value);
  devc->cmd2 = value;
}

#define SRC_IN		0
#define	SRC_PLAY	64
extern void madi_write_gain (madi_devc_t * devc, unsigned int chn,
			     unsigned int src, unsigned short value);
extern int madi_read_gain (madi_devc_t * devc, unsigned int chn,
			   unsigned int src);
extern int madi_install_mixer (madi_devc_t * devc);
extern void madi_activate_mixer (madi_devc_t * devc);
