#ifndef __SYSTEM_I2C_H__
#define __SYSTEM_I2C_H__

#include <apical-isp/apical_types.h>

void I2C_init(void);
void I2C_close(void);
uint8_t I2C_write(uint8_t address, uint8_t* data, uint32_t size);
uint8_t I2C_read(uint8_t address, uint8_t* data, uint32_t size);

#define I2C_OK 			0
#define I2C_NOACK 		1
#define I2C_NOCONNECT 	2

#endif /* __SYSTEM_I2C_H__ */
