/*
 * Purpose: Definitions of various PCI specific constants and functions
 *
 * All drivers for PCI devices should included this file.
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
#ifndef OSS_PCI_H
#define OSS_PCI_H
#define OSS_HAVE_PCI
#define PCIBIOS_SUCCESSFUL		0x00
#define PCIBIOS_FAILED			-1

#define PCI_CLASS_MULTIMEDIA_AUDIO	0x0401
#define PCI_CLASS_MULTIMEDIA_OTHER	0x0480
#define PCI_VENDOR_ID			0x00
#define PCI_REVISION_ID			0x08
#define PCI_COMMAND			0x04
#define PCI_DEVICE_ID			0x02
#define PCI_LATENCY_TIMER		0x0d
#define PCI_INTERRUPT_LINE		0x3c
#define PCI_BASE_ADDRESS_0		0x10

#define PCI_MEM_BASE_ADDRESS_0		0x10
#define PCI_MEM_BASE_ADDRESS_1		0x14
#define PCI_MEM_BASE_ADDRESS_2		0x18
#define PCI_MEM_BASE_ADDRESS_3		0x1c
#define PCI_BASE_ADDRESS_1		0x14
#define PCI_BASE_ADDRESS_2		0x18
#define PCI_BASE_ADDRESS_3		0x1c
#define PCI_BASE_ADDRESS_4		0x20
#define PCI_BASE_ADDRESS_5		0x24
#define PCI_COMMAND_IO         		0x01
#define PCI_COMMAND_MEMORY		0x02
#define PCI_COMMAND_MASTER		0x04
#define PCI_COMMAND_PARITY		0x40
#define PCI_COMMAND_SERR		0x100

#define PCI_STATUS			0x06
#define PCI_SUBSYSTEM_VENDOR_ID		0x2c
#define PCI_SUBSYSTEM_ID		0x2e

extern int pci_read_config_byte (oss_device_t * osdev, offset_t where,
				 unsigned char *val);
extern int pci_read_config_irq (oss_device_t * osdev, offset_t where,
				unsigned char *val);
extern int pci_read_config_word (oss_device_t * osdev, offset_t where,
				 unsigned short *val);
extern int pci_read_config_dword (oss_device_t * osdev, offset_t where,
				  unsigned int *val);
extern int pci_write_config_byte (oss_device_t * osdev, offset_t where,
				  unsigned char val);
extern int pci_write_config_word (oss_device_t * osdev, offset_t where,
				  unsigned short val);
extern int pci_write_config_dword (oss_device_t * osdev, offset_t where,
				   unsigned int val);
extern int pci_enable_msi (oss_device_t * osdev);
#endif
