#if !defined(__SYSTEM_SPI_H__)
#define __SYSTEM_SPI_H__

#include <apical-isp/apical_types.h>

uint32_t spi_rw32(uint32_t sel_mask, uint32_t control, uint32_t data,uint8_t data_size);
uint32_t spi_rw48(uint32_t sel_mask, uint32_t control, uint32_t addr,uint8_t addr_size, uint32_t data,uint8_t data_size);
void spi_init_access(void);

#endif /* __SYSTEM_SPI_H__ */
