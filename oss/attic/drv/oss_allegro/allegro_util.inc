/*
 *      allegro_util.inc  --  ESS Technology allegro audio driver.
 *
 *      Copyright (C) 1992-2000  Don Kim (don.kim@esstech.com)
 *
 */

#define  DOCKF_DOCKED            0x00000001	/* Default Un-Docked */
#define  DOCKF_DEST_CODEC2       0x00000008	/* Tells VxD CODEC2 active */
#define  HW_INIT_SET_ACTIVE_ACRW    0x00000003	/* Sets the active AC Read/Write */
#define  HW_INIT_SET_ACTIVE_ACINTF  0x00000004	/* ... AC Intf (Local or Remote) */

VOID CodecReset (void);
void Set2Use36Mhz (void);
void HWMGR_EnableRemoteCodec (allegro_devc * devc, BOOLEAN enable);

#ifdef later
VOID CloseModem ();
#endif
extern PHWI gphwi;
extern oss_device_t *allegro_osdev;

#define CODEC_TYPE_AC97   1
#define CODEC_TYPE_ESS    2

#define DSP33MHZ 0
#define DSP36MHZ 1
#define DSP49MHZ 2

static unsigned char bChipType;
static unsigned char fRemoteCodec;
static unsigned char bCodec;
static unsigned char gbHwVolEnabled = TRUE;
static unsigned char bEapdSupportMode;
static unsigned int wEapdGPOPolarity_Port;
static unsigned char bEnable4Speaker;
BOOL IsAC3Format = FALSE;
BOOL g4Speaker = FALSE;

/*
 * DOCK
 */
static WORD gwDockVal;
UCHAR bEnableDockDetect = TRUE;
UCHAR bDockingDetectGPIPort = 0;	/* TODO: What is the right value */
static WORD wDockedMask;
static WORD wDockedValue;
UCHAR bMonoOutputSelect = 0;
static WORD gwDefaultMonoOutVol;
int bHwVolCtrlMode = -1;
static ULONG ulCodec;
#define NEC_VENDOR_ID3  0x80F11033
void HwRelease (allegro_devc * devc);
#define WriteLego HWMGR_WriteCodecData
PCLIENT_INST pClient_SPDIFIN = NULL;
BOOL fSPDIFOUT = FALSE;


#if 0
void
dDbgOut (char *sz, ...)
{
  char buf[256];
  va_list va;
  va_start (va, sz);
  vsprintf (buf, sz, va);
  va_end (va);
  printk (buf);
  printk ("\n");
}
#endif

void
HwSetSystemParameter (allegro_devc * devc, DWORD dwValue)
{
  switch (dwValue >> 16)
    {
    case HW_INIT_SET_ACTIVE_ACRW:	/*3 */
      {
	dprintf1 (("ACRW %x", dwValue & DOCKF_DEST_CODEC2));

	/*
	 * Set the active AC out direction
	 */
	if ((WORD) dwValue & DOCKF_DEST_CODEC2)	/* In/Out AC2 */
	  HWMGR_EnableRemoteCodec (devc, TRUE);
	else
	  HWMGR_EnableRemoteCodec (devc, FALSE);
      }
      break;

    case HW_INIT_SET_ACTIVE_ACINTF:	/*4 */
      {
	WORD wTmp;

	dprintf1 (("ACINTF %x", dwValue));
	if (!(dwValue & DOCKF_DOCKED))	/* Nothing there */
	  {
	    /* CODEC DAC I/F set to local only */
	    wTmp = inpw (devc->osdev, devc->base + 0x3a);
	    wTmp &= 0xFFF3;	/* [3:2] = "00" */
	    outpw (devc->osdev, devc->base + 0x3a, wTmp);

	    /* CODEC ADC I/F set to local only */
	    wTmp = inpw (devc->osdev, devc->base + 0x3c);
	    wTmp &= 0xFFF3;	/* [3:2] = "00" */
	    outpw (devc->osdev, devc->base + 0x3c, wTmp);
	  }
	else
	  {			/* DOCKED */
	    /* CODEC DAC I/F set to local + remote */
	    wTmp = inpw (devc->osdev, devc->base + 0x3a);
	    wTmp &= 0xFFF3;
	    wTmp |= 0x000C;	/* [3:2] = "11" */
	    outpw (devc->osdev, devc->base + 0x3a, wTmp);

	    /* CODEC ADC I/F set to remote AC2 (or both??) */
	    wTmp = inpw (devc->osdev, devc->base + 0x3c);
	    wTmp &= 0xFFF3;
	    wTmp |= 0x000C;	/* [3:2] = "11" */
	    outpw (devc->osdev, devc->base + 0x3c, wTmp);
	  }
      }
      break;
    }
}

VOID
PCIWrite (allegro_devc * devc, WORD address, ULONG value)
{

  pci_write_config_dword (devc->osdev, address, value);

}

ULONG
PCIRead (allegro_devc * devc, WORD address)
{
  unsigned int dw;
  pci_read_config_dword (devc->osdev, address, &dw);
  return dw;
}

#ifdef DEBUGINTERFACE
void
dumpreg (WORD programid, WORD index, WORD value, WORD * out)
{
  ULONG dwData;

  switch (programid)
    {
    case 1:			/* aggr */
      *out = inpw (devc->osdev, devc->base + index);
      break;

    case 2:			/* aggw */
      outpw (devc->osdev, devc->base + index, value);
      break;

    case 3:			/* legor */
      HWMGR_ReadCodecData (devc, devc->osdev, index, out);
      break;

    case 4:			/* legow */
      HWMGR_WriteCodecData (devc, (UCHAR) index, value);
      break;

    case 5:			/* pciar */
      index &= ~0x1;		/* make sure it's even address */
      dwData = PCIRead (devcv, index & ~0x3);
      if ((index % 4) == 0)
	*out = (WORD) dwData;
      else
	*out = (WORD) (dwData >> 16);
      break;

    case 6:			/* pciaw */
      index &= ~0x1;		/* make sure it's even address */
      dwData = PCIRead (devc, index & ~0x3);
      if ((index % 4) == 0)
	{
	  dwData &= ~0xffff;
	  dwData |= value;
	}
      else
	{
	  dwData &= 0xffff;
	  dwData |= (value << 16);
	}
      PCIWrite (devc, (index & ~0x3), dwData);
      break;
    }
}
#endif /* DEBUGINTERFACE */

void
DelayMillisec (int millisec)
{
  int count;

  millisec = millisec * 1000 / 50;
  for (count = 0; count < millisec; count++)
    KeStallExecutionProcessor (50);
}				/* DelayMillisec */

/* -------------------------------------------------------------------------- */

UCHAR
HWMGR_ReadDataByte (allegro_devc * devc, UCHAR Index)
{
  return READ_PORT_UCHAR (devc->osdev, devc->base + Index);
}				/* HWMGR_ReadDataByte */


USHORT
HWMGR_ReadDataWord (allegro_devc * devc, UCHAR Index)
{
  return READ_PORT_USHORT (devc->osdev, devc->base + Index);
}				/* HWMGR_ReadDataWord */


void
HWMGR_WriteDataByte (allegro_devc * devc, UCHAR Index, UCHAR Value)
{
  WRITE_PORT_UCHAR (devc->osdev, devc->base + Index, Value);
}				/* HWMGR_WriteDataByte */


void
HWMGR_WriteDataWord (allegro_devc * devc, UCHAR Index, USHORT Value)
{
  WRITE_PORT_USHORT (devc->osdev, devc->base + Index, Value);
}				/* HWMGR_WriteDataWord */


void
HWMGR_RestoreACLink (allegro_devc * devc)
{
  int i;
  USHORT wDataCodec;
  USHORT wDataIn;
  USHORT wDataOut;
  USHORT wControl = HWMGR_ReadDataWord (devc, RING_BUS_CTRL_A);

  HWMGR_WriteDataWord (devc, RING_BUS_CTRL_A, (USHORT) (wControl &
							~SERIAL_AC_LINK_ENABLE));
  KeStallExecutionProcessor (50);
  HWMGR_WriteDataWord (devc, RING_BUS_CTRL_A, wControl);

  wDataCodec = HWMGR_ReadDataWord (devc, RING_BUS_CTRL_B);
  wDataIn = HWMGR_ReadDataWord (devc, SDO_IN_DEST_CTRL);
  wDataOut = HWMGR_ReadDataWord (devc, SDO_OUT_DEST_CTRL);

  HWMGR_WriteDataWord (devc, SDO_OUT_DEST_CTRL, (USHORT) (wDataOut &
							  ~COMMAND_ADDR_OUT));
  HWMGR_WriteDataWord (devc, SDO_IN_DEST_CTRL, (USHORT) (wDataIn &
							 ~STATUS_ADDR_IN));
  HWMGR_WriteDataWord (devc, RING_BUS_CTRL_B, (USHORT) (wDataCodec &
							~SECOND_CODEC_ID_MASK));

  /* power down DAC to resynchronize after AC-link change */
  HWMGR_WriteDataWord (devc, CODEC_DATA, AC97_PR1);
  HWMGR_WriteDataByte (devc, CODEC_COMMAND, AC97_POWER_DOWN_CTRL);
  for (i = 0; i < 1000; i++)
    if (!(HWMGR_ReadDataByte (devc, CODEC_STATUS) & CODEC_BUSY_B))
      break;
  KeStallExecutionProcessor (1);
  HWMGR_WriteDataWord (devc, CODEC_DATA, 0);
  HWMGR_WriteDataByte (devc, CODEC_COMMAND, AC97_POWER_DOWN_CTRL);
  for (i = 0; i < 1000; i++)
    if (!(HWMGR_ReadDataByte (devc, CODEC_STATUS) & CODEC_BUSY_B))
      break;
  HWMGR_WriteDataWord (devc, RING_BUS_CTRL_B, wDataCodec);
  HWMGR_WriteDataWord (devc, SDO_IN_DEST_CTRL, wDataIn);
  HWMGR_WriteDataWord (devc, SDO_OUT_DEST_CTRL, wDataOut);
}				/* HWMGR_RestoreACLink */

BOOLEAN
HWMGR_ReadCodecData (allegro_devc * devc, UCHAR Index, PUSHORT Value)
{
  int i;

  *Value = 0;
  HWMGR_WriteDataByte (devc, CODEC_COMMAND, (UCHAR) (CODEC_READ_B | Index));
  for (i = 0; i < 1000; i++)
    {
      if (!(HWMGR_ReadDataByte (devc, CODEC_STATUS) & CODEC_BUSY_B))
	{
	  *Value = HWMGR_ReadDataWord (devc, CODEC_DATA);
	  return TRUE;
	}
    }
  if (CODEC_TYPE_ESS == bCodec)
    HWMGR_RestoreACLink (devc);

  return FALSE;
}				/* HWMGR_ReadCodecData */


BOOLEAN
HWMGR_WriteCodecData (allegro_devc * devc, UCHAR Index, USHORT Value)
{
  int i;

  HWMGR_WriteDataWord (devc, CODEC_DATA, Value);
  HWMGR_WriteDataByte (devc, CODEC_COMMAND, Index);
  for (i = 0; i < 1000; i++)
    {
      if (!(HWMGR_ReadDataByte (devc, CODEC_STATUS) & CODEC_BUSY_B))
	{
	  return TRUE;
	}
    }
  if (CODEC_TYPE_ESS == bCodec)
    HWMGR_RestoreACLink (devc);
  return FALSE;
}				/* HWMGR_WriteCodecData */

void
HWMGR_EnableExternalAmp (allegro_devc * devc, BOOLEAN enable)
{
  USHORT wDirection;
  USHORT wGPO;
  USHORT wGPO2;
  USHORT wPolarity;
  USHORT wPolarity2;

  /*
   * by default, setup for reference board
   */
  if (bEapdSupportMode == 0)
    {
      bEapdSupportMode = 1;
      if (bChipType >= M3_1998)
	wEapdGPOPolarity_Port = 0x1100;
      else
	{
	  wEapdGPOPolarity_Port = 0x1800;
	  if (bChipType == 2)
	    wEapdGPOPolarity_Port = 0x1c00;
	  if (bEnable4Speaker)
	    wEapdGPOPolarity_Port = 0x1600;
	}
    }

  dprintf3 (("Mode=%x PPort=%x", bEapdSupportMode, wEapdGPOPolarity_Port));
  wGPO2 = wPolarity2 = 0;
  switch (bEapdSupportMode)
    {
    case 0:
      break;
    case 2:
      wGPO2 = wEapdGPOPolarity_Port & 0x0F;
      wPolarity2 = wEapdGPOPolarity_Port >> 4 & 0x0F;
      if (enable)
	wPolarity2 = !wPolarity2;
      wPolarity2 = wPolarity2 << wGPO2;
      wGPO2 = 1 << wGPO2;
      wGPO = wEapdGPOPolarity_Port >> 8 & 0x0F;
      wPolarity = wEapdGPOPolarity_Port >> 12;
      if (enable)
	wPolarity = !wPolarity;
      wPolarity = wPolarity << wGPO;
      wGPO = 1 << wGPO;
      wGPO |= wGPO2;
      wPolarity |= wPolarity2;
      HWMGR_WriteDataWord (devc, GPIO_MASK, (USHORT) ~ wGPO);
      wDirection = HWMGR_ReadDataWord (devc, GPIO_DIRECTION) | wGPO;
      HWMGR_WriteDataWord (devc, GPIO_DIRECTION, wDirection);
      HWMGR_WriteDataWord (devc, GPIO_DATA, (USHORT) (GPO_SECONDARY_AC97 |
						      GPO_PRIMARY_AC97 |
						      wPolarity));
      HWMGR_WriteDataWord (devc, GPIO_MASK, 0xFFFF);
      break;

    case 1:
      wGPO = wEapdGPOPolarity_Port >> 8 & 0x0F;
      wPolarity = wEapdGPOPolarity_Port >> 12;
      if (enable)
	wPolarity = !wPolarity;
      wPolarity = wPolarity << wGPO;
      wGPO = 1 << wGPO;
      wGPO |= wGPO2;
      wPolarity |= wPolarity2;
      HWMGR_WriteDataWord (devc, GPIO_MASK, (USHORT) ~ wGPO);
      wDirection = HWMGR_ReadDataWord (devc, GPIO_DIRECTION) | wGPO;
      HWMGR_WriteDataWord (devc, GPIO_DIRECTION, wDirection);
      HWMGR_WriteDataWord (devc, GPIO_DATA, (USHORT) (GPO_SECONDARY_AC97 |
						      GPO_PRIMARY_AC97 |
						      wPolarity));
      HWMGR_WriteDataWord (devc, GPIO_MASK, 0xFFFF);
      break;
    }
}				/* HWMGR_EnableExternalAmp */


void
HWMGR_EnableRemoteCodec (allegro_devc * devc, BOOLEAN enable)
{
  USHORT wData;

/* This function MUST be in a non-paged segment */
/*    PAGED_CODE(); */
  if (enable == fRemoteCodec)
    return;
  fRemoteCodec = enable;
  if (enable)
    {

      /* enable remote codec */
      wData = HWMGR_ReadDataWord (devc, RING_BUS_CTRL_B);
      wData = (wData & ~SECOND_CODEC_ID_MASK) | 1;
      HWMGR_WriteDataWord (devc, RING_BUS_CTRL_B, wData);
      wData = HWMGR_ReadDataWord (devc, SDO_OUT_DEST_CTRL);
      wData = (wData & ~COMMAND_ADDR_OUT) | 1;
      HWMGR_WriteDataWord (devc, SDO_OUT_DEST_CTRL, wData);
      wData = HWMGR_ReadDataWord (devc, SDO_IN_DEST_CTRL);
      wData = (wData & ~STATUS_ADDR_IN) | 1;
      HWMGR_WriteDataWord (devc, SDO_IN_DEST_CTRL, wData);
    }
  else
    {

      /* disable remote codec */
      wData = HWMGR_ReadDataWord (devc, RING_BUS_CTRL_B);
      wData = wData & ~SECOND_CODEC_ID_MASK;
      HWMGR_WriteDataWord (devc, RING_BUS_CTRL_B, wData);
      wData = HWMGR_ReadDataWord (devc, SDO_OUT_DEST_CTRL);
      wData = wData & ~COMMAND_ADDR_OUT;
      HWMGR_WriteDataWord (devc, SDO_OUT_DEST_CTRL, wData);
      wData = HWMGR_ReadDataWord (devc, SDO_IN_DEST_CTRL);
      wData = wData & ~STATUS_ADDR_IN;
      HWMGR_WriteDataWord (devc, SDO_IN_DEST_CTRL, wData);
    }
}				/* HWMGR_EnableRemoteCodec */

#define AKM_CODEC       0x414B4D
#define TRA_CODEC       0x545241
#define ESS_CODEC       0x458383
#define STAC9721_CODEC  0x838476
#define STAC9721_REV_C  0x09
#define STAC9721_REV_D  0x44

BOOLEAN
HWMGR_ReadVendorId (allegro_devc * devc, OUT PULONG pulData, OUT PBYTE pbRev)
{
  USHORT wData;

  *pulData = 0;
  if (HWMGR_ReadCodecData (devc, AC97_VENDOR_ID1, &wData))
    {
      *pulData = (ULONG) wData << 16;
      if (HWMGR_ReadCodecData (devc, AC97_VENDOR_ID2, &wData))
	{
	  *pulData |= wData;
	  *pbRev = (UCHAR) * pulData;
	  *pulData >>= 8;
	  return TRUE;
	}
      *pulData = 0;
    }
  return FALSE;
}				/* HWMGR_ReadVendorId */

void
HWMGR_ResetCodec (allegro_devc * devc)
{
  ULONG ulData;
  USHORT wData;
  USHORT wDirection;
  int delay_count;
  int reset_count;
  int wait_count;
  int DoReset = TRUE;
  BYTE bSaveCodec;
  BYTE bRev;

  delay_count = reset_count = wait_count = 0;
  wDirection = HWMGR_ReadDataWord (devc, GPIO_DIRECTION);

  /* not sure if this applies for Allegro-1/Maestro-3 */
  if (PCIRead (devc, PCI_USER_CONFIG) & EXT_PCI_MASTER_ENABLE)
    {

      /* GPIO4 = output */
      wDirection |= 0x10;
    }

  HWMGR_EnableRemoteCodec (devc, FALSE);

  /* set bCodec to undefined so that ReadCodecData will not try to restore */
  /* AC-link */
  bSaveCodec = bCodec;
  bCodec = 0;

  /* If we can read codec, skip codec reset to avoid pop sound. */
  /* HP Omnibook M3 does not setup AC-link correctly */
  HWMGR_WriteDataWord (devc, RING_BUS_CTRL_A, IO_SRAM_ENABLE);

  /* delay at least 20 us if disabling AC-link */
  KeStallExecutionProcessor (20);
  HWMGR_WriteDataWord (devc, RING_BUS_CTRL_A,
		       IO_SRAM_ENABLE | SERIAL_AC_LINK_ENABLE);
  KeStallExecutionProcessor (1);
  if (HWMGR_ReadVendorId (devc, &ulData, &bRev))
    {

      dprintf1 (("codec1 id = %x", ulData));
      /* do not need to reset */
      DoReset = FALSE;
    }
  if (ulData != 0 && ulData != 0xFFFFFF)
    {
      ulCodec = ulData;
      bCodec = CODEC_TYPE_AC97;
      if (TRA_CODEC == ulCodec)
	{

	  /* fix for tritech 2.0a codec */
	  HWMGR_WriteCodecData (devc, 0x2A, 0x0001);
	  HWMGR_WriteCodecData (devc, 0x2C, 0x0000);
	  HWMGR_WriteCodecData (devc, 0x2C, 0xFFFF);
	}
      else if (ESS_CODEC == ulCodec)
	{
	  bCodec = CODEC_TYPE_ESS;
	}
    }
  else
    {
      if (bChipType >= M3_1998)
	bCodec = CODEC_TYPE_AC97;
      else
	bCodec = CODEC_TYPE_ESS;
    }
  if (bSaveCodec)
    {
      bCodec = bSaveCodec;
    }

  dprintf1 (("reset codec1 bCodec=%d DoReset=%d", bCodec, DoReset));
  /* reset primary codec */
  if (DoReset)
    {

      /* regular AC97 codec */
      if (bChipType >= M3_1998)
	{
	  delay_count = 20;
#if CRYSTAL_CODEC_FIX
	  if (!ulCodec || CRY_CODEC == ulCodec)
	    {
	      wait_count = 500;
	    }
	  else
	    {
	      wait_count = 20;
	    }
#else
	  wait_count = 20;
#endif
	}
      else
	{
	  delay_count = 50;

	  /* delay 800 ms!? */
	  wait_count = 800;
	}
      reset_count = 0;

    RESET_CODEC:

      HWMGR_WriteDataWord (devc, RING_BUS_CTRL_A, IO_SRAM_ENABLE);

      /* delay at least 20 us if disabling AC-link */
      KeStallExecutionProcessor (20);

      /* write GPO0 */
      HWMGR_WriteDataWord (devc, GPIO_DIRECTION,
			   wDirection & ~GPO_PRIMARY_AC97);
      HWMGR_WriteDataWord (devc, GPIO_MASK, (USHORT) ~ GPO_PRIMARY_AC97);
      HWMGR_WriteDataWord (devc, GPIO_DATA, 0x000);
      wDirection |= GPO_PRIMARY_AC97;
      HWMGR_WriteDataWord (devc, GPIO_DIRECTION, wDirection);

      /* delay 20 ms */
      DelayMillisec (delay_count);
      HWMGR_WriteDataWord (devc, GPIO_DATA, GPO_PRIMARY_AC97);
      KeStallExecutionProcessor (5);
      HWMGR_WriteDataWord (devc, RING_BUS_CTRL_A,
			   IO_SRAM_ENABLE | SERIAL_AC_LINK_ENABLE);
      HWMGR_WriteDataWord (devc, GPIO_MASK, 0xFFFF);

      /* wait for codec to be ready */
      DelayMillisec (wait_count);

#if defined( AC97_RESETFIX )
      /* If unable to read from Codec, perform special reset */
      if (!HWMGR_ReadCodecData (devc, AC97_VENDOR_ID1, &wData))
	{
	  /* Save Serial Link Configuration */
	  HWMGR_ReadDataWord (devc, RING_BUS_CTRL_A, &wRingBusReg);

	  /* Disable Serial AC Link and Cold Reset Codec */
	  HWMGR_WriteDataWord (devc, RING_BUS_CTRL_A, _AC_SDFS_ENABLE);

	  /* Delay 20mS */
	  for (count = 0; count < 400; count++)
	    KeStallExecutionProcessor (50);

	  /* Enable AC Serial Link */
	  HWMGR_WriteDataWord (devc, RING_BUG_CTRL_A, wRingBusReg);

	  if (!HWMGR_ReadCodecData (devc, AC97_VENDOR_ID1, &wData))
	    {
	      /* Disable Serial AC Link and Cold Reset Codec */
	      HWMGR_WriteDataWord (devc, RING_BUS_CTRL_A, _AC_SDFS_ENABLE);

	      /* Warm Reset Codec */
	      HWMGR_WriteDataByte (devc, CODEC_COMMAND, 0x00);
	      /* Delay 20mS */
	      for (count = 0; count < 400; count++)
		KeStallExecutionProcessor (50);

	      /* Enable AC Serial Link */
	      HWMGR_WriteDataWord (devc, RING_BUG_CTRL_A, wRingBusReg);

	      /* Set AC97 PR4 26h[12] */
	      HWMGR_ReadCodecData (devc, AC97_POWER_DOWN_CTRL, &wData);
	      HWMGR_WriteCodecData (devc, AC97_POWER_DOWN_CTRL,
				    wData | AC97_PR4);

	      /* Warm Reset Codec */
	      HWMGR_WriteDataByte (devc, CODEC_COMMAND, 0x00);
	      /* Delay 20mS */
	      for (count = 0; count < 400; count++)
		KeStallExecutionProcessor (50);
	    }
	}
#endif

      if (!bCodec)
	{
	  if (HWMGR_ReadVendorId (devc, &ulData, &bRev))
	    {
	      if (ulData != 0 && ulData != 0xFFFFFF)
		{
		  ulCodec = ulData;
		  bCodec = CODEC_TYPE_AC97;
		  if (TRA_CODEC == ulCodec)
		    {

		      /* fix for tritech 2.0a codec */
		      HWMGR_WriteCodecData (devc, 0x2A, 0x0001);
		      HWMGR_WriteCodecData (devc, 0x2C, 0x0000);
		      HWMGR_WriteCodecData (devc, 0x2C, 0xFFFF);
		    }
		  else if (ESS_CODEC == ulCodec)
		    {
		      bCodec = CODEC_TYPE_ESS;
		    }
		}
	    }
	  else
	    {
	      if (++reset_count < 20)
		{
		  delay_count += 10;
		  wait_count += 100;
		  goto RESET_CODEC;
		}
	    }
	  if (!bCodec)
	    {
	      if (bChipType >= M3_1998)
		bCodec = CODEC_TYPE_AC97;
	      else
		bCodec = CODEC_TYPE_ESS;
	    }
	}
    }
  else if ((wDirection & GPO_PRIMARY_AC97) == 0)
    {
      HWMGR_WriteDataWord (devc, GPIO_MASK, (USHORT) ~ GPO_PRIMARY_AC97);
      HWMGR_WriteDataWord (devc, GPIO_DATA, GPO_PRIMARY_AC97);
      wDirection |= GPO_PRIMARY_AC97;
      HWMGR_WriteDataWord (devc, GPIO_DIRECTION, wDirection);
      HWMGR_WriteDataWord (devc, GPIO_MASK, 0xFFFF);
    }

  dprintf1 (("Codec:%d\n", bCodec));

  if (CODEC_TYPE_ESS == bCodec)
    {
      if (HWMGR_ReadCodecData (devc, AC97_CLOCK_DELAY, &wData))
	{
	  wData &= ~(AC97_CLOCK_DELAY_SEL << AC97_ADC_CDS_SHIFT);
	  wData |= 18 << AC97_ADC_CDS_SHIFT;
	  HWMGR_WriteCodecData (devc, AC97_CLOCK_DELAY, wData);
	}
      else
	{
	  if (++reset_count < 20)
	    {
	      delay_count += 10;
	      wait_count += 100;
	      goto RESET_CODEC;
	    }
	}
    }

  if (gwDockVal)
    {
      HWMGR_EnableRemoteCodec (devc, TRUE);

      wData = HWMGR_ReadDataWord (devc, RING_BUS_CTRL_B);
      wData &= ~(SECOND_AC_ENABLE | SECOND_CODEC_ID_MASK);
      HWMGR_WriteDataWord (devc, RING_BUS_CTRL_B,
			   (USHORT) (wData | SECOND_AC_ENABLE | 1));

      /* If we can read codec, skip codec reset to avoid pop sound. */
      DoReset = TRUE;
      if (HWMGR_ReadVendorId (devc, &ulData, &bRev))
	{

	  dprintf1 (("Read:%x,%x\n", ulData, bRev));

	  /* do not need to reset */
	  DoReset = FALSE;
	}

      /* reset secondary codec */
      if (DoReset)
	{
	  HWMGR_WriteDataWord (devc, RING_BUS_CTRL_A, IO_SRAM_ENABLE);

	  /* delay at least 20 us if disabling AC-link */
	  KeStallExecutionProcessor (20);

	  /* write GPO3 */
	  HWMGR_WriteDataWord (devc, GPIO_DIRECTION,
			       wDirection & ~GPO_SECONDARY_AC97);
	  HWMGR_WriteDataWord (devc, GPIO_MASK,
			       (USHORT) ~ GPO_SECONDARY_AC97);
	  HWMGR_WriteDataWord (devc, GPIO_DATA, 0x000);
	  wDirection |= GPO_SECONDARY_AC97;
	  HWMGR_WriteDataWord (devc, GPIO_DIRECTION, wDirection);

	  /* delay 20 ms */
	  DelayMillisec (20);
	  HWMGR_WriteDataWord (devc, GPIO_DATA, GPO_SECONDARY_AC97);

	  /* delay 20 ms */
	  DelayMillisec (20);
	  HWMGR_WriteDataWord (devc, GPIO_MASK, 0xFFFF);

	  HWMGR_WriteDataWord (devc, RING_BUS_CTRL_A,
			       IO_SRAM_ENABLE | SERIAL_AC_LINK_ENABLE);
	  KeStallExecutionProcessor (1);

	  /* read vendor id */
	  (void) HWMGR_ReadVendorId (devc, &ulData, &bRev);

	  dprintf1 (("Reset:%x,%x\n", ulData, bRev));
	}
      else if ((wDirection & GPO_SECONDARY_AC97) == 0)
	{
	  HWMGR_WriteDataWord (devc, GPIO_MASK,
			       (USHORT) ~ GPO_SECONDARY_AC97);
	  HWMGR_WriteDataWord (devc, GPIO_DATA, GPO_SECONDARY_AC97);
	  wDirection |= GPO_SECONDARY_AC97;
	  HWMGR_WriteDataWord (devc, GPIO_DIRECTION, wDirection);
	  HWMGR_WriteDataWord (devc, GPIO_MASK, 0xFFFF);
	}
      if (ulData != 0 && ulData != 0xFFFFFF)
	{
	  if (TRA_CODEC == ulData)
	    {

	      /* fix for tritech 2.0a codec */
	      HWMGR_WriteCodecData (devc, 0x2A, 0x0001);
	      HWMGR_WriteCodecData (devc, 0x2C, 0x0000);
	      HWMGR_WriteCodecData (devc, 0x2C, 0xFFFF);
	    }
	  else if (STAC9721_CODEC == ulData)
	    {
	      USHORT wStatus;

	      if (!HWMGR_ReadCodecData
		  (devc, AC97_POWER_DOWN_CTRL, &wStatus) || 0 == wStatus)
		{
		  HWMGR_WriteCodecData (devc, 0x76, 0xABBA);
		  if (STAC9721_REV_D == bRev)
		    {
		      HWMGR_WriteCodecData (devc, 0x78, 0x2002);
		      KeStallExecutionProcessor (20);
		      HWMGR_WriteCodecData (devc, 0x78, 0x2802);
		    }
		  else
		    {
		      HWMGR_WriteCodecData (devc, 0x78, 0x3002);
		      KeStallExecutionProcessor (20);
		      HWMGR_WriteCodecData (devc, 0x78, 0x3802);
		    }
		}
	    }
	}
      HWMGR_EnableRemoteCodec (devc, FALSE);

      HWMGR_WriteDataWord (devc, RING_BUS_CTRL_B, wData);
    }
}				/* HWMGR_ResetCodec */

#if 0
#define DSP33MHZ  0
#define DSP36MHZ  1
#define DSP49MHZ  2
#endif


UCHAR bGray_Code[32] = {
  0x00, 0x01, 0x03, 0x02, 0x07, 0x06, 0x04, 0x05,
  0x0F, 0x0E, 0x0C, 0x0D, 0x08, 0x09, 0x0B, 0x0A,
  0x1F, 0x1E, 0x1C, 0x1D, 0x18, 0x19, 0x1B, 0x1A,
  0x10, 0x11, 0x13, 0x12, 0x17, 0x16, 0x14, 0x15
};

void
HWMGR_SetClockSpeed (allegro_devc * devc, UCHAR speed)
{
  ULONG ulData;
  int choice;
  int mode;
  UCHAR bModeValue[8];
  UCHAR bData;
  UCHAR bDelta;

  HWMGR_WriteDataByte (devc, ASSP_CONTROL_B, RESET_ASSP);

  ulData = PCIRead (devc, PCI_ALLEGRO_CONFIG);
  ulData &= ~INT_CLK_SELECT;
  ulData &= ~(CLK_MULT_MODE_SELECT | CLK_MULT_MODE_SELECT_2);
  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);

  if (DSP36MHZ == speed)
    {
      ulData = PCIRead (devc, PCI_USER_CONFIG);
      ulData &= ~IN_CLK_12MHZ_SELECT;
      PCIWrite (devc, PCI_USER_CONFIG, ulData);

      for (mode = 0; mode < 4; mode++)
	{
	  ulData |= INT_CLK_MULT_RESET;
	  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
	  ulData &= ~CLK_MULT_MODE_SELECT;
	  ulData |= mode << CLK_MULT_MODE_SHIFT;
	  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
	  ulData &= ~INT_CLK_MULT_RESET;
	  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);

	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  bModeValue[mode] =
	    bGray_Code[HWMGR_ReadDataWord (devc, CLK_MULT_DATA_PORT) >> 5
		       & 0x1F];
	}
      ulData |= CLK_MULT_MODE_SELECT_2;
      for (mode = 0; mode < 4; mode++)
	{
	  ulData |= INT_CLK_MULT_RESET;
	  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
	  ulData &= ~CLK_MULT_MODE_SELECT;
	  ulData |= mode << CLK_MULT_MODE_SHIFT;
	  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
	  ulData &= ~INT_CLK_MULT_RESET;
	  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);

	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  bModeValue[mode + 4] =
	    bGray_Code[HWMGR_ReadDataWord (devc, CLK_MULT_DATA_PORT) >> 5
		       & 0x1F];
	}
#define Abs(x) ((x)<0) ? -(x) : (x)
      bDelta = Abs (bModeValue[0] - 0x10);
      choice = 0;
      for (mode = 1; mode < 8; mode++)
	{
	  if (bDelta > Abs (bModeValue[mode] - 0x10))
	    {
	      bDelta = Abs (bModeValue[mode] - 0x10);
	      choice = mode;
	    }
	}

#if 0
      _Debug_Printf_Service ("mode:%d\n", choice);
#endif
      ulData &= ~(CLK_MULT_MODE_SELECT | CLK_MULT_MODE_SELECT_2);
      if (choice > 3)
	{
	  ulData |= CLK_MULT_MODE_SELECT_2;
	  choice -= 4;
	}
      ulData |= INT_CLK_MULT_RESET;
      PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
      ulData |= choice << CLK_MULT_MODE_SHIFT;
      PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
      ulData &= ~INT_CLK_MULT_RESET;
      PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);

      ulData |= INT_CLK_SELECT;
      PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
      READ_PORT_UCHAR (devc->osdev, 0x80);
      READ_PORT_UCHAR (devc->osdev, 0x80);
    }
  else
    {
      ulData = PCIRead (devc, PCI_USER_CONFIG);
      ulData |= IN_CLK_12MHZ_SELECT;
      PCIWrite (devc, PCI_USER_CONFIG, ulData);
    }

  bData = HWMGR_ReadDataByte (devc, ASSP_CONTROL_A);
  bData &= ~(DSP_CLK_36MHZ_SELECT | ASSP_CLK_49MHZ_SELECT);
  bData |= ASSP_0_WS_ENABLE;
  switch (speed)
    {
    case DSP36MHZ:
      bData |= DSP_CLK_36MHZ_SELECT;
      break;
    case DSP49MHZ:
      bData |= ASSP_CLK_49MHZ_SELECT;
      break;
    }
  HWMGR_WriteDataByte (devc, ASSP_CONTROL_A, bData);

  HWMGR_WriteDataByte (devc, ASSP_CONTROL_B, RUN_ASSP);
}				/* HWMGR_SetClockSpeed */

void
HWMGR_SetM3ClockSpeed (allegro_devc * devc, UCHAR speed)
{
  ULONG ulData;
  int choice;
  int mode;
  UCHAR bModeValue[8];
  UCHAR bData;
  UCHAR bDelta;

  HWMGR_WriteDataByte (devc, ASSP_CONTROL_B, RESET_ASSP);

  ulData = PCIRead (devc, PCI_ALLEGRO_CONFIG);
  if (DSP33MHZ == speed)
    ulData &= ~INT_CLK_SRC_NOT_PCI;
  else
    ulData |= INT_CLK_SRC_NOT_PCI;
  ulData &= ~(INT_CLK_MULT_ENABLE | INT_CLK_SELECT);
  ulData &= ~(CLK_MULT_MODE_SELECT | CLK_MULT_MODE_SELECT_2);
  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);

  if (DSP36MHZ == speed)
    {
      ulData = PCIRead (devc, PCI_USER_CONFIG);
      ulData |= IN_CLK_12MHZ_SELECT;
      PCIWrite (devc, PCI_USER_CONFIG, ulData);

      for (mode = 0; mode < 4; mode++)
	{
	  ulData &= ~INT_CLK_MULT_ENABLE;
	  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
	  ulData &= ~CLK_MULT_MODE_SELECT;
	  ulData |= mode << CLK_MULT_MODE_SHIFT;
	  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
	  ulData |= INT_CLK_MULT_ENABLE;
	  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);

	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  bModeValue[mode] =
	    bGray_Code[HWMGR_ReadDataWord (devc, CLK_MULT_DATA_PORT) >> 5
		       & 0x1F];
	}
      ulData |= CLK_MULT_MODE_SELECT_2;
      for (mode = 0; mode < 4; mode++)
	{
	  ulData &= ~INT_CLK_MULT_ENABLE;
	  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
	  ulData &= ~CLK_MULT_MODE_SELECT;
	  ulData |= mode << CLK_MULT_MODE_SHIFT;
	  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
	  ulData |= INT_CLK_MULT_ENABLE;
	  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);

	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  READ_PORT_UCHAR (devc->osdev, 0x80);
	  bModeValue[mode + 4] =
	    bGray_Code[HWMGR_ReadDataWord (devc, CLK_MULT_DATA_PORT) >> 5
		       & 0x1F];
	}
      bDelta = Abs (bModeValue[0] - 0x10);
      choice = 0;
      for (mode = 1; mode < 8; mode++)
	{
	  if (bDelta > Abs (bModeValue[mode] - 0x10))
	    {
	      bDelta = Abs (bModeValue[mode] - 0x10);
	      choice = mode;
	    }
	}

#if 0
      _Debug_Printf_Service ("mode:%d\n", choice);
#endif
      ulData &= ~(INT_CLK_MULT_ENABLE | CLK_MULT_MODE_SELECT |
		  CLK_MULT_MODE_SELECT_2);
      if (choice > 3)
	{
	  ulData |= CLK_MULT_MODE_SELECT_2;
	  choice -= 4;
	}
      PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
      ulData |= choice << CLK_MULT_MODE_SHIFT;
      PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
      ulData |= INT_CLK_MULT_ENABLE;
      PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);
    }

  bData = HWMGR_ReadDataByte (devc, ASSP_CONTROL_A);
  bData &= ~(DSP_CLK_36MHZ_SELECT | ASSP_CLK_49MHZ_SELECT);
  bData |= ASSP_0_WS_ENABLE;
  switch (speed)
    {
    case DSP36MHZ:
      bData |= DSP_CLK_36MHZ_SELECT;
      break;
    case DSP49MHZ:
      bData |= ASSP_CLK_49MHZ_SELECT;
      break;
    }
  HWMGR_WriteDataByte (devc, ASSP_CONTROL_A, bData);

  HWMGR_WriteDataByte (devc, ASSP_CONTROL_B, RUN_ASSP);
}				/* HWMGR_SetM3ClockSpeed */


BOOLEAN
HWMGR_InitKernel (allegro_devc * devc)
{
  ULONG ulData;

  ulData = PCIRead (devc, PCI_ALLEGRO_CONFIG);
  ulData &= REDUCED_DEBOUNCE;

  if (gbHwVolEnabled)
    {
      ulData |= HV_CTRL_ENABLE;

      if (bHwVolCtrlMode == -1)
	{
	  bHwVolCtrlMode = 0;
	  if (bChipType < M3_1998)
	    bHwVolCtrlMode = 1;
	}

      if (PCIRead (devc, 0x2c) == NEC_VENDOR_ID3)
	{
	  bHwVolCtrlMode |= (0x80 | 0x40);
	}

      if (bHwVolCtrlMode & 0x80)
	{
	  bHwVolCtrlMode &= ~0x80;
	  ulData |= REDUCED_DEBOUNCE;
	}

      if (bHwVolCtrlMode & 0x40)
	{
	  bHwVolCtrlMode &= ~0x40;
	  PCIWrite (devc, PCI_USER_CONFIG,
		    PCIRead (devc, PCI_USER_CONFIG) | HV_CTRL_TEST);
	}

      dprintf1 (("bHwVolCtrlMode=%x", bHwVolCtrlMode));

      /* default 53/54 pin */
      switch (bHwVolCtrlMode)
	{

	  /* 44/45 pin */
	case 0x01:
	  ulData |= HV_BUTTON_FROM_GD;
	  break;

#if 0
	  /* M3E */
	case 0x02:
	  break;
	  /* M3E */
	case 0x03:
	  break;
	  /* M3E */
	case 0x04:
	  break;
#endif
	}
    }

  ulData |= PM_CTRL_ENABLE | CLK_DIV_BY_49 | USE_PCI_TIMING;
  PCIWrite (devc, PCI_ALLEGRO_CONFIG, ulData);

  ulData = PCIRead (devc, PCI_USER_CONFIG);
  ulData |= MIDI_1_ENABLE;
  PCIWrite (devc, PCI_USER_CONFIG, ulData);

  if (bChipType >= M3_1998)
    HWMGR_SetM3ClockSpeed (devc, DSP49MHZ);
  else
    /* new Allegro board only works with external 49 Mhz clock */
    HWMGR_SetClockSpeed (devc, DSP49MHZ);
  /*ulDSPConnectIn = KCONNECT_ADC1; */

  /* initialize the DSP kernel */
  if (kInitKernel (devc, &gphwi, 0x1978, 0x10, devc->base, 0) !=
      KRETURN_SUCCESS)
    return FALSE;

#ifdef later
  InitModem ();
#endif

#ifdef FIXED_MODEM
  {
    extern DWORD gdwESModem;
    extern DWORD DisableModemClient (VOID);
    if (gdwESModem)
      {
	if (kOpenInstance
	    (gphwi, CLIENT_MODEM, 0, 0x180 * 2,
	     &pClient_Modem) != KRETURN_SUCCESS)
	  return FALSE;
	DisableModemClient ();
      }
  }
#endif

  return TRUE;
}				/* HWMGR_InitKernel */

/* -------------------------------------------------------------------------- */

void
HWMGR_InitSystem (allegro_devc * devc)
{
  WORD wData;
  DWORD dwVal;

#if 0				/* this cause zip sound for Compaq. Code here is for problems for older */
  /* allegro chip, new one does not need this any more. */
  /* prevent AC-link deadlocking */
  HWMGR_WriteDataWord (devc, HOST_INT_CTRL, SOFTWARE_RESET_ENABLE);
  KeStallExecutionProcessor (5);
  HWMGR_WriteDataWord (devc, HOST_INT_CTRL, 0x0);
#endif

  HWMGR_InitKernel (devc);

  if (bEnableDockDetect)
    {
      wDockedMask = bDockingDetectGPIPort & 0x0F;
      wDockedValue = (bDockingDetectGPIPort >> 4 & 0x0F) << wDockedMask;
      wDockedMask = 1 << wDockedMask;
      gwDockVal =
	(HWMGR_ReadDataWord (devc, GPIO_DATA) & wDockedMask) == wDockedValue;
    }
  else
    {
      wDockedMask = wDockedValue = 0;
    }

  /* force to set correct codec */
  fRemoteCodec = 0xFF;

  HWMGR_ResetCodec (devc);

  /* allegro codec proble from cold boot so one more resetcodec */
  HWMGR_ResetCodec (devc);

  if (CODEC_TYPE_ESS == bCodec)
    {

      /* power down DAC to resynchronize after AC-link change */
      HWMGR_WriteCodecData (devc, AC97_POWER_DOWN_CTRL, AC97_PR1);
      KeStallExecutionProcessor (1);
    }

  /* codec not reset every time */
  HWMGR_WriteCodecData (devc, AC97_POWER_DOWN_CTRL, 0);
  HWMGR_EnableExternalAmp (devc, TRUE);
  if (gwDockVal)
    {
      wData = HWMGR_ReadDataWord (devc, RING_BUS_CTRL_B);
      wData |= SECOND_AC_ENABLE;
      HWMGR_WriteDataWord (devc, RING_BUS_CTRL_B, wData);
    }

  HWMGR_ReadCodecData (devc, AC97_GENERAL_PURPOSE, &wData);
  if (bMonoOutputSelect)
    {
      wData |= 0x300;

      HWMGR_WriteCodecData (devc, AC97_MASTER_MONO_VOL, gwDefaultMonoOutVol);
      KeStallExecutionProcessor (100);
      HWMGR_WriteCodecData (devc, AC97_MASTER_MONO_VOL, gwDefaultMonoOutVol);
    }
  else
    wData &= ~0x300;
  HWMGR_WriteCodecData (devc, AC97_GENERAL_PURPOSE, wData);

  /* fix DAC volume at 0x0808 */
  HWMGR_WriteCodecData (devc, AC97_MASTER_VOL, 0x0404);
  HWMGR_WriteCodecData (devc, AC97_PCM_OUT_VOL, 0x0404);

  /* Mute the ADC */
  if (!g4Speaker)
    HWMGR_WriteCodecData (devc, AC97_RECORD_GAIN, 0x8000);

  gwDSPConnectIn = KCONNECT_ADC1;

  /*
   * Undock it first
   */
  dwVal = HW_INIT_SET_ACTIVE_ACINTF;
  dwVal <<= 16;
  HwSetSystemParameter (devc, dwVal);

  /*
   * Setup active I/F
   */
  dwVal = HW_INIT_SET_ACTIVE_ACINTF;
  dwVal <<= 16;

  HwSetSystemParameter (devc, dwVal);
}				/* HWMGR_InitSystem */


void
HwRelease (allegro_devc * devc)
{
#ifdef later
  CloseModem ();
#endif

  kTermKernel (devc, gphwi, devc->base);

  HWMGR_EnableExternalAmp (devc, FALSE);

  /* MUTE THE DAMN THING FIRST */
  WriteLego (devc, 0x02, 0x3F3F);	/* LOUT1 Master Attenuation (-1.5dB steps: 0000 = 0dB, 3F3F = -94dB, ) */

  WriteLego (devc, 0x04, 0x9F1F);	/* Headphone */
  WriteLego (devc, 0x06, 0x9F1F);	/* MonoOut */
  WriteLego (devc, 0x12, 0x9F1F);	/* CD to Mixer Level (80=mute; 00=+12.0dB, 08=0.0dB, 0F = -33dB) */
  WriteLego (devc, 0x16, 0x9F1F);	/* AUX to Mixer Level (80=mute; 00=+12.0dB, 08=0.0dB, 0F = -33dB) */
  WriteLego (devc, 0x0E, 0x9F1F);	/* Mic         to Mixer Level (80=mute; 00=+12.0dB, 08=0.0dB, 0F = -33dB) */
  WriteLego (devc, 0x18, 0x9F1F);	/* DAC         to Mixer Level (80=mute; 00=+12.0dB, 08=0.0dB, 0F = -33dB) */
  WriteLego (devc, 0x10, 0x9F1F);	/* Line in     to Mixer Level (80=mute; 00=+12.0dB, 08=0.0dB, 0F = -33dB) */
  WriteLego (devc, 0x1A, 0);	/* Rec Gain */
  WriteLego (devc, 0x1C, 0x8F0F);	/* Rec Gain */
}
