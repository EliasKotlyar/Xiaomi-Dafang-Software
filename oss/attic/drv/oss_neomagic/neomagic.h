/*
 * Purpose: Definitions for the neomagic driver
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

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

/* The revisions that we currently handle.  */
enum neomagicrev
{
  REV_NEOMAGICAV, REV_NEOMAGICZX
};

#define MAX_PORTC 2

/* The two memory ports.  */
typedef struct neomagic_portc
{
  unsigned int physaddr;	/* Physical address of the port. */
  char *ptr;			/* Our mapped-in pointer. */
  unsigned int start_offset;	/* PTR's offset within the physical port.  */
  unsigned int end_offset;	/* And the offset of the end of the buffer.  */
  unsigned int samplerate;
  unsigned char bits;
  unsigned char stereo;
}
neomagic_portc;


typedef struct neomagic_devc
{
  /* OS struct for OSS */
  oss_device_t *osdev;

  /* Our IRQ number. */
  int irq;

  /* Mutex */
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* Magic number used to verify that this struct is valid. */
#define NM_MAGIC_SIG 0x55aa00ff
  int magsig;

  enum neomagicrev rev;		/* Revision number */

  char *chip_name;		/* Chip name. */

  ac97_devc ac97devc;

  int dev[2];			/* Our audio device numbers. */

  int opencnt[2];		/* The # of times each device has been opened. */

  /* We use two devices, because we can do simultaneous play and record.
     This keeps track of which device is being used for what purpose;
     these are the actual device numbers. */
  int dev_for_play;
  int dev_for_record;

  int mixer_dev;		/* The mixer device. */

  /* 
   * Can only be opened once for each operation.  These aren't set
   * until an actual I/O operation is performed; this allows one
   * device to be open for read/write without inhibiting I/O to
   * the other device.
   */
  int is_open_play;
  int is_open_record;

  int playing;			/* Non-zero if we're currently playing a sample. */
  int recording;		/* Same for recording a sample. */

  neomagic_portc portc[MAX_PORTC];

  /* The following are offsets within memory port 1. */
  unsigned int coeffBuf;
  unsigned int allCoeffBuf;

  /* Record and playback buffers. */
  unsigned int abuf1, abuf2;
  char *abuf1virt, *abuf2virt;

  /* Offset of the mixer status register in memory port 2.  */
  unsigned int mixer_status_offset;

  /* 
   * Status mask bit; (*mixer_status_loc & mixer_status_mask) == 0 means
   * it's ready.  
   */
  unsigned short mixer_status_mask;

  /* The sizes of the playback and record ring buffers. */
  unsigned int playbackBufferSize;
  unsigned int recordBufferSize;

  /* The devc interrupt service routine. */
  int (*introutine) (struct neomagic_devc *);

  int mixer_cache[64];
}
neomagic_devc;

/* The BIOS signature. */
#define NM_SIGNATURE 0x4e4d0000
/* Signature mask. */
#define NM_SIG_MASK 0xffff0000

/* Size of the second memory area. */
#define NM_PORT2_SIZE 4096

/* The base offset of the mixer in the second memory area. */
#define NM_MIXER_OFFSET 0x600

/* The maximum size of a coefficient entry. */
#define NM_MAX_COEFFICIENT 0x5000

/* The interrupt register. */
#define NM_INT_REG 0xa04
/* And its bits. */
#define NM_PLAYBACK_INT 0x40
#define NM_RECORD_INT 0x100
#define NM_MISC_INT_1 0x4000
#define NM_MISC_INT_2 0x1
#define NM_ACK_INT(CARD, X) neomagic_writePort16((CARD), 2, NM_INT_REG, (X) << 1)

/* The AV's "mixer ready" status bit and location. */
#define NM_MIXER_STATUS_OFFSET 0xa06
#define NM_MIXER_READY_MASK 0x0800
#define NM_PRESENCE_MASK 0x0050
#define NM_PRESENCE_VALUE 0x0040

/*
 * For the ZX.  It uses the same interrupt register, but it holds 32
 * bits instead of 16.
 */
#define NMZX_PLAYBACK_INT 0x10000
#define NMZX_RECORD_INT 0x80000
#define NMZX_MISC_INT_1 0x8
#define NMZX_MISC_INT_2 0x2
#define NMZX_ACK_INT(CARD, X) neomagic_writePort32((CARD), 2, NM_INT_REG, (X))

/* The ZX's "mixer ready" status bit and location. */
#define NMZX_MIXER_STATUS_OFFSET 0xa08
#define NMZX_MIXER_READY_MASK 0x0800

/* The playback registers start from here. */
#define NM_PLAYBACK_REG_OFFSET 0x0
/* The record registers start from here. */
#define NM_RECORD_REG_OFFSET 0x200

/* The rate register is located 2 bytes from the start of the register area. */
#define NM_RATE_REG_OFFSET 2

/* Mono/stereo flag, number of bits on playback, and rate mask. */
#define NM_RATE_STEREO 1
#define NM_RATE_BITS_16 2
#define NM_RATE_MASK 0xf0

/* Playback enable register. */
#define NM_PLAYBACK_ENABLE_REG (NM_PLAYBACK_REG_OFFSET + 0x1)
#define NM_PLAYBACK_ENABLE_FLAG 1
#define NM_PLAYBACK_ONESHOT 2
#define NM_PLAYBACK_FREERUN 4

/* Mutes the audio output. */
#define NM_AUDIO_MUTE_REG (NM_PLAYBACK_REG_OFFSET + 0x18)
#define NM_AUDIO_MUTE_LEFT 0x8000
#define NM_AUDIO_MUTE_RIGHT 0x0080

/* Recording enable register. */
#define NM_RECORD_ENABLE_REG (NM_RECORD_REG_OFFSET + 0)
#define NM_RECORD_ENABLE_FLAG 1
#define NM_RECORD_FREERUN 2

#define NM_RBUFFER_START (NM_RECORD_REG_OFFSET + 0x4)
#define NM_RBUFFER_END   (NM_RECORD_REG_OFFSET + 0x10)
#define NM_RBUFFER_WMARK (NM_RECORD_REG_OFFSET + 0xc)
#define NM_RBUFFER_CURRP (NM_RECORD_REG_OFFSET + 0x8)

#define NM_PBUFFER_START (NM_PLAYBACK_REG_OFFSET + 0x4)
#define NM_PBUFFER_END   (NM_PLAYBACK_REG_OFFSET + 0x14)
#define NM_PBUFFER_WMARK (NM_PLAYBACK_REG_OFFSET + 0xc)
#define NM_PBUFFER_CURRP (NM_PLAYBACK_REG_OFFSET + 0x8)

/* A few trivial routines to make it easier to work with the registers
   on the chip. */

/* This is a common code portion used to fix up the port offsets. */
#define NM_FIX_PORT \
  if (port < 1 || port > 2 || devc == NULL) \
      return 0; \
\
    if (offset < devc->portc[port - 1].start_offset \
	|| offset >= devc->portc[port - 1].end_offset) { \
	cmn_err(CE_WARN, "Bad access: port %d, offset 0x%x\n", port, offset); \
	return 0; \
    } \
    offset -= devc->portc[port - 1].start_offset;

#define DEFwritePortX(X, func) \
static inline int neomagic_writePort##X (neomagic_devc *devc,\
				      int port, int offset, int value)\
{\
    U##X *addr;\
\
    DDB (cmn_err(CE_CONT, "Writing 0x%x to %d:0x%x\n", value, port, offset));\
\
    NM_FIX_PORT;\
\
    addr = (U##X *)(devc->portc[port - 1].ptr + offset);\
    func (value, addr);\
    return 0;\
}

static inline void
nm_writeb (unsigned char value, unsigned char *ptr)
{
  *ptr = value;
}

static inline void
nm_writew (unsigned short value, unsigned short *ptr)
{
  *ptr = value;
}

static inline void
nm_writel (unsigned int value, unsigned int *ptr)
{
  *ptr = value;
}


DEFwritePortX (8, nm_writeb)
DEFwritePortX (16, nm_writew) DEFwritePortX (32, nm_writel)
#define DEFreadPortX(X, func) \
static inline U##X neomagic_readPort##X (neomagic_devc *devc,\
					int port, int offset)\
{\
    U##X *addr;\
\
    NM_FIX_PORT\
\
    addr = (U##X *)(devc->portc[port - 1].ptr + offset);\
    return func(addr);\
}
     static inline unsigned char
     nm_readb (unsigned char *ptr)
{
  return *ptr;
}

static inline unsigned short
nm_readw (unsigned short *ptr)
{
  return *ptr;
}

static inline unsigned int
nm_readl (unsigned int *ptr)
{
  return *ptr;
}

DEFreadPortX (8, nm_readb)
DEFreadPortX (16, nm_readw)
DEFreadPortX (32, nm_readl)

static inline int
neomagic_writeBuffer8 (neomagic_devc * devc, unsigned char *src,
			    int port, int offset, int amt)
{
  NM_FIX_PORT;
  memcpy (devc->portc[port - 1].ptr + offset, src, amt);
  return 0;
}

#if 0
static inline int
neomagic_readBuffer8 (neomagic_devc * devc, unsigned char *dst, int port,
		      int offset, int amt)
{
  NM_FIX_PORT;
  memcpy (dst, devc->portc[port - 1].ptr + offset, amt);
  return 0;
}
#endif
