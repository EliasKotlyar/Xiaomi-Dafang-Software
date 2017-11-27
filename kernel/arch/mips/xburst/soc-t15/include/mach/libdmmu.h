#ifndef _LIBDMMU_
#define _LIBDMMU_

#include <linux/device.h>

unsigned long dmmu_map(struct device *dev,unsigned long vaddr,unsigned long len);
int dmmu_unmap(struct device *dev,unsigned long vaddr, int len);
int dmmu_unmap_all(struct device *dev);
void dmmu_dump_vaddr(unsigned long vaddr);

#endif
