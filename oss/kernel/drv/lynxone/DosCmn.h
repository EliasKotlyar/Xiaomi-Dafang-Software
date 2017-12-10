/***************************************************************************
 DOSCMN.H

 Created: B. Bauman, November 1998

 Copyright © 1998	Lynx Studio Technology, Inc.

 Environment: DOS 16 Bit Mode (32 bit instructions must be enabled)

 Revision History

 When      Who  Description
 --------- ---  ---------------------------------------------------------------
****************************************************************************/


////////////////////////////////////////////////////////////////////////////
// Base Addresses for DOS
////////////////////////////////////////////////////////////////////////////
#define BAR0_DOS_ADDR_RJB	0xB0000	//works in Bob's computer
#define BAR2_DOS_ADDR_RJB	0xD0000
#define BAR3_DOS_ADDR_RJB	0xB0100

#define BAR0_DOS_ADDR_DAH	0xDFF00	// works in David's computer
#define BAR2_DOS_ADDR_DAH	0xE0000
#define BAR3_DOS_ADDR_DAH	0xDFE00

#define BAR0_SIZE				0x00080
#define BAR2_SIZE				0x10000
#define BAR3_SIZE				0x00010

#define CARRY_FLAG 0x01		/* 80x86 Flags Register Carry Flag bit */

// PLX register defines
#define PLX_LAS0RR				0x00

// PCI BIOS Functions
#define PCI_FUNCTION_ID			0xb1
#define PCI_BIOS_PRESENT		0x01
#define FIND_PCI_DEVICE			0x02
#define FIND_PCI_CLASS_CODE		0x03
#define GENERATE_SPECIAL_CYCLE	0x06
#define READ_CONFIG_BYTE		0x08
#define READ_CONFIG_WORD		0x09
#define READ_CONFIG_DWORD		0x0a
#define WRITE_CONFIG_BYTE		0x0b
#define WRITE_CONFIG_WORD		0x0c
#define WRITE_CONFIG_DWORD		0x0d

// PCI Configuration Space Registers
#define PCI_CS_VENDOR_ID         0x00
#define PCI_CS_DEVICE_ID         0x02
#define PCI_CS_COMMAND           0x04
#define PCI_CS_STATUS            0x06
#define PCI_CS_REVISION_ID       0x08
#define PCI_CS_CLASS_CODE        0x09
#define PCI_CS_CACHE_LINE_SIZE   0x0c
#define PCI_CS_MASTER_LATENCY    0x0d
#define PCI_CS_HEADER_TYPE       0x0e
#define PCI_CS_BIST              0x0f
#define PCI_CS_BASE_ADDRESS_0    0x10
#define PCI_CS_BASE_ADDRESS_1    0x14
#define PCI_CS_BASE_ADDRESS_2    0x18
#define PCI_CS_BASE_ADDRESS_3    0x1c
#define PCI_CS_BASE_ADDRESS_4    0x20
#define PCI_CS_BASE_ADDRESS_5    0x24
#define PCI_CS_EXPANSION_ROM     0x30
#define PCI_CS_INTERRUPT_LINE    0x3c
#define PCI_CS_INTERRUPT_PIN     0x3d
#define PCI_CS_MIN_GNT           0x3e
#define PCI_CS_MAX_LAT           0x3f

// Global addresses declared in DOSCMN.C
extern ULONG gulLomemBAR0Address;
extern ULONG gulLomemBAR2Address;
extern ULONG gulLomemBAR3Address;


typedef struct
{
  ADAPTER Adapter;
}
GLOBAL_INFO, *PGLOBAL_INFO;

extern GLOBAL_INFO GDI;


////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////
BOOLEAN IsPCIBiosPresent (VOID);
BOOLEAN ReadConfigurationArea (BYTE ucFunction, BYTE ucBusNumber,
			       BYTE ucDeviceFunction, BYTE ucRegisterNumber,
			       PULONG pulData);
BOOLEAN WriteConfigurationArea (BYTE ucFunction, BYTE ucBusNumber,
				BYTE ucDeviceFunction, BYTE ucRegisterNumber,
				ULONG ulData);
