/*
 * Purpose: Definition of the _dev_info_t structure for Linux
 */
#ifndef OSSDIP_H
#define OSSDIP_H
struct _dev_info_t
{
  struct pci_dev *pcidev;
};
#endif
