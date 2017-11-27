#ifndef __JZ4780_EFUSE_H__
#define __JZ4780_EFUSE_H__

struct jz4780_efuse_platform_data {
	int gpio_vddq_en_n;	/* supply 2.5V to VDDQ */
};

void jz_efuse_id_read(int is_chip_id, uint32_t *buf);

#endif
