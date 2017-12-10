/*
 * Purpose: Global definitions for the SB Live/Audigy driver
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

#ifndef USERLAND
#include "uart401.h"
#endif

#define EMU10K1_MAGIC		0xe10001
#define EMU10K2_MAGIC		0xe10002

/* Audio */

#if defined(OSR5) || defined(__bsdi__)
#define MAX_ADEV        2
#else
#define MAX_ADEV			32	/* How many devices */
#endif

#define DMABUF_SIZE		(64*1024)	/* Maximum DMA buffer size supported */
#define AUDIO_MAXVOICE		(2*MAX_ADEV)
#define AUDIO_MEMSIZE		(MAX_ADEV*DMABUF_SIZE+4096)	/* Audio buffer + silent page */
#define SYNTH_MEMBASE		AUDIO_MEMSIZE

/* Synth */
#define MAX_PATCH			256
#define MAX_SAMPLE			512
#define MAX_VOICE 			64

#define SYNTH_FIRSTVOICE		AUDIO_MAXVOICE
#define SYNTH_LASTVOICE			(MAX_VOICE-1)

/* Synth memory allocation */
#define SBLIVE_MEMBLOCK_SIZE	(128*1024)	/* Default synth mem alloc chunk size */
#define MIN_BLOCK_SIZE			(8*1024)
#define SBLIVE_MAX_MEMBLOCKS	1024	/* Max number of mem chunks to allocate */

/* Hardware config register */

#define HCFG_CODECFORMAT_MASK	0x00070000	/* CODEC format */
#define HCFG_CODECFORMAT_AC97	0x00000000	/* AC97 CODEC format -- Primary Output */
#define HCFG_CODECFORMAT_I2S	0x00010000	/* I2S CODEC format -- Secondary (Rear) Output  */
#define HCFG_GPINPUT0		0x00004000	/* External pin112 */
#define HCFG_GPINPUT1		0x00002000	/* External pin110 */
#define HCFG_GPOUTPUT_MASK	0x00001c00	/* External pins which may be controlled */
#define HCFG_GPOUT0		0x00001000	/* set to enable digital out on 5.1 cards */
#define HCFG_GPOUT1             0x00000800	/* IR */
#define HCFG_GPOUT2             0x00000400	/* IR */
#define HCFG_JOYENABLE      	0x00000200	/* Internal joystick enable */
#define HCFG_PHASETRACKENABLE	0x00000100	/* Phase tracking enable */
						/* 1 = Force all 3 async digital inputs to use  */
						/* the same async sample rate tracker (ZVIDEO)  */
#define HCFG_AC3ENABLE_MASK	0x0x0000e0	/* AC3 async input control - Not implemented    */
#define HCFG_AC3ENABLE_ZVIDEO	0x00000080	/* Channels 0 and 1 replace ZVIDEO              */
#define HCFG_AC3ENABLE_CDSPDIF	0x00000040	/* Channels 0 and 1 replace CDSPDIF             */
#define HCFG_AC3ENABLE_GPSPDIF  0x00000020	/* Channels 0 and 1 replace GPSPDIF             */
#define HCFG_AUTOMUTE		0x00000010	/* When set, the async sample rate convertors   */
						/* will automatically mute their output when    */
						/* they are not rate-locked to the external     */
						/* async audio source                           */
#define HCFG_LOCKSOUNDCACHE	0x00000008	/* 1 = Cancel bustmaster accesses to soundcache */
						/* NOTE: This should generally never be used.   */
#define HCFG_LOCKTANKCACHE_MASK	0x00000004	/* 1 = Cancel bustmaster accesses to tankcache  */
						/* NOTE: This should generally never be used.   */
#define HCFG_LOCKTANKCACHE	0x01020014
#define HCFG_MUTEBUTTONENABLE	0x00000002	/* 1 = Master mute button sets AUDIOENABLE = 0. */
						/* NOTE: This is a 'cheap' way to implement a   */
						/* master mute function on the mute button, and */
						/* in general should not be used unless a more  */
						/* sophisticated master mute function has not   */
						/* been written.                                */
#define HCFG_AUDIOENABLE	0x00000001	/* 0 = CODECs transmit zero-valued samples      */
						/* Should be set to 1 when the EMU10K1 is       */
						/* completely initialized.                      */
#define A_HCFG_VMUTE		0x00004000
#define A_HCFG_AUTOMUTE		0x00008000
#define A_HCFG_XM		0x00040000	/* Xtended address mode */

/*
 * GPIO bit definitions (global register 0x18) for Audigy.
 */

#define A_IOCFG_GPOUT0          0x0044	/* analog/digital? */
#define A_IOCFG_GPOUT1          0x0002	/* IR */
#define A_IOCFG_GPOUT2          0x0001	/* IR */

/* Status bits (read only) */
#define GPIO_VERSAPLUGGED	0x2000	/* Center/LFE/digital */
#define GPIO_FRONTPLUGGED	0x4000
#define GPIO_REARPLUGGED	0x8000
#define GPIO_HEADPHPLUGGED	0x0100
#define GPIO_ANALOG_MUTE	0x0040
#define GPIO_DIGITAL_ENABLE	0x0004	/* Center/lfe (0) or digital (1) switch */

#define FILL_PAGE_MAP_ENTRY(e, v) devc->page_map[e] = LSWAP (((v) << devc->emu_page_shift) | (e));
/*
 * Audio block registers 
 */

#define CPF		0x000	/* DW:cnl   Current pitch and fraction */
#define CPF_CURRENTPITCH_MASK   0xffff0000	/* Current pitch (linear, 0x4000 == unity pitch shift)  */
#define CPF_CURRENTPITCH        0x10100000
#define CPF_STEREO_MASK         0x00008000	/* 1 = Even channel interleave, odd channel locked      */
#define CPF_STOP_MASK           0x00004000	/* 1 = Current pitch forced to 0                        */
#define CPF_FRACADDRESS_MASK    0x00003fff	/* Linear fractional address of the current channel     */


#define PTAB		0x001	/* DW:cnl   Pitch target and sends A and B */
#define PTRX_PITCHTARGET_MASK   0xffff0000	/* Pitch target of specified channel                    */
#define PTRX_PITCHTARGET        0x10100001
#define PTRX_FXSENDAMOUNT_A_MASK 0x0000ff00	/* Linear level of channel output sent to FX send bus A */
#define PTRX_FXSENDAMOUNT_A     0x08080001
#define PTRX_FXSENDAMOUNT_B_MASK 0x000000ff	/* Linear level of channel output sent to FX send bus B */
#define PTRX_FXSENDAMOUNT_B     0x08000001


#define CVCF		0x002	/* DW:cnl   Curr vol and curr filter cutoff */
#define VTFT		0x003	/* DW:cnl   Volume tgt and filter cutoff tgt */
#define Z2		0x004	/* DW:cnl   Filter delay memory 2 */
#define Z1		0x005	/* DW:cnl   Filter delay memory 1 */
#define SCSA		0x006	/* DW:cnl   Send C and Start addr */
#define SDL		0x007	/* DW:cnl   Send D and Loop addr */
#define QKBCA		0x008	/* DW:cnl   Filter Q, ROM, etc */
#undef CCR
#define CCR		0x009
#define CCR_CACHEINVALIDSIZE    0x07190009
#define CCR_CACHEINVALIDSIZE_MASK       0xfe000000	/* Number of invalid samples cache for this channel     */
#define CCR_CACHELOOPFLAG       0x01000000	/* 1 = Cache has a loop service pending                 */
#define CCR_INTERLEAVEDSAMPLES  0x00800000	/* 1 = A cache service will fetch interleaved samples   */
#define CCR_WORDSIZEDSAMPLES    0x00400000	/* 1 = A cache service will fetch word sized samples    */
#define CCR_READADDRESS         0x06100009
#define CCR_READADDRESS_MASK    0x003f0000	/* Location of cache just beyond current cache service  */
#define CCR_LOOPINVALSIZE       0x0000fe00	/* Number of invalid samples in cache prior to loop     */
						/* NOTE: This is valid only if CACHELOOPFLAG is set     */
#define CCR_LOOPFLAG            0x00000100	/* Set for a single sample period when a loop occurs    */
#define CCR_CACHELOOPADDRHI     0x000000ff	/* DSL_LOOPSTARTADDR's hi byte if CACHELOOPFLAG is set  */

#define CLP		0x00a
#define SRHE		0x07c
#define STHE		0x07d
#define SRDA		0x07e
#define STDA		0x07f
#define L_FXRT		0x00b
#define FXRT		((devc->feature_mask&SB_AUDIGY)? 0x7d:0x00b)	/* W:cnl */
#define MAPA		0x00c
#define MAPB		0x00d
#define VEV		0x010	/* W:cnl */
#define VEHA		0x011	/* W:cnl */
#define VEDS		0x012	/* W:cnl */
#define MLV		0x013	/* W:cnl */
#define MEV		0x014	/* W:cnl */
#define MEHA		0x015	/* W:cnl */
#define MEDS		0x016	/* W:cnl */
#define VLV		0x017	/* W:cnl */
#define IP		0x018	/* W:cnl */
#define IFA		0x019	/* W:cnl */
#define PEFE		0x01a	/* W:cnl */
#define PEFE_PITCHAMOUNT_MASK   0x0000ff00	/* Pitch envlope amount */
#define PEFE_PITCHAMOUNT        0x0808001a
#define PEFE_FILTERAMOUNT_MASK  0x000000ff	/* Filter envlope amount */
#define PEFE_FILTERAMOUNT       0x0800001a

#define VFM		0x01b	/* W:cnl */
#define TMFQ		0x01c	/* W:cnl */
#define VVFQ		0x01d	/* W:cnl */
#define TMPE		0x01e	/* W:cnl */
#define CD0		0x020	/* DW:cnl (16 registers) */
#define PTBA		0x040	/* DW:nocnl */
#define TCBA		0x041	/* DW:nocnl */
#define ADCSR		0x042	/* B:nocnl */
#define FXWC		0x043	/* DW:nocnl */
#define TCBS		0x044	/* B:nocnl */
#define MBA		0x045	/* DW:nocnl */
#define ADCBA		0x046	/* DW:nocnl */
#define FXBA		0x047	/* DW:nocnl */

#define MBS		0x049	/* B:nocnl */
#define ADCBS		0x04a	/* B:nocnl */
#define FXBS		0x04b	/* B:nocnl */
#define CSBA	0x4c
#define CSDC	0x4d
#define CSFE	0x4e
#define CSHG	0x4f
#define CDCS		0x050	/* DW:nocnl */
#define GPSCS		0x051	/* DW:nocnl */
#define DBG		0x052	/* DW:nocnl */
#define AUDIGY_DBG	0x053	/* DW:nocnl */
#define SCS0		0x054	/* DW:nocnl */
#define SCS1		0x055	/* DW:nocnl */
#define SCS2		0x056	/* DW:nocnl */
#define CLIEL		0x058	/* DW:nocnl */
#define CLIEH		0x059	/* DW:nocnl */
#define CLIPL		0x05a	/* DW:nocnl */
#define CLIPH		0x05b	/* DW:nocnl */
#define SOLL		0x05c	/* DW:nocnl */
#define SOLH		0x05d	/* DW:nocnl */
#define SOC		0x05e	/* DW:nocnl */
#define AC97SLOT	0x05f
#define AC97SLOT_REAR_RIGHT	0x01
#define AC97SLOT_REAR_LEFT	0x02
#define AC97SLOT_CENTER		0x10
#define AC97SLOT_LFE		0x20
#define CDSRCS		0x060	/* DW:nocnl */
#define GPSRCS		0x061	/* DW:nocnl */
#define ZVSRCS		0x062	/* DW:nocnl */
#define ADCIDX		0x063	/* W:nocnl */
#define MIDX		0x064	/* W:nocnl */
#define FXIDX		0x065	/* W:nocnl */

/* Half loop interrupt registers (audigy only) */
#define HLIEL		0x066	/* DW:nocnl */
#define HLIEH		0x067	/* DW:nocnl */
#define HLIPL		0x068	/* DW:nocnl */
#define HLIPH		0x069	/* DW:nocnl */
#define GPR0		((devc->feature_mask&SB_LIVE)? 0x100:0x400)	/* DW:nocnl */
#define TMA0		0x300	/* Tank memory */
#define UC0		((devc->feature_mask&SB_LIVE) ? 0x400:0x600)	/* DSM microcode memory */

/* Interrupt enable register */
#define IE	0x0c
#	define		IE_RXA		0x00000001
#	define		IE_IT		0x00000004
#	define		IE_AB		0x00000040

/* EMU10K2 MIDI UART */
#define MUADAT		0x070
#define MUACMD		0x071
#define MUASTAT		MUACMD

/* EMU10K2 S/PDIF recording buffer */
#define SPRI		0x6a
#define SPRA		0x6b
#define SPRC		0x6c

#define EHC		0x76	/* Audigy 2 */

#define SRHE	0x07c
#define STHE	0x07d
#define SRDA	0x07e

#define HCFG_GPOUT0             0x00001000	/* set to enable digital out on 5.1 cards */
#define HCFG_GPOUT1             0x00000800	/* IR on SBLive */
#define HCFG_GPOUT2             0x00000400	/* IR on SBLive */
#define HCFG_JOYENABLE          0x00000200	/* Internal joystick enable */

#define A_IOCFG_GPOUT0          0x0044	/* analog/digital? */
#define A_IOCFG_GPOUT1          0x0002	/* IR on Audigy */
#define A_IOCFG_GPOUT2          0x0001	/* IR on Audigy */

#define ROM0            0x00000000	/* interpolation ROM 0 */
#define ROM1            0x02000000	/* interpolation ROM 1 */
#define ROM2            0x04000000	/* interpolation ROM 2 */
#define ROM3            0x06000000	/* interpolation ROM 3 */
#define ROM4            0x08000000	/* interpolation ROM 4 */
#define ROM5            0x0A000000	/* interpolation ROM 5 */
#define ROM6            0x0C000000	/* interpolation ROM 6 */
#define ROM7            0x0E000000	/* interpolation ROM 7 */
#define BYTESIZE        0x01000000	/* byte sound memory */

#define MAX_GPR	256
#define MAX_GPR_PARMS	60
#define MAX_CONST_PARMS	128
#define GPR_NAME_SIZE 32
typedef struct
{
  char name[GPR_NAME_SIZE];
  unsigned int num;
  int type;
  int def;
}
gpr_t;

typedef struct
{
  unsigned int gpr;
  int value;
}
const_t;

typedef struct
{
  unsigned int ngpr;

  gpr_t gpr[MAX_GPR_PARMS];
}
gpr_info;

typedef struct
{
  unsigned int nconst;

  const_t consts[MAX_CONST_PARMS];
}
const_info;

typedef struct sblive_portc
{
  int audiodev;
  int mode;
  int input_type;
#define ITYPE_ANALOG	0
#define ITYPE_SPDIF	1
  int uses_spdif;		/* This device uses the S/PDIF passthrough channel */
  int audio_active;
  volatile int trigger_bits;
  int format, speed, channels;
  int speedsel;

  int voice_chn;
  int port_number;
  int out_sz;			/* Output sample size */
  int in_szbits;
  unsigned long rec_starttime;

  /* 3D soft mixer */
  int playvol, playangle, playdist;
  int vu_left, vu_right;
  int speaker_mode;
#define SMODE_FRONT		0	/* Front speakers only */
#define SMODE_SURR		1	/* Rear speakers only */
#define SMODE_FRONTREAR		2	/* Front and rear speakers */
#define SMODE_BIND		3	/* Use channel bindings */
#define SMODE_3D		4	/* 3D positioning */
  int binding;
  unsigned char *routing;
  int resetvol;
  int memptr;
}
sblive_portc;

typedef struct
{
  int active;
  int program;
  int sample;
  int note_num, note_freq, orig_freq;
  int velosity;
  struct patch_info *patch;
  int sample_ptr;
  int fixed_pitch;
  int main_vol, expression_vol, panning, frontrear, bender, bender_range;
}
sblive_voice_t;

typedef struct sblive_devc
{
  oss_native_word base;
  int irq;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;
  oss_device_t *osdev;
  char *card_name;

  /*
   * Device feature mask tells which kind of features are suported by the
   * hardware. Audigy2/2val have multiple bits set while Live! has just
   * the SB_LIVE bits. So Features of Audigy will be reported by Audigy2/val
   * too.
   */
  int feature_mask;
#define SB_LIVE		1
#define SB_AUDIGY	2
#define SB_AUDIGY2	4
#define SB_AUDIGY2VAL	8
  int mpu_attached;

  int *page_map;		/* Table for up to 8k pointers to pages of 4k */
  unsigned char **vpage_map;	/* Virtual address map */
  oss_dma_handle_t page_map_dma_handle;
  int emu_page_shift;
  int max_mem, max_pages, nr_pages;
  unsigned int subvendor;
/*
 * Mixer
 */
  int mixer_dev;
  ac97_devc ac97devc;
  int input_routing_pc;
  int input_sel;		/* 0=AC97 */
  gpr_info *gpr;
  int mixer_group;
  int gpr_values[MAX_GPR];
  int extinfo_loaded;
  int passthrough_gpr;
  int vu_tmp, vu_tmp2;

/*
 * Audio
 */

  int n_audiodevs;
  int min_audiodevs;
  int audio_memptr;
  int *silent_page;
  oss_dma_handle_t silent_page_dma_handle;
  oss_native_word silent_page_phys;

  sblive_portc portc[MAX_ADEV + 2];
  int recording_dev;
  int spdif_busy;

  int autoreset;
  int speaker_mode;

/*
 * Wave table RAM alloc structures
 */
  int memblock_size;		/* Size of blocks to be allocated */
  int num_memblocks;
  void *memblocks[SBLIVE_MAX_MEMBLOCKS];
  int memblock_sizes[SBLIVE_MAX_MEMBLOCKS];
  int total_memblock_size;

/*
 * Audigy UART
 */
  oss_midi_inputbyte_t midi_input_intr;
  int midi_opened, midi_disabled;
  volatile unsigned char input_byte;
  int midi_dev;
  int sysex_p;
  unsigned char sysex_buf[20];
  uart401_devc uart401devc;

/*
 * Wave table
 */

  int synthdev;
  int synth_open;
  int synth_membase, synth_memlimit, synth_memptr, synth_memtop;
  unsigned int voice_busy[2];
  int free_sample;
  struct patch_info *samples;
  long sample_ptrs[MAX_SAMPLE + 1];
  int programs[MAX_PATCH];

  sblive_voice_t voices[MAX_VOICE];

}
sblive_devc;

/*
 * Private ioctl() interface
 */

typedef struct
{
  unsigned int reg;
  unsigned int chn;
  unsigned int value;
}
sblive_reg;

typedef struct
{
  int magic;
  int feature_mask;
  int size;			/* # of instructions */
  unsigned int code[1024];
  gpr_info parms;
  const_info consts;
}
emu10k1_file;

typedef unsigned int sblive_code[512];

#define SBLIVE_READREG		__SIOWR('L', 1, sblive_reg)
#define SBLIVE_WRITEREG		__SIOW ('L', 2, sblive_reg)
#define SBLIVE_WRITECODE1	__SIOW ('L', 3, sblive_code)
#define SBLIVE_WRITECODE2	__SIOW ('L', 4, sblive_code)
#define SBLIVE_WRITEPARMS	__SIOW ('L', 5, gpr_info)
#define SBLIVE_WRITECONST	__SIOW ('L', 6, const_info)
#define SBLIVE_GETCHIPTYPE	__SIOR ('L', 7, int)
#define SBLIVE_WRITEGPIO	__SIOW ('L', 8, int)
#define SBLIVE_READGPIO		__SIOR ('L', 9, int)

#define EMU_MIXT_EQ1		0x10000000
#define EMU_MIXT_EQ2		0x10000001
#define EMU_MIXT_EQ3		0x10000002
#define EMU_MIXT_EQ4		0x10000003

/*
 * Overridden mixer controls (GPR registers)
 * Note that these definitions heve to be kept in sync with
 * init_compiler() routine of sndkit/sblive/asm10k1.c and the
 * mixer_override() routine of sblive.c.
 */
#define GPR_DUMMY		0	/* 2 locations reserved for NULL control */
#define GPR_PCM			2	/* 2 locations required for stereo slider */
#define GPR_VOLUME		4	/* 2 locations required for stereo slider */
#define NEXT_FREE_GPR	6	/* Needs to be 32 or below so that SPECIAL_GPRS works */
#define SPECIAL_GPRS ((1<<GPR_PCM) | (1<<GPR_VOLUME))

#ifdef OSS_BIG_ENDIAN
static __inline__ unsigned int
swap_int (unsigned int x)
{
  return ((x & 0x000000ff) << 24) |
    ((x & 0x0000ff00) << 8) |
    ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24);
}

static __inline__ unsigned short
swap_short (unsigned short x)
{
  return ((x | 0xff) << 8) | ((x >> 8) & 0xff);
}

#define LSWAP(x) swap_int(x)
#define SSWAP(x) swap_short(x)
#else
#define LSWAP(x) 	x
#define SSWAP(x) 	x
#endif
extern void sblive_set_loop_stop (sblive_devc * devc, int voice, int s);
extern void sblive_set_voice_intr (sblive_devc * devc, int voice, int s);
extern void sblive_write_reg (sblive_devc * devc, int reg, int chn,
			      unsigned int value);
extern unsigned int sblive_read_reg (sblive_devc * devc, int reg, int chn);
extern void sblive_init_voice (sblive_devc * devc, int chn);
extern void sblive_synth_interrupt (sblive_devc * devc);
extern int sblive_get_voice_loopintr (sblive_devc * devc, int voice);
extern int sblive_get_voice_halfloopintr (sblive_devc * devc, int voice);
