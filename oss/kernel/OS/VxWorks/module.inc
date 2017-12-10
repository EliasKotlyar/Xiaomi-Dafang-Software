#if DRIVER_TYPE == DRV_PCI
#include <drv/pci/pciConfigLib.h>
#undef PCI_LATENCY_TIMER
#include <oss_pci.h>

int
oss_pci_read_config_byte (oss_device_t * osdev, offset_t where,
			  unsigned char *val)
{
  oss_pci_device_t *pd = osdev->dip;

  return pciConfigInByte (pd->bus, pd->dev, pd->func, where, val);
}

int
oss_pci_read_config_irq (oss_device_t * osdev, offset_t where,
			 unsigned char *val)
{
  oss_pci_device_t *pd = osdev->dip;

  return pciConfigInByte (pd->bus, pd->dev, pd->func, where, val);
}

int
oss_pci_read_config_word (oss_device_t * osdev, offset_t where,
			  unsigned short *val)
{
  oss_pci_device_t *pd = osdev->dip;

  if (osdev == NULL)
    {
      cmn_err (CE_CONT, "oss_pci_read_config_word: osdev==NULL\n");
      return PCIBIOS_FAILED;
    }

  return pciConfigInWord (pd->bus, pd->dev, pd->func, where, val);
}

int
oss_pci_read_config_dword (oss_device_t * osdev, offset_t where,
			   unsigned int *val)
{
  oss_pci_device_t *pd = osdev->dip;

  return pciConfigInLong (pd->bus, pd->dev, pd->func, where, val);
}

int
oss_pci_write_config_byte (oss_device_t * osdev, offset_t where,
			   unsigned char val)
{
  oss_pci_device_t *pd = osdev->dip;

  return pciConfigOutByte (pd->bus, pd->dev, pd->func, where, val);
}

int
oss_pci_write_config_word (oss_device_t * osdev, offset_t where,
			   unsigned short val)
{
  oss_pci_device_t *pd = osdev->dip;

  return pciConfigOutWord (pd->bus, pd->dev, pd->func, where, val);
}

int
oss_pci_write_config_dword (oss_device_t * osdev, offset_t where,
			    unsigned int val)
{
  oss_pci_device_t *pd = osdev->dip;

  return pciConfigOutLong (pd->bus, pd->dev, pd->func, where, val);
}
#endif

int
DRIVER_NAME(void)
{
#if DRIVER_TYPE == DRV_PCI
	int i;
	int bus, dev, func;
	unsigned int d, vendor_id, dev_id;
  	static int instance = 0;

	if (id_table[0] == 0)
           {
		cmn_err (CE_WARN, DRIVER_NICK ": ID table is empty\n");
		return OSS_EIO;
           }

	i=0;

	while ((d=id_table[i]) != 0)
	{
		int index=0;
		vendor_id = (d >> 16) & 0xffff;
		dev_id = d & 0xffff;

		while (pciFindDevice(vendor_id, dev_id, instance,&bus, &dev, &func) == OK)
		{
			oss_pci_device_t *pcidev = malloc(sizeof(*pcidev));
			oss_device_t *osdev;


			cmn_err(CE_CONT, "Found pci device %08x / %d : b=%d, d=%d, f=%d\n", d, index, bus, dev, func);

			pcidev->bus = bus;
			pcidev->dev = dev;
			pcidev->func = func;

			if ((osdev =
			     osdev_create ((dev_info_t*)pcidev, DRIVER_TYPE, instance++, DRIVER_NICK,
				     NULL)) == NULL)
			   {
			     return OSS_ENOMEM;
			   }

			index++;
		}

		i++;
	}
	
#endif

	return 0;
}
