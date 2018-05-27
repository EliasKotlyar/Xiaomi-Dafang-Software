#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/mod_devicetable.h>
#include <linux/log2.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/i2c.h>
#include "codec_i2c_dev.h"
#include "es8374_codec.h"

struct i2c_client *audio_codec_client = NULL;

static struct i2c_board_info es8374_i2c_info = {
	I2C_BOARD_INFO("es8374", 0x10),
};

int codec_i2c_dev_init(void)
{
	struct i2c_adapter *adapter;
	int i2c_id = 2;

	adapter = i2c_get_adapter(i2c_id);
	if (!adapter) {
		printk("error:(%s,%d), i2c get adapter failed.\n",__func__,__LINE__);
		return -1;
	}

	audio_codec_client = i2c_new_device(adapter, &es8374_i2c_info);
	if (!audio_codec_client) {
		printk("error:(%s,%d),new probed device failed.\n",__func__,__LINE__);
		return -1;
	}
	i2c_put_adapter(adapter);

	return 0;
}

void codec_i2c_dev_exit(void)
{
	i2c_unregister_device(audio_codec_client);
	audio_codec_client = NULL;
}

void codec_i2c_drv_init(int codec_type)
{
	switch(codec_type) {
		case 1:
			es8374_i2c_drv_init();
			break;
		default:
			printk("error:(%s,%d), error external codec type.\n",__func__,__LINE__);
			break;
	}
	return;
}

void codec_i2c_drv_exit(int codec_type)
{
	switch(codec_type) {
		case 1:
			es8374_i2c_drv_exit();
			break;
		default:
			printk("error:(%s,%d), error external codec type.\n",__func__,__LINE__);
			break;
	}
	return;
}
