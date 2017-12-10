/*
 *      srcmgr.c  --  ESS Technology allegro audio driver.
 *
 *      Copyright (C) 1992-2000  Don Kim (don.kim@esstech.com)
 *
 */

#include "hckernel.h"
static int num_errors = 0; /* Dummy variable used to supress lint warnings. */

#define DMAREC

/*/////////////////////////////////////////////////////////////////////////// */
/*  DSP kernel support */
/* index 0 - 44.1k; 1 - 32k; 2 - 22.05k; 3 - 11.025k; 4 - 8k */
/* */
/*/////////////////////////////////////////////////////////////////////////// */
WORD gwSRC3_PB_SIZE_OUT[5] =
  { 37 * 2 * 2, 6 * 2, 37 * 2 * 2, 26 * 2, 12 * 2 };
WORD gwSRC3_PB_SIZE_IN[5] = { 34 * 2, 4, 17 * 2, 6, 2 };
WORD gwSRC3_PB_FILTER_LENGTH[5] = { 666, 60, 666, 338, 360 };
WORD gwSRC3_PB_FILTER_TAP_SIZE[5] = { 17, 19, 17, 25, 59 };	/* SIZE - 1 */
WORD gwSRC3_PB_BIG_LOOP_COUNTER[5] = { 11, 1, 5, 2, 0 };
WORD gwSRC3_PB_SMALL_LOOP_COUNTER[5] = { 0, 0, 1, 3, 4 };
WORD gwSRC3_IN_BufferSize[5] =
  { 2 * 17 * 16, 192 * 2, 2 * 17 * 16, 192 * 2, 192 * 2 };
WORD gwSRC3_OUT_BufferSize[5] =
  { 160 * 4, 160 * 2, 160 * 4, 160 * 2, 160 * 2 };

WORD gwSRC3_REC_SIZE_OUT[5] = { 34 * 2, 4, 12, 6, 2 };
WORD gwSRC3_REC_SIZE_IN[5] = { 37 * 2, 6, 26, 26, 12 };
WORD gwSRC3_REC_FILTER_LENGTH[5] = { 680, 60, 312, 156, 60 };
WORD gwSRC3_REC_FILTER_TAP_SIZE[5] = { 19, 29, 51, 51, 59 };	/* SIZE - 1 */

/* WORD gwSRC3_REC_BIG_LOOP_COUNTER[5]     = {      9,       0,      4,      1,     -1    }; */
WORD gwSRC3_REC_SMALL_LOOP_COUNTER[5] = { 0, 0, 1, 3, 4 };
WORD gwSRC3_REC_IN_BufferSize[5] = { 37 * 16 * 2, 32 * 6 * 2, 416, 416, 384 };
WORD gwSRC3_REC_OUT_BufferSize[5] =
  { 160 * 4, 160 * 2, 160 * 2, 160 * 2, 160 * 2 };

#define PB_DMA_BUFFER_SIZE      0x2400
#define PLAYBACK_BUFFER_SIZE    PB_DMA_BUFFER_SIZE
/*#define WAVE_BUFFER_BLOCK_SIZE  (MAXNUM_WAVE_RENDER * PLAYBACK_BUFFER_SIZE) + 0x8000 */

#define STREAM_TYPE_DSPSRC   1	/*  high quality SRC stream */
#define STREAM_TYPE_DSPMSRC  2	/*  general mini SRC stream */
#define STREAM_TYPE_NULL     (( ULONG ) -1 )

BYTE AllocateSRCPlay = FALSE;
BYTE AllocateSRCRec = FALSE;
/* -------------------------------------------------------------------------- */

/* khs, for full-dupulex */
#define MAX_SRC_STREAMS  2


PHWI gphwi = NULL;

#define _Debug_Printf_Service(x) dprintf3((x))

#if 0
extern PHYSICAL_ADDRESS PPhysicalAddress;
extern PHYSICAL_ADDRESS RPhysicalAddress;
extern ULONG SWTAddress;
#endif

#define wSPDIFVarLen (CDATA_HEADER_LEN + 10)
#define gwSPDIF_IN_BufferSize 192
/* -------------------------------------------------------------------------- */

#define MAXNUM_WAVE_RENDER    1
#define MAXNUM_WAVE_CAPTURE   1
#define MAXNUM_WAVE_MIXER     1
#define MAXNUM_WAVE_CHANNELS  MAXNUM_WAVE_RENDER + MAXNUM_WAVE_CAPTURE + MAXNUM_WAVE_MIXER


ALLEGRO_WAVE WaveStreams[MAXNUM_WAVE_CHANNELS] = { {0} };

PALLEGRO_WAVE CaptureStream = NULL;
PALLEGRO_WAVE PlaybackStream = NULL;
PALLEGRO_WAVE MidiStream = NULL;

MBUFFER CaptureBuffer;
MBUFFER RenderBuffer;
MBUFFER MidiBuffer;

static int StreamsRunning = 0;

void
InitStream ()
{
  int i;
  for (i = 0; i < 3; i++)
    {
      WaveStreams[i].PositionBeforePaused = 0;
      WaveStreams[i].DspClientInstance = NULL;
      WaveStreams[i].StreamType = STREAM_TYPE_NULL;
      WaveStreams[i].State = KSSTATE_STOP;
    }

  WaveStreams[0].WaveType = WAVE_PLAYBACK;
  WaveStreams[1].WaveType = WAVE_CAPTURE;
  WaveStreams[2].WaveType = WAVE_MIXER;

#if 0
  RenderBuffer.Physical = (ULONG) PPhysicalAddress.LowPart;
  RenderBuffer.Length = 0x2000;
  WaveStreams[0].Buffer = &RenderBuffer;
  WaveStreams[0].Length = RenderBuffer.Length;

  CaptureBuffer.Physical = (ULONG) RPhysicalAddress.LowPart;
  CaptureBuffer.Length = 0x4000;
  WaveStreams[1].Buffer = &CaptureBuffer;
  WaveStreams[1].Length = CaptureBuffer.Length;

  MidiBuffer.Physical = (ULONG) SWTAddress;
  MidiBuffer.Length = 3520;
  WaveStreams[2].Buffer = &MidiBuffer;
  WaveStreams[2].Length = MidiBuffer.Length;
  WaveStreams[2].StreamType = STREAM_TYPE_DSPMSRC;
#endif

  PlaybackStream = &WaveStreams[0];
  CaptureStream = &WaveStreams[1];
  MidiStream = &WaveStreams[2];
}

/*ARGSUSED*/
void
AllocateStream (allegro_devc * devc, IN BYTE waveType)
{
  dprintf1 (("AllocateStream %d", waveType));
  WaveStreams[waveType].DspClientInstance = NULL;
  WaveStreams[waveType].StreamType = STREAM_TYPE_NULL;
  WaveStreams[waveType].State = KSSTATE_STOP;

  /* */
  /* at the request of record, turn off SPDIF out if already on. */
  /* at the end of record, turn on */
#ifdef later
  if ((WAVE_CAPTURE == waveType) && fSPDIFOUT)
    HWMGR_Enable_SPDIF (devc, FALSE);
#endif

/*    SetFormat( &WaveStreams[ waveType ], WaveInfo ); */
}				/* AllocateStream */

void
FreeStream (allegro_devc * devc, IN PALLEGRO_WAVE AllegroWave)
{
  /* close DSP instance */
  dprintf3 (("FreeStream:%x", AllegroWave->DspClientInstance));
  if (AllegroWave->DspClientInstance)
    {
      if (kCloseInstance (devc, gphwi, AllegroWave->DspClientInstance,
			  KOPENCLOSE_SYNCHRONOUS) != KRETURN_SUCCESS)
	{
			/*_Debug_Printf_Service( "Close Inst!\n" ); */
	  dprintf1 (("ERROR: FreeStream kCloseInstance"));
	  num_errors++;
	}
      AllegroWave->DspClientInstance = NULL;
    }

  if (IsAC3Format)
    {
      /*  */
      /* reset the spdif out data type to PCM */
      HWMGR_WriteDataWord (devc, ASSP_MEMORY_PORT, DSP_DATA_MEMORY);
      HWMGR_WriteDataWord (devc, ASSP_INDEX_PORT, KDATA_SPDIF_FRAME0);
      HWMGR_WriteDataWord (devc, ASSP_DATA_PORT, 0x100);
    }

  if (STREAM_TYPE_DSPSRC == AllegroWave->StreamType)
    {
      AllocateSRCRec = FALSE;
      AllocateSRCPlay = FALSE;
    }
}				/* FreeStream */


void
SRCMGR_Stop (allegro_devc * devc, IN PALLEGRO_WAVE AllegroWave)
{
  dprintf1 (("SRCMGR_Stop: Client = %x", AllegroWave->DspClientInstance));
  if (AllegroWave->DspClientInstance)
    {

      /* stop DMA transfer */
      kStopTransfer (devc, gphwi, AllegroWave->DspClientInstance);

      /* stop the DSP client */
      kSetInstanceNotReady (devc, gphwi, AllegroWave->DspClientInstance);
    }
}				/* SRCMGR_Stop */

void
SRCMGR_StartSPDIFIN (allegro_devc * devc, IN BOOLEAN start)
{
  ULONG FilterLength;
  ULONG i;
  USHORT wCC_offset;
  USHORT wCD_offset;

  if (start)
    {
      if (pClient_SPDIFIN)
	{
	  kCloseInstance (devc, gphwi, pClient_SPDIFIN,
			  KOPENCLOSE_SYNCHRONOUS);
	  pClient_SPDIFIN = NULL;
#if 0
	  StreamsRunning--;
	  if (0 == StreamsRunning)
	    {
	      WRITE_PORT_USHORT (devc->osdev, devc->base + HOST_INT_CTRL,
				 (USHORT) (READ_PORT_USHORT
					   (devc->osdev,
					    devc->base +
					    HOST_INT_CTRL) &
					   ~CLKRUN_GEN_ENABLE));
	    }
#endif
	}
      kOpenInstance (devc, gphwi,
		     CLIENT_MINISRC,
		     0,
		     2 * (MINISRC_TMP_BUFFER_SIZE / 2 +
			  MINISRC_IN_BUFFER_SIZE / 2 + 1 +
			  MINISRC_OUT_BUFFER_SIZE / 2 + 1), &pClient_SPDIFIN);
      if (pClient_SPDIFIN)
	{
#if 0
	  StreamsRunning++;
	  if (1 == StreamsRunning)
	    {
	      WRITE_PORT_USHORT (devc->osdev, devc->base + HOST_INT_CTRL,
				 (USHORT) (READ_PORT_USHORT
					   (devc->osdev,
					    devc->base +
					    HOST_INT_CTRL) |
					   CLKRUN_GEN_ENABLE));
	    }
#endif
	  wCC_offset = (WORD) (pClient_SPDIFIN->dwDspCodeClientArea);
	  wCD_offset = (WORD) (pClient_SPDIFIN->dwDspDataClientArea);

	  dprintf1 (("success %x %x %x", pClient_SPDIFIN, wCC_offset,
		     wCD_offset));
	  kStartTransfer (devc, gphwi, pClient_SPDIFIN, TRUE, 0L, 0L, 0L, 0L,
			  (ULONG) (wCD_offset + MINISRC_TMP_BUFFER_SIZE / 2),
			  (ULONG) (MINISRC_IN_BUFFER_SIZE),
			  (ULONG) (wCD_offset + MINISRC_TMP_BUFFER_SIZE / 2 +
				   MINISRC_IN_BUFFER_SIZE / 2 + 1),
			  (ULONG) (MINISRC_OUT_BUFFER_SIZE), KCONNECT_SPDIFIN,
			  KCONNECT_MIXER);

	  /* ------------------------------------------------------------------------- */
	  /* load the coefficient starting at offset of program size in program memory */
	  /* ------------------------------------------------------------------------- */

	  /* filter length calculation */
	  FilterLength = sizeof (MINISRC_LPF) / 2;

	  for (i = 0; i < FilterLength; i++)
	    {
	      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_CODE, wCC_offset + MINISRC_COEF_LOC + i, (USHORT) (MINISRC_LPF[i]));	/*since it is in sequence. */
	    }

	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_CODE,
			 wCC_offset + MINISRC_COEF_LOC + FilterLength,
			 0X8000);

/*------------------------------------------------------------------------------------------- */
/* Transfer all parameters into DSP Data memory */
/* All the change below should be consistent with DSP client       --- khs 04/17/98 */
/*------------------------------------------------------------------------------------------- */

	  /* left Volume */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + CDATA_LEFT_VOLUME, 0X7FFF);

	  /* right Volume */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + CDATA_LEFT_VOLUME + 1, 0X7FFF);

	  /* DIRECTION */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA, wCD_offset + SRC3_DIRECTION_OFFSET, 0	/* playback */
	    );

	  /* MODE */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA, wCD_offset + SRC3_DIRECTION_OFFSET + 1, 0	/* stereo */
	    );

	  /* WORDLEN */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA, wCD_offset + SRC3_DIRECTION_OFFSET + 2, 0	/* 16bit */
	    );

	  /* FRACTION */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 3, 0X0000);

	  /* FIRST_L */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 4, 0);

	  /* FIRST_R */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 5, 0);

	  /* SECOND_L */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 6, 0);

	  /* SECOND_R */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 7, 0);

	  /* DELTA_L */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 8, 0);

	  /* DELTA_R */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 9, 0);

	  /* ROUND */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 10, 0X8000);

	  /* HIGHER BYTE MASK */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 11, 0XFF00);

	  /* TEMPORARY INPUT BUFFER ADDRESS */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 12,
			 (WORD) (wCD_offset + 40 + 8));

	  /* TEMP0 */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 13, 0);

	  /* C_FRACTION */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 14, 0);

	  /* COUNTER */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 15, 0);

	  /* NUMIN */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 16, 8);

	  /* NUMOUT */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 17, 24);

	  /* NUMSTAGE */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 18,
			 (MINISRC_BIQUAD_STAGE - 1));

	  /* COEF */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 19,
			 (WORD) (wCC_offset + MINISRC_COEF_LOC));

	  /* FILTERTAP */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 20, 0);

	  /* BOOSTER */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 21, 0);

	  /* SKIPLPF */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 22, 0xFF);


	  /* set the run-flag to start */
	  kSetInstanceReady (devc, gphwi, pClient_SPDIFIN);
	}
    }
  else
    {
      if (pClient_SPDIFIN)
	{

	  /* stop DMA transfer */
	  kStopTransfer (devc, gphwi, pClient_SPDIFIN);

	  /* stop the DSP client */
	  kSetInstanceNotReady (devc, gphwi, pClient_SPDIFIN);

	  kCloseInstance (devc, gphwi, pClient_SPDIFIN,
			  KOPENCLOSE_SYNCHRONOUS);
	  pClient_SPDIFIN = NULL;
#if 0
	  StreamsRunning--;
	  if (0 == StreamsRunning)
	    {
	      WRITE_PORT_USHORT (devc->osdev, devc->base + HOST_INT_CTRL,
				 (USHORT) (READ_PORT_USHORT
					   (devc->osdev,
					    devc->base +
					    HOST_INT_CTRL) &
					   ~CLKRUN_GEN_ENABLE));
	    }
#endif
	}
    }
}				/* SRCMGR_StartSPDIFIN */

#define REAL_SHIFT  15
#define REAL_SCALE  ( 1 << REAL_SHIFT )

void
SRCMGR_SetFrequency (allegro_devc * devc, IN PALLEGRO_WAVE AllegroWave)
{
  LONG frequency;

  if (AllegroWave->DspClientInstance)
    {
      frequency = (AllegroWave->Frequency * REAL_SCALE + 24000) / 48000;
      if (frequency > 0)
	--frequency;
      kSetFrequency (devc, gphwi, AllegroWave->DspClientInstance,
		     (WORD) frequency);
    }
}				/* SRCMGR_SetFrequency */

void
SRCMGR_StartPlay (allegro_devc * devc, IN PALLEGRO_WAVE AllegroWave)
{
  ULONG FilterLength;
  ULONG i;
  ULONG Length;
  ULONG SizeIn;
  ULONG SizeOut;
  ULONG StartOffset;
  USHORT wCC_offset;
  USHORT wCD_offset;

  if (AllegroWave->DspClientInstance)
    {

      if (!IsAC3Format)
	SRCMGR_SetFrequency (devc, AllegroWave);

      wCC_offset =
	(WORD) (AllegroWave->DspClientInstance->dwDspCodeClientArea);
      wCD_offset =
	(WORD) (AllegroWave->DspClientInstance->dwDspDataClientArea);

      Length = devc->dma_dac.dmasize;	/*AllegroWave->Buffer->Length; */
      AllegroWave->Length = Length;

      if (!IsAC3Format)
	{
	  StartOffset = 0x20 * 2;
	  SizeIn = MINISRC_IN_BUFFER_SIZE - StartOffset;
	  SizeOut = MINISRC_OUT_BUFFER_SIZE + StartOffset;
	  StartOffset = wCD_offset + MINISRC_TMP_BUFFER_SIZE / 2;

	  kStartTransfer (devc, gphwi, AllegroWave->DspClientInstance, TRUE, devc->dma_dac.base,	/*AllegroWave->Buffer->Physical, */
			  Length,
			  0L,
			  0L,
			  StartOffset,
			  SizeIn,
			  StartOffset + SizeIn / 2 + 1,
			  SizeOut, KCONNECT_DMA, KCONNECT_MIXER);

	  /* ------------------------------------------------------------------------- */
	  /* load the coefficient starting at offset of program size in program memory */
	  /* ------------------------------------------------------------------------- */

	  /* filter length calculation */
	  FilterLength = sizeof (MINISRC_LPF) / 2;

	  for (i = 0; i < FilterLength; i++)
	    {
	      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_CODE, wCC_offset + MINISRC_COEF_LOC + i, (USHORT) (MINISRC_LPF[i]));	/*since it is in sequence. */
	    }

	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_CODE,
			 wCC_offset + MINISRC_COEF_LOC + FilterLength,
			 0X8000);

/*------------------------------------------------------------------------------------------- */
/* Transfer all parameters into DSP Data memory */
/* All the change below should be consistent with DSP client       --- khs 04/17/98 */
/*------------------------------------------------------------------------------------------- */

	  /* left Volume */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + CDATA_LEFT_VOLUME, 0X7FFF);

	  /* right Volume */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + CDATA_LEFT_VOLUME + 1, 0X7FFF);

	  /* DIRECTION */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA, wCD_offset + SRC3_DIRECTION_OFFSET, 0	/* playback */
	    );

	  /* MODE */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 1,
			 (WORD) ! AllegroWave->FormatStereo);

	  /* WORDLEN */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 2,
			 (WORD) ! AllegroWave->Format16Bit);

	  /* FRACTION */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 3, 0X0000);

	  /* FIRST_L */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 4, 0);

	  /* FIRST_R */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 5, 0);

	  /* SECOND_L */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 6, 0);

	  /* SECOND_R */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 7, 0);

	  /* DELTA_L */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 8, 0);

	  /* DELTA_R */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 9, 0);

	  /* ROUND */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 10, 0X8000);

	  /* HIGHER BYTE MASK */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 11, 0XFF00);

	  /* TEMPORARY INPUT BUFFER ADDRESS */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 12,
			 (WORD) (wCD_offset + 40 + 8));

	  /* TEMP0 */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 13, 0);

	  /* C_FRACTION */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 14, 0);

	  /* COUNTER */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 15, 0);

	  /* NUMIN */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 16, 8);

	  if (0 < AllegroWave->Frequency && AllegroWave->Frequency < 8000)
	    {
	      Length = 48000 * 8 / AllegroWave->Frequency;

	      /* add one more for remainder */
	      ++Length;

	      /* make it even */
	      if (Length & 1)
		++Length;

	      /* NUMOUT */
	      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			     wCD_offset + SRC3_DIRECTION_OFFSET + 17,
			     (USHORT) Length * 2);
	    }
	  else
	    /* NUMOUT */
	    kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			   wCD_offset + SRC3_DIRECTION_OFFSET + 17, 50 * 2);

	  /* NUMSTAGE */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 18,
			 (MINISRC_BIQUAD_STAGE - 1));

	  /* COEF */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 19,
			 (WORD) (wCC_offset + MINISRC_COEF_LOC));

	  /* FILTERTAP */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 20, 0);

	  /* BOOSTER */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 21, 0);

	  /* SKIPLPF */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 22,
			 (WORD) ((AllegroWave->Frequency >
				  45000) ? 0xFF : 0));


	  if (AllegroWave->PositionBeforePaused)
	    kAlterTransfer (devc, gphwi,
			    AllegroWave->DspClientInstance,
			    KALTER_POSITION,
			    TRUE, AllegroWave->PositionBeforePaused);
	}
      else
	{
	  kStartTransfer (devc, gphwi, AllegroWave->DspClientInstance, TRUE, devc->dma_dac.base,	/*AllegroWave->Buffer->Physical, */
			  Length,
			  0L,
			  0L,
			  (ULONG) (wCD_offset + wSPDIFVarLen),
			  (ULONG) (gwSPDIF_IN_BufferSize),
			  (DWORD) NULL, 0, KCONNECT_DMA, 0);

	  /* only one parameter */
	  /* when you close ac3-spdif, you have to put it back with "0x100" */
	  HWMGR_WriteDataWord (devc, ASSP_MEMORY_PORT, DSP_DATA_MEMORY);
	  HWMGR_WriteDataWord (devc, ASSP_INDEX_PORT, KDATA_SPDIF_FRAME0);
	  HWMGR_WriteDataWord (devc, ASSP_DATA_PORT, 0x102);	/*;0x102 -- AC3 data */
	}
      /*;0x100 -- pcm data */
      /* set the run-flag to start */
      kSetInstanceReady (devc, gphwi, AllegroWave->DspClientInstance);
    }
}				/* SRCMGR_StartPlay */


void
SRCMGR_StartRecord (allegro_devc * devc, IN PALLEGRO_WAVE AllegroWave)
{
  ULONG Length;
  ULONG SizeIn;
  ULONG SizeOut;
  ULONG StartOffset;
  USHORT wCD_offset;

  if (AllegroWave->DspClientInstance)
    {

      SRCMGR_SetFrequency (devc, AllegroWave);

      wCD_offset =
	(WORD) (AllegroWave->DspClientInstance->dwDspDataClientArea);

      Length = devc->dma_adc.dmasize;	/*AllegroWave->Buffer->Length; */
      AllegroWave->Length = Length;
      StartOffset = 0x10 * 2;
      SizeIn = MINISRC_IN_BUFFER_SIZE + StartOffset;
      SizeOut = MINISRC_OUT_BUFFER_SIZE - StartOffset;
      StartOffset = wCD_offset + MINISRC_TMP_BUFFER_SIZE / 2;

      kStartTransfer (devc, gphwi, AllegroWave->DspClientInstance, TRUE, 0L, 0L, devc->dma_adc.base,	/*AllegroWave->Buffer->Physical, */
		      Length,
		      StartOffset,
		      SizeIn,
		      StartOffset + SizeIn / 2 + 1,
		      SizeOut, gwDSPConnectIn, KCONNECT_DMA);

/*------------------------------------------------------------------------------------------- */
/* Transfer all parameters into DSP Data memory */
/* All the change below should be consistent with DSP client       --- khs 04/17/98 */
/*------------------------------------------------------------------------------------------- */

      /* left Volume */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + CDATA_LEFT_VOLUME, 0X7FFF);

      /* right Volume */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + CDATA_LEFT_VOLUME + 1, 0X7FFF);

      /* DIRECTION */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA, wCD_offset + SRC3_DIRECTION_OFFSET, 1	/* recording */
	);

      /* MODE */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 1,
		     (WORD) ! AllegroWave->FormatStereo);

      /* WORDLEN */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 2,
		     (WORD) ! AllegroWave->Format16Bit);

      /* FRACTION */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 3, 0X0000);

      /* FIRST_L */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 4, 0);

      /* FIRST_R */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 5, 0);

      /* SECOND_L */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 6, 0);

      /* SECOND_R */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 7, 0);

      /* DELTA_L */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 8, 0);

      /* DELTA_R */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 9, 0);

      /* ROUND */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 10, 0X8000);

      /* HIGHER BYTE MASK */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 11, 0XFF00);

      /* TEMPORARY INPUT BUFFER ADDRESS */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 12,
		     (WORD) (wCD_offset + 40 + 8));

      /* TEMP0 */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 13, 0);

      /* C_FRACTION */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 14, 0);

      /* COUNTER */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 15, 0);

      if (0 < AllegroWave->Frequency && AllegroWave->Frequency < 8000)
	{
	  Length = 48000 * 8 / AllegroWave->Frequency;

	  /* add one more for remainder */
	  ++Length;

	  /* make it even */
	  if (Length & 1)
	    ++Length;

	  /* NUMIN */
	  kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
			 wCD_offset + SRC3_DIRECTION_OFFSET + 16,
			 (USHORT) Length);
	}
      else
	/* NUMIN */
	kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		       wCD_offset + SRC3_DIRECTION_OFFSET + 16, 50);

      /* NUMOUT */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 17, 8);

      /*AY */
      /* NUMSTAGE */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 18, 0);

      /* COEF */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 19, 0);

      /* FILTERTAP */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 20, 0);

      /* BOOSTER */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 21, 0);

      /* SKIPLPF = FF ; no LPF for any recording */
      kDspWriteWord (devc, devc->base, MEMTYPE_INTERNAL_DATA,
		     wCD_offset + SRC3_DIRECTION_OFFSET + 22, 0xFF);

      /* set the run-flag to start */
      kSetInstanceReady (devc, gphwi, AllegroWave->DspClientInstance);
    }
}				/* SRCMGR_StartRec */
void
SRCMGR_SetVolume (allegro_devc * devc, IN PCLIENT_INST client, IN USHORT left,
		  IN USHORT right)
{
  USHORT lvalue;
  USHORT rvalue;

  if (client)
    {
      lvalue = left >> 1;
      rvalue = right >> 1;

      kSetVolume (devc, gphwi, client, lvalue, rvalue, 0);
    }
}				/* SRCMGR_SetVolume */

void
SRCMGR_SetRearVolume (allegro_devc * devc, IN PCLIENT_INST client,
		      IN USHORT left, IN USHORT right)
{
  USHORT lvalue;
  USHORT rvalue;

  if (client)
    {
      lvalue = left >> 1;
      rvalue = right >> 1;

      kSetRearVolume (devc, gphwi, client, lvalue, rvalue);
    }
}				/* SRCMGR_SetVolume */

void
SRCMGR_SetupStream (allegro_devc * devc, IN PALLEGRO_WAVE AllegroWave)
{

#ifdef SRC4
  /* using host src */
  AllegroWave->SR_8khzFlag = FALSE;

#else
  /* not using host src */
  AllegroWave->SR_8khzFlag = TRUE;
#endif
  if (STREAM_TYPE_NULL == AllegroWave->StreamType)
    {
      AllegroWave->StreamType = STREAM_TYPE_DSPSRC;

      /* 8-bit not supported by SRC */
      if (!AllegroWave->Format16Bit)
	AllegroWave->StreamType = STREAM_TYPE_DSPMSRC;
    }

#if ALLEGRO_DEBUG
  if (AllegroWave->StreamType == STREAM_TYPE_DSPMSRC)
    {
      _Debug_Printf_Service ("M. SRC\r\n");
    }
  else
    {
      _Debug_Printf_Service ("SRC 3.X\r\n");
    }				/* endif */
#endif
  /*  see if we can acquire a DSP client instance for the new */
  /*  requested format */
  if (AllegroWave->StreamType == STREAM_TYPE_DSPSRC)
    {
      switch (AllegroWave->Frequency)
	{
	case 44100:
	  AllegroWave->wFreqIndex = SRC3_SR_44100;
	  break;

	case 32000:
	  AllegroWave->wFreqIndex = SRC3_SR_32000;
	  break;
#if 1
	case 22050:
	  AllegroWave->wFreqIndex = SRC3_SR_22050;
	  break;
	case 11025:
	  AllegroWave->wFreqIndex = SRC3_SR_11025;
	  break;
	case 8000:
	  AllegroWave->wFreqIndex = SRC3_SR_8000;

	  /* khs */
	  AllegroWave->SR_8khzFlag = TRUE;
	  break;
#endif
	default:
	  AllegroWave->StreamType = STREAM_TYPE_DSPMSRC;
	}
    }

#if 1
  AllegroWave->StreamType = STREAM_TYPE_DSPMSRC;	/*DON */
  dprintf1 (("SRCMGR_SetupStream Mode %d", AllegroWave->StreamType));
#endif
#if ALLEGRO_DEBUG
  if (AllegroWave->StreamType == STREAM_TYPE_DSPMSRC)
    {
      _Debug_Printf_Service ("M. SRC\r\n");
    }
  else
    {
      _Debug_Printf_Service ("SRC 3.X\r\n");
    }				/* endif */
#endif

  /* ok, if it was determined we are to attempt to do a SCR stream */
  /* we still don't know if we can so we have to see if the SRC */
  /* manager will allow us to do it... and if so we have to try and */
  /* get an instance from the DSP kernel, if the SRC manager didn't */
  /* let us get it or the DSP kernel didn't allow it we defer it */
  /* back to Mini SRC stream */
  if (AllegroWave->StreamType == STREAM_TYPE_DSPSRC)
    {
      if (WAVE_CAPTURE == AllegroWave->WaveType)
	{
	  if (!AllocateSRCRec)
	    {

	      /* Temporary Buffer Size: Two Filter Tap buffer, one block of temporary output buffer */
	      AllegroWave->wSRC3VarLen = SRC3_TEMP_OUTBUF_ADDR_OFFSET + 2
		+ 2 * (gwSRC3_REC_FILTER_TAP_SIZE[AllegroWave->wFreqIndex] +
		       1) + 2 + gwSRC3_REC_SIZE_IN[AllegroWave->wFreqIndex] +
		1 + gwSRC3_REC_SIZE_OUT[AllegroWave->wFreqIndex] + 1 + 1 +
		FOR_FUTURE_USE;

	      kOpenInstance (devc, gphwi,
			     CLIENT_SRC,
			     0,
			     2 * (AllegroWave->wSRC3VarLen +
				  gwSRC3_REC_IN_BufferSize[AllegroWave->
							   wFreqIndex] / 2 +
				  1 +
				  gwSRC3_REC_OUT_BufferSize[AllegroWave->
							    wFreqIndex] / 2 +
				  1), &AllegroWave->DspClientInstance);
	      if (AllegroWave->DspClientInstance)
		{

/* SRC does not support full-duplex */
#if 1
		  AllocateSRCPlay = TRUE;
#endif
		  AllocateSRCRec = TRUE;
		}
	    }
	}
      else
	{
	  if (!AllocateSRCPlay)
	    {

	      /* khs 082098 */
	      /* Temporary Buffer Size: Two Filter Tap buffer, one block of temporary input/output buffer */
	      AllegroWave->wSRC3VarLen = SRC3_TEMP_OUTBUF_ADDR_OFFSET + 2
		+ 2 * (gwSRC3_PB_FILTER_TAP_SIZE[AllegroWave->wFreqIndex] + 1)
		+ 2
		+ gwSRC3_PB_SIZE_OUT[AllegroWave->wFreqIndex] + 1
		+ gwSRC3_PB_SIZE_IN[AllegroWave->wFreqIndex] + 1
		+ FOR_FUTURE_USE;

	      kOpenInstance (devc, gphwi, CLIENT_SRC, 0, 2 * (AllegroWave->wSRC3VarLen + gwSRC3_IN_BufferSize[AllegroWave->wFreqIndex] / 2 + 1 + gwSRC3_OUT_BufferSize[AllegroWave->wFreqIndex] / 2 + 1),	/* word */
			     &AllegroWave->DspClientInstance);
	      dprintf3 (("kOpenIns:%x", AllegroWave->DspClientInstance));
	      if (AllegroWave->DspClientInstance)
		{
		  AllocateSRCPlay = TRUE;

/* SRC does not support full-duplex */
#if 1
		  AllocateSRCRec = TRUE;
#endif
		}
	    }
	}
      if (!AllegroWave->DspClientInstance)
	AllegroWave->StreamType = STREAM_TYPE_DSPMSRC;
    }

  if (STREAM_TYPE_DSPMSRC == AllegroWave->StreamType)
    {
      if (kOpenInstance (devc, gphwi,
			 IsAC3Format ? CLIENT_SPDIF : CLIENT_MINISRC,
			 0,
			 IsAC3Format ? 2 * (wSPDIFVarLen +
					    gwSPDIF_IN_BufferSize / 2) : 2 *
			 (MINISRC_TMP_BUFFER_SIZE / 2 +
			  MINISRC_IN_BUFFER_SIZE / 2 + 1 +
			  MINISRC_OUT_BUFFER_SIZE / 2 + 1),
			 &AllegroWave->DspClientInstance) != KRETURN_SUCCESS)
	{
	  dprintf1 (("%x:Open M SRC Inst!\n"));
	  num_errors++;
	}
    }
}				/* SRCMGR_SetupStream */

ULONG
SRCMGR_GetPosition (allegro_devc * devc, IN PALLEGRO_WAVE AllegroWave)
{
  int i;
  ULONG Position;
  ULONG BufferPosition = 0;
  KIRQL OldIrql;

/*dprintf1(("GetPos=%x %x %x %x",  AllegroWave->State, AllegroWave->DspClientInstance, AllegroWave->Length, AllegroWave->WaveType)); */

  /*  to make sure nothing funny happens if this stream is in stop */
  /*  state, just return 0 */
  if (KSSTATE_STOP == AllegroWave->State || !AllegroWave->DspClientInstance)
    return 0;

  KeAcquireSpinLock (&gpGDI->ControlSpinLock, &OldIrql);

  if (WAVE_CAPTURE == AllegroWave->WaveType)
    {

#ifndef DMAREC
      /* khs 090298 for PIO recording */
      kPIOInterruptHandler (devc, gphwi, AllegroWave->DspClientInstance);
#endif
      for (i = 0; i < 100; i++)
	{
	  if (kQueryPosition (devc, gphwi,
			      AllegroWave->DspClientInstance,
			      FALSE, &BufferPosition) == KRETURN_SUCCESS)
	    break;
	}

#if 0
      if (BufferPosition >= AllegroWave->Length)
	{
	  dprintf1 (("!%d ", BufferPosition));
	  BufferPosition = 0;
	}
#endif
      Position = BufferPosition % AllegroWave->Length;
    }
  else
    {
      for (i = 0; i < 100; i++)
	{
	  if (kQueryPosition (devc, gphwi,
			      AllegroWave->DspClientInstance,
			      TRUE, &BufferPosition) == KRETURN_SUCCESS)
	    break;
	}

#if 0
      if (BufferPosition >= AllegroWave->Length)
	{
	  dprintf1 (("!%d ", BufferPosition));
	  BufferPosition = 0;
	}
#endif
      Position = BufferPosition % AllegroWave->Length;
    }

  KeReleaseSpinLock (&gpGDI->ControlSpinLock, OldIrql);

  return (Position);
}				/* GetPosition */


void
SetFormat (allegro_devc * devc, IN PALLEGRO_WAVE AllegroWave,
	   IN PWAVE_INFO WaveInfo)
{

  /* close previous client if it is SRC and sampling rate is different */
  if (AllegroWave->DspClientInstance &&
      STREAM_TYPE_DSPSRC == AllegroWave->StreamType &&
      WaveInfo->SamplesPerSec != AllegroWave->Frequency)
    {
      dprintf1 (("SetFormat CloseInstance:%x",
		 AllegroWave->DspClientInstance));
      if (kCloseInstance
	  (devc, gphwi, AllegroWave->DspClientInstance,
	   KOPENCLOSE_SYNCHRONOUS) != KRETURN_SUCCESS)
	{
	   _Debug_Printf_Service ("Close inst!\n");
	   num_errors++;
	}

#if 0
      if (WAVE_CAPTURE == AllegroWave->WaveType)
	AllocateSRCRec = FALSE;
      else
	AllocateSRCPlay = FALSE;
#else
      AllocateSRCPlay = FALSE;
      AllocateSRCRec = FALSE;
#endif
      AllegroWave->DspClientInstance = NULL;
      AllegroWave->StreamType = STREAM_TYPE_NULL;
    }
  AllegroWave->FormatStereo = (WaveInfo->Channels == 2);
  AllegroWave->Format16Bit = (WaveInfo->BitsPerSample == 16);
  AllegroWave->Frequency = WaveInfo->SamplesPerSec;

  dprintf1 (("SetFormat:%d,%d,%d", WaveInfo->SamplesPerSec,
	     WaveInfo->BitsPerSample, WaveInfo->Channels));
}				/* SetFormat */


void
SetState (allegro_devc * devc,
	  IN PALLEGRO_WAVE AllegroWave, IN KSSTATE NewState)
{
  KIRQL OldIrql;

  dprintf1 (("SetState=(%x, %x)", AllegroWave->State, NewState));
  KeAcquireSpinLock (&gpGDI->ControlSpinLock, &OldIrql);
  if (AllegroWave->State != NewState)
    {
      switch (NewState)
	{
	case KSSTATE_STOP:


	  AllegroWave->PositionBeforePaused = 0;
	  if (KSSTATE_RUN == AllegroWave->State)
	    {
	      StreamsRunning--;

	      if (!StreamsRunning)
		{
		  kSetTimer (devc, gphwi, 0);
		  WRITE_PORT_USHORT (devc->osdev, (devc->base + 0x18),
				     READ_PORT_USHORT (devc->osdev,
						       (devc->base + 0x18)) &
				     ~CLKRUN_GEN_ENABLE);
		}
	      SRCMGR_Stop (devc, AllegroWave);
	    }
	  break;

	case KSSTATE_RUN:


	  /* client not opened */
	  if (!AllegroWave->DspClientInstance)
	    {
	      SRCMGR_SetupStream (devc, AllegroWave);
	    }

	  /*  start the transfer on whatevr method specified */
	  switch (AllegroWave->StreamType)
	    {
	    case STREAM_TYPE_DSPMSRC:
	      if (WAVE_CAPTURE == AllegroWave->WaveType)
		SRCMGR_StartRecord (devc, AllegroWave);
	      else
		SRCMGR_StartPlay (devc, AllegroWave);
	      break;
	    }

	  StreamsRunning++;

	  if (1 == StreamsRunning)
	    {
	      /* 4 ms * 12000 / 1000 */
	      kSetTimer (devc, gphwi, 240);
	      WRITE_PORT_USHORT (devc->osdev, (devc->base + 0x18),
				 READ_PORT_USHORT (devc->osdev,
						   (devc->base + 0x18)) |
				 CLKRUN_GEN_ENABLE);
	    }

	  break;
	}
      AllegroWave->State = NewState;
    }
  KeReleaseSpinLock (&gpGDI->ControlSpinLock, OldIrql);
}				/* SetState */

#ifdef later
ULONG
GetMidiPosition ()
{
  return SRCMGR_GetPosition (devc, MidiStream);
}

PCLIENT_INST pClient;
#define DBG_TRACE dprintf1

DWORD
InitModem (allegro_devc * devc)
{
  int cnt = 0;
  int wRC, retry = 0;

  do
    {
      wRC = kOpenInstance (devc, gphwi, CLIENT_MODEM, 0, 512 * 2 + 64, &pClient);	/* HSP_Open */
    }
  while ((wRC != KRETURN_SUCCESS) && (cnt++ < 100));	/* enddo */

  if (wRC != KRETURN_SUCCESS)
    {
      DBG_TRACE (("KOpenInstance from HSP modem fail"));
      return FALSE;
    }				/* endif */
  DBG_TRACE (("KOpenInstance from HSP modem OK"));

  dprintf1 (("pClient Area=%x", pClient->dwDspDataClientArea));
  /* setup the outbuffer address and length so Kernel can */
  /* move the output data to MIXER */

  retry = 0;
  do
    {

      wRC = kStartTransfer (devc, gphwi,
			    pClient,
			    NULL,
			    NULL,
			    0,
			    NULL,
			    0,
			    NULL,
			    0,
			    pClient->dwDspDataClientArea + 32,
			    128, KCONNECT_NONE, KCONNECT_MIXER);

#if ALLEGRO_DEBUG
      if (wRC != KRETURN_SUCCESS)
	{
	  DBG_TRACE (("kStartTransfer fail"));
	}			/* endif */
#endif
    }
  while ((wRC != KRETURN_SUCCESS) && (retry++ < 100));	/* enddo */


  if (wRC != KRETURN_SUCCESS)
    {
      DBG_TRACE (("kStartTransfer fail"));
      kCloseInstance (gphwi, pClient, 0);
#pragma message("----Closing client, can we handle this?")
      return FALSE;
    }
  else
    {
      DBG_TRACE (("kStartTransfer OK"));
    }				/* endif */
  return TRUE;

}

VOID
CloseModem ()
{

  int cnt = 0;
  int wRC;

  cnt = 0;
  if (!pClient)
    return;

  do
    {
      wRC = kStopTransfer (devc, gphwi, pClient);
#if ALLEGRO_DEBUG
      if (wRC != KRETURN_SUCCESS)
	{
	  DBG_TRACE (("DSPStopXfer fail"));
	}			/* endif */
#endif
    }
  while ((wRC != KRETURN_SUCCESS) && (cnt++ < 100));

  cnt = 0;
  do
    {
      wRC = kCloseInstance (devc, gphwi, pClient, 0);
    }
  while ((wRC != KRETURN_SUCCESS) && (cnt++ < 100));	/* enddo */

}
#endif

void
SRCMGR_SetPassThruVolume (allegro_devc * devc, IN PPASSTHRU client,
			  IN USHORT left, IN USHORT right)
{
  USHORT lvalue;
  USHORT rvalue;

  if (client)
    {
      lvalue = left >> 1;
      rvalue = right >> 1;

      kSetPassThruVolume (devc, gphwi, client, lvalue, rvalue);
      dprintf3 (("kSetPassThruVolume %x %x", lvalue, rvalue));
    }
}				/* SRCMGR_SetPassThruVolume */

void
SRCMGR_SetPassThruRearVolume (allegro_devc * devc, IN PPASSTHRU client,
			      IN USHORT left, IN USHORT right)
{
  USHORT lvalue;
  USHORT rvalue;

  if (!client)
    return;

  /*LeftPassThruVolRear = left; */
  /*RightPassThruVolRear = right; */


  if (client)
    {
      lvalue = left >> 1;
      rvalue = right >> 1;

      kSetPassThruRearVolume (devc, gphwi, client, lvalue, rvalue);
    }
}				/* SRCMGR_SetPassThruRearVolume */
