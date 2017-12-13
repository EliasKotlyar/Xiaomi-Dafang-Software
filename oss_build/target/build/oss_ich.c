/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_ich
#define DRIVER_NICK	"oss_ich"
#define DRIVER_PURPOSE	"Intel ICH1-7, nVidia nForce, SiS7012, AMD8111/786"
#define DRIVER_STR_INFO	oss_ich_str_info
#define DRIVER_ATTACH	oss_ich_attach
#define DRIVER_DETACH	oss_ich_detach
#define DRIVER_TYPE	DRV_PCI

int intelpci_rate_tuning = 240;
/*
 * Few broken motherboards had nonstandard crystal clocks that cause wrong
 * sampling rates. With such motherboards it was necessary to use
 * the intelpci_rate_tuning option to fix the rate. See the manual
 * for more info.
 */
int intelpci_force_mmio = 0;
/*
 * intelpci_force_mmio forces the driver to use Memory Mapped IO
 * (some bioses don't provide I/O mapped addresses).
 */
int ich_jacksense = 0;
/*
 * Force enabling jacksense on some AD198x mixers.
 */
#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1022,	.device=0x7445,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1022,	.device=0x746d,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1039,	.device=0x7012,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x1b1,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x3a,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x6a,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x8a,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0xda,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0xea,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x59,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x26b,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x2415,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x2425,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x2445,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x2485,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x24c5,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x24d5,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x25a6,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x266e,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x27de,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x7195,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(intelpci_rate_tuning , int, S_IRUGO);
MODULE_PARM_DESC(intelpci_rate_tuning , 
"\n"
"Few broken motherboards had nonstandard crystal clocks that cause wrong\n"
"sampling rates. With such motherboards it was necessary to use\n"
"the intelpci_rate_tuning option to fix the rate. See the manual\n"
"for more info.\n"
"\n");
module_param(intelpci_force_mmio , int, S_IRUGO);
MODULE_PARM_DESC(intelpci_force_mmio , 
"\n"
"intelpci_force_mmio forces the driver to use Memory Mapped IO\n"
"(some bioses don't provide I/O mapped addresses).\n"
"\n");
module_param(ich_jacksense , int, S_IRUGO);
MODULE_PARM_DESC(ich_jacksense , 
"\n"
"Force enabling jacksense on some AD198x mixers.\n"
"\n");


