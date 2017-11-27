#include <asm/io.h>
#include "system_i2c.h"

void I2C_init(void)
{
	printk("^^^ %s ^^^\n",__func__);
}

void I2C_close(void)
{
	printk("^^^ %s ^^^\n",__func__);
}

uint8_t I2C_write(uint8_t address, uint8_t* data, uint32_t size)
{
	printk("%s: {add = 0x%02x, *data = 0x%02x} size = %d\n",__func__, address, *data, size);
	return 0;
}

uint8_t I2C_read(uint8_t address, uint8_t* data, uint32_t size)
{
	printk("%s: {add = 0x%02x, data = %p} size = %d\n",__func__, address, data, size);
	return 0;
}
