/*
 * sensor_info.c
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <jz_proc.h>

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <soc/gpio.h>

static unsigned i2c_adapter_nr = 0;
module_param(i2c_adapter_nr, uint, 0644);
MODULE_PARM_DESC(i2c_adapter_nr, "sensor used i2c_adapter nr");

static int reset_gpio = GPIO_PA(18);
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");


#define SENSOR_INFO_IOC_MAGIC  'S'
#define IOCTL_SINFO_GET			_IO(SENSOR_INFO_IOC_MAGIC, 100)
#define IOCTL_SINFO_FLASH		_IO(SENSOR_INFO_IOC_MAGIC, 101)

#define SENSOR_TYPE_INVALID	-1

typedef struct SENSOR_INFO_S
{
	uint8_t *name;
	uint8_t i2c_addr;
	uint8_t *clk_name;
	uint32_t clk;

	uint32_t id_value[8];
	uint32_t id_value_len;
	uint32_t id_addr[8];
	uint32_t id_addr_len;
	uint8_t id_cnt;

	struct i2c_adapter *adap;
} SENSOR_INFO_T, *SENSOR_INFO_P;

enum SENSOR_TYPE
{
	SENSOR_TYPE_OV9712=0,
	SENSOR_TYPE_OV9732,
	SENSOR_TYPE_OV9750,
	SENSOR_TYPE_JXH42,
	SENSOR_TYPE_SC1035,
	SENSOR_TYPE_SC1135,
	SENSOR_TYPE_SC1045,
	SENSOR_TYPE_SC1145,
	SENSOR_TYPE_AR0130,
	SENSOR_TYPE_JXH61,
	SENSOR_TYPE_GC1024,
	SENSOR_TYPE_GC1064,
	SENSOR_TYPE_GC2023,
	SENSOR_TYPE_BF3115,
	SENSOR_TYPE_IMX225,
	SENSOR_TYPE_OV2710,
	SENSOR_TYPE_IMX322,
	SENSOR_TYPE_SC2135,
	SENSOR_TYPE_SP1409,
	SENSOR_TYPE_JXH62,
	SENSOR_TYPE_BG0806,
	SENSOR_TYPE_OV4689,
	SENSOR_TYPE_JXF22,
	SENSOR_TYPE_IMX323,
	SENSOR_TYPE_IMX291
};

SENSOR_INFO_T g_sinfo[] =
{
	{"ov9712", 0x30,  "cgu_cim", 24000000, {0x97, 0x11}, 1, {0xa, 0xb}, 1, 2, NULL},
	{"ov9732", 0x36,  "cgu_cim", 24000000, {0x97, 0x32}, 1, {0x300a, 0x300b}, 2, 2, NULL},
	{"ov9750", 0x36,  "cgu_cim", 24000000, {0x97, 0x50}, 1, {0x300b, 0x300c}, 2, 2, NULL},
	{"jxh42",  0x30,  "cgu_cim", 24000000, {0xa0, 0x42, 0x81}, 1, {0xa, 0xb, 0x9}, 1, 3, NULL},
	{"sc1035", 0x30,  "cgu_cim", 24000000, {0xf0, 0x00}, 1, {0x580b, 0x3c05}, 2, 2, NULL},
	{"sc1135", 0x30,  "cgu_cim", 24000000, {0x00, 0x35}, 1, {0x580b, 0x2148}, 2, 2, NULL},
	{"sc1045", 0x30,  "cgu_cim", 24000000, {0x10, 0x45}, 1, {0x3107, 0x3108}, 2, 2, NULL},
	{"sc1145", 0x30,  "cgu_cim", 24000000, {0x11, 0x45}, 1, {0x3107, 0x3108}, 2, 2, NULL},
	{"ar0130", 0x10,  "cgu_cim", 24000000, {0x2402}, 2, {0x3000}, 2, 1, NULL},
	{"jxh61",  0x30,  "cgu_cim", 24000000, {0xa0, 0x42, 0x3}, 1, {0xa, 0xb, 0x9}, 1, 3, NULL},
	{"gc1024", 0x3c,  "cgu_cim", 24000000, {0x10, 0x04}, 1, {0xf0, 0xf1}, 1, 2, NULL},
	{"gc1064", 0x3c,  "cgu_cim", 24000000, {0x10, 0x24}, 1, {0xf0, 0xf1}, 1, 2, NULL},
	{"gc2023", 0x37,  "cgu_cim", 24000000, {0x20, 0x23}, 1, {0xf0, 0xf1}, 1, 2, NULL},
	{"bf3115", 0x6e,  "cgu_cim", 24000000, {0x31, 0x16}, 1, {0xfc, 0xfd}, 1, 2, NULL},
	{"imx225", 0x1a,  "cgu_cim", 24000000, {0x10, 0x01}, 1, {0x3004, 0x3013}, 2, 2, NULL},
	{"ov2710", 0x36,  "cgu_cim", 24000000, {0x27, 0x10}, 1, {0x300a, 0x300b}, 2, 2, NULL},
	{"imx322", 0x1a,  "cgu_cim", 37125000, {0x55, 0xec}, 1, {0x31f3, 0x31f2}, 2, 2, NULL},
	{"sc2135", 0x30,  "cgu_cim", 24000000, {0x21, 0x35}, 1, {0x3107, 0x3108}, 2, 2, NULL},
	{"sp1409", 0x34,  "cgu_cim", 24000000, {0x14, 0x09}, 1, {0x04, 0x05}, 1, 2, NULL},
	{"jxh62",  0x30,  "cgu_cim", 24000000, {0xa0, 0x62}, 1, {0xa, 0xb}, 1, 2, NULL},
	{"bg0806", 0x32,  "cgu_cim", 24000000, {0x08, 0x06}, 1, {0x0000, 0x0001}, 2, 2, NULL},
	{"ov4689", 0x36,  "cgu_cim", 24000000, {0x46, 0x88}, 1, {0x300a, 0x300b}, 2, 2, NULL},
	{"jxf22",  0x40,  "cgu_cim", 24000000, {0x0f, 0x22}, 1, {0xa, 0xb}, 1, 2, NULL},
	{"imx323", 0x1a,  "cgu_cim", 37125000, {0x14, 0x6c}, 1, {0x31f3, 0x31f2}, 2, 2, NULL},
	{"imx291", 0x1a,  "cgu_cim", 37125000, {0xA0, 0xB2}, 1, {0x3008, 0x301e}, 2, 2, NULL},
};

static int8_t g_sensor_id = -1;
static struct mutex g_mutex;

int sensor_read(SENSOR_INFO_P sinfo, struct i2c_adapter *adap, uint32_t addr, uint32_t *value)
{
	int ret;
	uint8_t buf[4] = {0};
	uint8_t data[4] = {0};

	uint8_t rlen = sinfo->id_value_len;
	uint8_t wlen = sinfo->id_addr_len;
	struct i2c_msg msg[2] = {
		[0] = {
			.addr	= sinfo->i2c_addr,
			.flags	= 0,
			.len	= wlen,
			.buf	= buf,
		},
		[1] = {
			.addr	= sinfo->i2c_addr,
			.flags	= I2C_M_RD,
			.len	= rlen,
			.buf	= data,
		}
	};

	if (1 == wlen) {
		buf[0] = addr&0xff;
	} else if (2 == wlen){
		buf[0] = (addr>>8)&0xff;
		buf[1] = addr&0xff;
	} else if (3 == wlen){
		buf[0] = (addr>>16)&0xff;
		buf[1] = (addr>>8)&0xff;
		buf[2] = addr&0xff;
	} else if (4 == wlen){
		buf[0] = (addr>>24)&0xff;
		buf[1] = (addr>>16)&0xff;
		buf[2] = (addr>>8)&0xff;
		buf[3] = addr&0xff;
	} else {
		printk("error: %s,%d wlen = %d\n", __func__, __LINE__, wlen);
	}
	ret = i2c_transfer(adap, msg, 2);
	if (ret > 0) ret = 0;
	if (0 != ret)
		printk("error: %s,%d ret = %d\n", __func__, __LINE__, ret);
	if (1 == rlen) {
		*value = data[0];
	} else if (2 == rlen){
		*value = (data[0]<<8)|data[1];
	} else if (3 == rlen){
		*value = (data[0]<<16)|(data[1]<<8)|data[2];
	} else if (4 == rlen){
		*value = (data[0]<<24)|(data[1]<<16)|(data[2]<<8)|data[3];
	} else {
		printk("error: %s,%d rlen = %d\n", __func__, __LINE__, rlen);
	}
	printk(" sensor_read: addr=0x%x value = 0x%x\n", addr, *value);
	return ret;
}
static int32_t process_one_adapter(struct device *dev, void *data)
{
	int32_t ret;
	int32_t i = 0;
	int32_t j = 0;
	struct clk *mclk;
	struct i2c_adapter *adap;
	uint8_t scnt = sizeof(g_sinfo)/sizeof(g_sinfo[0]);
	unsigned long rate = 0;
	mutex_lock(&g_mutex);
	if (dev->type != &i2c_adapter_type) {
		mutex_unlock(&g_mutex);
		return 0;
	}

	adap = to_i2c_adapter(dev);
	printk("name : %s nr : %d\n", adap->name, adap->nr);

	if (adap->nr != i2c_adapter_nr) {
		mutex_unlock(&g_mutex);
		return 0;
	}

	for (i = 0; i < scnt; i++) {
		uint8_t idcnt = g_sinfo[i].id_cnt;
		mclk = clk_get(NULL, g_sinfo[i].clk_name);
		if (IS_ERR(mclk)) {
			printk("Cannot get sensor input clock cgu_cim\n");
			mutex_unlock(&g_mutex);
			return PTR_ERR(mclk);
		}
		clk_set_rate(mclk, g_sinfo[i].clk);
		clk_enable(mclk);
		if(reset_gpio != -1){
			ret = gpio_request(reset_gpio,"reset");
			if(!ret){
				gpio_direction_output(reset_gpio, 1);
				mdelay(20);
				gpio_direction_output(reset_gpio, 0);
				mdelay(20);
				gpio_direction_output(reset_gpio, 1);
				mdelay(20);
			}else{
				printk("gpio requrest fail %d\n",reset_gpio);
			}
		}
		if(pwdn_gpio != -1){
			ret = gpio_request(pwdn_gpio,"pwdn");
			if(!ret){
				gpio_direction_output(pwdn_gpio, 1);
				mdelay(150);
				gpio_direction_output(pwdn_gpio, 0);
			}else{
				printk("gpio requrest fail %d\n",pwdn_gpio);
			}
		}

		for (j = 0; j < idcnt; j++) {
			uint32_t value = 0;
			ret = sensor_read(&g_sinfo[i], adap, g_sinfo[i].id_addr[j], &value);
			if (0 != ret) {
				printk("err sensor read addr = 0x%x, value = 0x%x\n", g_sinfo[i].id_addr[j], value);
				break;
			}
			if (value != g_sinfo[i].id_value[j])
				break;
		}

		if (-1 != reset_gpio)
			gpio_free(reset_gpio);
		if (-1 != pwdn_gpio)
			gpio_free(pwdn_gpio);
		clk_disable(mclk);
		clk_put(mclk);
		if (j == idcnt) {
			printk("info: success sensor find : %s\n", g_sinfo[i].name);
			g_sinfo[i].adap = adap;
			g_sensor_id = i;
			goto end_sensor_find;
		}
	}
	printk("info: failed sensor find\n");
	g_sensor_id = -1;
	mutex_unlock(&g_mutex);
	return 0;
end_sensor_find:
	mutex_unlock(&g_mutex);
	return 0;
}

static long sinfo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int32_t data;

	mutex_lock(&g_mutex);
	switch (cmd) {
	case IOCTL_SINFO_GET:
		if (-1 == g_sensor_id)
			data = -1;
		else
			data = g_sensor_id;
		if (copy_to_user((void *)arg, &data, sizeof(data))) {
			printk("copy_from_user error!!!\n");
			ret = -EFAULT;
			break;
		}
		break;
	case IOCTL_SINFO_FLASH:
		i2c_for_each_dev(NULL, process_one_adapter);
		break;
	default:
		printk("invalid command: 0x%08x\n", cmd);
		ret = -EINVAL;
	}
	mutex_unlock(&g_mutex);
	return ret;
}
static int sinfo_open(struct inode *inode, struct file *filp)
{
	i2c_for_each_dev(NULL, process_one_adapter);
	return 0;
}
static int sinfo_release(struct inode *inode, struct file *filp)
{
	printk ("misc sinfo_release\n");
	return 0;
}

static ssize_t sinfo_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

static struct file_operations sinfo_fops =
{
	.owner = THIS_MODULE,
	.read = sinfo_read,
	.unlocked_ioctl = sinfo_ioctl,
	.open = sinfo_open,
	.release = sinfo_release,
};

static struct miscdevice misc_sinfo = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "sinfo",
	.fops = &sinfo_fops,
};


static int sinfo_proc_show(struct seq_file *m, void *v)
{
	if (-1 == g_sensor_id)
		seq_printf(m, "sensor not found\n");
	else
		seq_printf(m, "sensor :%s\n", g_sinfo[g_sensor_id].name);
	return 0;
}

static int sinfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sinfo_proc_show, NULL);
}

ssize_t sinfo_proc_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
	uint8_t data;
	if(copy_from_user(&data, buf, 1))
	{
		return -EFAULT;
	}
	if ('1' == data) {
		i2c_for_each_dev(NULL, process_one_adapter);
	}
	return len;
}

static const struct file_operations sinfo_proc_fops = {
	.owner = THIS_MODULE,
	.open = sinfo_proc_open,
	.read = seq_read,
	.write = sinfo_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};

struct proc_dir_entry *g_sinfo_proc;
static __init int init_sinfo(void)
{
	int ret = 0;
	mutex_init(&g_mutex);
	g_sinfo_proc = proc_mkdir("jz/sinfo", 0);
	if (!g_sinfo_proc) {
		printk("err: jz_proc_mkdir failed\n");
	}
	proc_create_data("info", S_IRUGO, g_sinfo_proc, &sinfo_proc_fops, NULL);
	/* i2c_for_each_dev(NULL, process_one_adapter); */
	ret = misc_register(&misc_sinfo);
	/* printk("##### g_sensor_id = %d\n", g_sensor_id); */
	return ret;

}

static __exit void exit_sinfo(void)
{
	proc_remove(g_sinfo_proc);
	misc_deregister(&misc_sinfo);
}

module_init(init_sinfo);
module_exit(exit_sinfo);

MODULE_DESCRIPTION("A Simple driver for get sensors info ");
MODULE_LICENSE("GPL");
