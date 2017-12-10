/*
 *      ESS Technology allegro audio driver.
 *
 *      Copyright (C) 1992-2000  Don Kim (don.kim@esstech.com)
 */
#ifndef _HCKERNEL_H_
#define _HCKERNEL_H_




#define  IIS_APU56     0x38
#define  IIS_APU57     0x39

#define  DSP_4_CHANNEL  0x08000000	/*AY specify DSP 4 channel */
#define  DSP_6_CHANNEL  0x04000000	/*AY specify DSP 6 channel */

#define DSP_MEMORY_INDEX  0x80
#define DSP_MEMORY_TYPE   0x82
#define DSP_DATA_MEMORY   0x0003
#define DSP_CODE_MEMORY   0x0002
#define DSP_MEMORY_VALUE  0x84

#define NOT_READY       0
#define RUNNING         1
#define PAUSE           2
#define STOP            3

/*DSPAPU */
#define DSPAPU_IN_BUF_SIZE    96	/* 32 * 3 BUFFERS */
#define DSPAPU_OUT_BUF_SIZE   96	/* 32 * 3 BUFFERS */

/****************** SRC3 Begin ********************** */
/* max # of playback/record clients */

#define MAXSRCPLAYCLIENT   1	/* ASSUME NO MULTIPLE STREAMS */
#define MAXSRCRECCLIENT    1
#define MAXSVCLIENT        1	/* assume 1 for speaker virtualization */


/* value for bDirection */
#define SRCPLAYBACK        TRUE
#define SRCRECORD          FALSE

/*/dwMode */
#define SRC_PCM_16BIT           0x01
#define SRC_PCM_8BIT            0x02
#define SRC_PCM_STEREO          0x04
#define SRC_PCM_MONO            0x08


/* khs 0302 */
#define SRC3_PLAYBACK            0
#define SRC3_RECORD              1

#define SRC3_STEREO              0
#define SRC3_MONO                1

#define SRC3_16BIT               0
#define SRC3_8BIT                1

#define SRC3_SR_44100            0
#define SRC3_SR_32000            1
#define SRC3_SR_22050            2
#define SRC3_SR_11025            3
#define SRC3_SR_8000             4

#define DSP_PROGRAM_SIZE                0x15c	/* maximum program size */
#define SRC_PB_FILTER_SIZE              666	/* maximum playback filter size */

#define SRC3_DIRECTION_OFFSET           CDATA_HEADER_LEN
#define SRC3_MODE_OFFSET                CDATA_HEADER_LEN + 1
#define SRC3_WORD_LENGTH_OFFSET         CDATA_HEADER_LEN + 2
#define SRC3_PARAMETER_OFFSET           CDATA_HEADER_LEN + 3
#define SRC3_COEFF_ADDR_OFFSET          CDATA_HEADER_LEN + 8
#define SRC3_FILTAP_ADDR_OFFSET         CDATA_HEADER_LEN + 10
#define SRC3_TEMP_INBUF_ADDR_OFFSET     CDATA_HEADER_LEN + 16
#define SRC3_TEMP_OUTBUF_ADDR_OFFSET    CDATA_HEADER_LEN + 17
#define FOR_FUTURE_USE                  10	/* for storing temporary variable in future */

/****************** SRC3 End ********************** */


/****************** Speaker Virtualization Begin ********************** */

 /* specify the sample rate flag = 1 for 44.1K ; 0 for 48K */
 /* default is 0 so it is for 48K */
#define SPKRVIRT_SR_FLAG             CDATA_HEADER_LEN  + 1
#define SPKRVIRT_44K                 0X0001
#define SPKRVIRT_48K                 0X0000


/*VMAx Speaker Virtualization */
#define SPKRVIRT_VARIABLE_LEN        81	/* IN WORD */
#define SPKRVIRT_IN_BUFFER_OFFSET    CDATA_HEADER_LEN  + SPKRVIRT_VARIABLE_LEN
#define SPKRVIRT_IN_BUFFER_SIZE      (384 * 2)	/* BYTE */

#define SPKRVIRT_OUT_BUFFER_OFFSET   SPKRVIRT_IN_BUFFER_OFFSET + (SPKRVIRT_IN_BUFFER_SIZE / 2)
#define SPKRVIRT_OUT_BUFFER_SIZE     (192 * 2)	/* BYTE */

#define SPKRVIRT_OWN_DATA_SIZE       SPKRVIRT_OUT_BUFFER_OFFSET + (SPKRVIRT_OUT_BUFFER_SIZE / 2)

/*CRL Sensaura */

#define CRL_SPKRVIRT_VARIABLE_LEN        237	/* IN WORD */

#define CRL_SPKRVIRT_IN_BUFFER_OFFSET    CDATA_HEADER_LEN  + CRL_SPKRVIRT_VARIABLE_LEN
/*#define CRL_SPKRVIRT_IN_BUFFER_SIZE      (384 * 2)  // BYTE */
#define CRL_SPKRVIRT_IN_BUFFER_SIZE      (256 * 2)	/* BYTE */

#define CRL_SPKRVIRT_OUT_BUFFER_OFFSET   CRL_SPKRVIRT_IN_BUFFER_OFFSET + (CRL_SPKRVIRT_IN_BUFFER_SIZE / 2)
/*#define CRL_SPKRVIRT_OUT_BUFFER_SIZE     (192 * 2)  // BYTE */
#define CRL_SPKRVIRT_OUT_BUFFER_SIZE     (128 * 2)	/* BYTE */

#define CRL_SPKRVIRT_OWN_DATA_SIZE       CRL_SPKRVIRT_OUT_BUFFER_OFFSET + (CRL_SPKRVIRT_OUT_BUFFER_SIZE / 2)

/****************** Speaker Virtualization End ********************** */

/* WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! */
/* */
/* Do NOT change DSP_STARTTRANSFER_INFO structure without updating hckernel.inc */
/* */
/* WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! */


typedef struct sTRANSFER_INFO
{
  PCLIENT_INST pClient_Inst;
  DWORD dwAutoRepeat;
  DWORD dwHostSrcBufferAddr;
  DWORD dwHostSrcBufferLen;
  DWORD dwHostDstBufferAddr;
  DWORD dwHostDstBufferLen;
  DWORD dwDSPInBufferAddr;
  DWORD dwDSPInBufferLen;
  DWORD dwDSPOutBufferAddr;
  DWORD dwDSPOutBufferLen;
  DWORD dwDSPInConnection;
  DWORD dwDSPOutConnection;

}
TRANSFER_INFO, *PTRANSFER_INFO;


/*similar to kOpenInstance */
extern WORD __cdecl DSPxxxOpen (DWORD, DWORD, DWORD, PCLIENT_INST *);
/*similar to kCloseInstance */
extern WORD __cdecl DSPxxxClose (PCLIENT_INST, DWORD);
/*similar to kStartTransfer */
extern WORD __cdecl DSPStartXfer (PTRANSFER_INFO);
/*similar to kStopTransfer */
extern WORD __cdecl DSPStopXfer (PCLIENT_INST);
/*similar to kQueryPosition */
extern WORD __cdecl DSPQueryPosition (PCLIENT_INST, DWORD, PDWORD);
/*similar to kSetInstanceReady */
extern WORD __cdecl DSPSetInstanceReady (PCLIENT_INST);
/*similar to kSetInstanceNotReady */
extern WORD __cdecl DSPSetInstanceNotReady (PCLIENT_INST);
/*similar to kPIOInterruptHandler */
extern WORD __cdecl DSPPIOInterruptHandler (PCLIENT_INST);
extern WORD __cdecl DSPSetTimer (DWORD);
/*similar to kAlterTransfer */
extern WORD __cdecl DSPAlterTransfer (PCLIENT_INST, DWORD, DWORD, DWORD);

extern WORD __cdecl DSPUnmaskInt (void);
extern WORD __cdecl DSPMaskInt (void);

extern WORD __cdecl DSPReadWord (DWORD, DWORD, DWORD);
extern VOID __cdecl DSPWriteWord (DWORD, DWORD, DWORD, WORD);
/*similar to kI2SInterruptHandler */
extern WORD __cdecl DSPI2SInterruptHandler (PDWORD);

extern WORD __cdecl DSPOpenPassThru (PPASSTHRU *, DWORD, DWORD);
extern WORD __cdecl DSPClosePassThru (PPASSTHRU);

extern WORD gwDisable_DSP;
extern DWORD gwDisable_VMAx;
#define VMAX_MAGIC    0x564D4158
#define CRL_MAGIC     0x43524C

extern WORD gwDSPConnectIn;	/* tell DSP where to grab data from */
extern WORD gwDSPConnectOut;	/* tell DSP where to send data to */
#endif
