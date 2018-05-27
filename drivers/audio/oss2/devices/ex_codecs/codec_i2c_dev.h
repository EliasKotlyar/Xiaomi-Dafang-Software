#ifndef _CODEC_I2C_DEV_H
#define _CODEC_I2C_DEV_H

struct audio_codec_i2c_board_info {
	char type[20];
	int addr;
	int i2c_adapter_id;
};

struct audio_codec_register_info {
	char name[32];
	struct audio_codec_i2c_board_info i2c;
};

int codec_i2c_dev_init(void);
void codec_i2c_dev_exit(void);

void codec_i2c_drv_init(int codec_type);
void codec_i2c_drv_exit(int codec_type);

#endif
