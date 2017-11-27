/*
 * linux/drivers/mtd/devices/jz_spi_norflash.c
 *
 * Add spi-norflsah to mtd
 *
 * Copyright (c) 2014 Ingenic
 * Author:Tiger <bohu.liang@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/delay.h>
#include "jz_spi_norflash.h"

/* Max time can take up to 3 seconds! */
#define MAX_READY_WAIT_TIME	650	/* the time of erase BE(64KB) */

struct jz_spi_norflash {
	struct spi_device	*spi;
	struct mutex		lock;
	struct mtd_info		mtd;
};

struct spi_device_id jz_id_table[] = {
	{
		.name = "jz_spi_norflash",
	},
};

struct mtd_partition *jz_mtd_partition;

struct jz_spi_norflash *to_jz_spi_norflash(struct mtd_info *mtd_info)
{
	return container_of(mtd_info, struct jz_spi_norflash, mtd);
}

static int jz_spi_norflash_status(struct jz_spi_norflash *flash, int *status)
{
	int ret;
	unsigned char command[1];
	struct spi_message message;
	struct spi_transfer transfer[2];

	spi_message_init(&message);
	memset(&transfer, 0, sizeof(transfer));

	command[0] = SPINOR_OP_RDSR;

	transfer[0].tx_buf = command;
	transfer[0].len = sizeof(command);
	spi_message_add_tail(&transfer[0], &message);

	transfer[1].rx_buf = command;
	transfer[1].len = sizeof(command);
	spi_message_add_tail(&transfer[1], &message);

	ret = spi_sync(flash->spi, &message);
	if (ret)
		return ret;

	*status = command[0];

	return 0;
}

static int jz_spi_norflash_wait_till_ready(struct jz_spi_norflash *flash)
{
	int status, ret;
	unsigned long deadline;

	deadline = jiffies + msecs_to_jiffies(MAX_READY_WAIT_TIME);
	do {
		ret = jz_spi_norflash_status(flash, &status);
		if (ret)
			return ret;

		if (!(status & SR_WIP))
			return 0;

		cond_resched();
	} while (!time_after_eq(jiffies, deadline));

	return -ETIMEDOUT;
}

static int jz_spi_norflash_write_enable(struct jz_spi_norflash *flash)
{
	int status, ret;
	unsigned char command[2];

	command[0] = SPINOR_OP_WREN;
	ret = spi_write(flash->spi, command, 1);
	if (ret) {
		return ret;
	}

	command[0] = SPINOR_OP_WRSR;
	command[1] = 0;
	ret = spi_write(flash->spi, command, 2);
	if (ret) {
		return ret;
	}

	ret = jz_spi_norflash_wait_till_ready(flash);
	if (ret)
		return ret;

	command[0] = SPINOR_OP_WREN;
	ret = spi_write(flash->spi, command, 1);
	if (ret) {
		return ret;
	}

	ret = jz_spi_norflash_status(flash, &status);
	if (ret) {
		return ret;
	}

	if (!(status & SR_WEL)) {
		return -EROFS;
	}

	return 0;
}

static int jz_spi_norflash_erase_sector(struct jz_spi_norflash *flash, uint32_t offset)
{
	int ret;
	unsigned char command[4];

	ret = jz_spi_norflash_write_enable(flash);
	if (ret)
		return ret;

	switch(flash->mtd.erasesize) {
	case 0x1000:
		command[0] = SPINOR_OP_BE_4K;
		break;
	case 0x8000:
		command[0] = SPINOR_OP_BE_32K;
		break;
	case 0x10000:
		command[0] = SPINOR_OP_SE;
		break;
	}

	command[1] = offset >> 16;
	command[2] = offset >> 8;
	command[3] = offset;
	ret = spi_write(flash->spi, command, 4);
	if (ret)
		return ret;

	ret = jz_spi_norflash_wait_till_ready(flash);
	if (ret)
		return ret;

	return 0;
}

static int jz_spi_norflash_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int ret;
	uint32_t addr, end;
	struct jz_spi_norflash *flash;

	flash = to_jz_spi_norflash(mtd);

	addr = (instr->addr & (mtd->erasesize - 1));
	if (addr) {
		printk("%s eraseaddr no align\n", __func__);
		return -EINVAL;
	}

	end = (instr->len & (mtd->erasesize - 1));
	if (end) {
		printk("%s erasesize no align\n", __func__);
		return -EINVAL;
	}

	addr = (uint32_t)instr->addr;
	end = addr + (uint32_t)instr->len;

	mutex_lock(&flash->lock);

	ret = jz_spi_norflash_wait_till_ready(flash);
	if (ret) {
		printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
		printk("spi wait timeout !\n");
		mutex_unlock(&flash->lock);
		return ret;
	}

	while (addr < end) {
		ret = jz_spi_norflash_erase_sector(flash, addr);
		if (ret) {
			printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
			printk("erase error !\n");
			mutex_unlock(&flash->lock);
			instr->state = MTD_ERASE_FAILED;
			return ret;
		}

		addr += mtd->erasesize;
	}

	mutex_unlock(&flash->lock);

	instr->state = MTD_ERASE_DONE;

	mtd_erase_callback(instr);

	return 0;
}

static int jz_spi_norflash_read(struct mtd_info *mtd, loff_t from,
		size_t len, size_t *retlen, unsigned char *buf)
{
	int ret;
	unsigned char command[5];
	struct spi_message message;
	struct jz_spi_norflash *flash;
	struct spi_transfer transfer[2];

	flash = to_jz_spi_norflash(mtd);

	spi_message_init(&message);
	memset(&transfer, 0, sizeof(transfer));

	command[0] = SPINOR_OP_READ_FAST;
	command[1] = from >> 16;
	command[2] = from >> 8;
	command[3] = from;
	command[4] = 0;

	transfer[0].tx_buf = command;
	transfer[0].len = sizeof(command);
	spi_message_add_tail(&transfer[0], &message);

	transfer[1].rx_buf = buf;
	transfer[1].len = len;
	spi_message_add_tail(&transfer[1], &message);

	mutex_lock(&flash->lock);

	ret = jz_spi_norflash_wait_till_ready(flash);
	if (ret) {
		printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
		printk("spi wait timeout !\n");
		mutex_unlock(&flash->lock);
		return ret;
	}

	ret = spi_sync(flash->spi, &message);
	if(ret) {
		printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
		printk("spi_sync() error !\n");
		return ret;
	}

	*retlen = message.actual_length - sizeof(command);

	mutex_unlock(&flash->lock);

	return 0;
}

static int jz_spi_norflash_write(struct mtd_info *mtd, loff_t to, size_t len,
			size_t *retlen, const unsigned char *buf)
{
	unsigned int i, j, ret;
	int ret_addr, actual_len, write_len;
	struct spi_message message;
	struct spi_transfer transfer[1];
	struct jz_spi_norflash *flash;
	unsigned char command[mtd->writesize + 4];

	flash = to_jz_spi_norflash(mtd);

	*retlen = 0;

	ret_addr = (to & (mtd->writesize - 1));

	mutex_lock(&flash->lock);

	if(mtd->writesize - ret_addr > len)
		actual_len = len;
	else
		actual_len = mtd->writesize - ret_addr;

	/* less than mtd->writesize */
	{
		ret = jz_spi_norflash_write_enable(flash);
		if (ret) {
			printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
			printk("write enable error !\n");
			mutex_unlock(&flash->lock);
			return ret;
		}

		spi_message_init(&message);
		memset(&transfer, 0, sizeof(transfer));

		command[0] = SPINOR_OP_PP;
		command[1] = to >> 16;
		command[2] = to >> 8;
		command[3] = to;

		for(j = 0; j < actual_len; j++) {
			command[j + 4] = buf[j];
		}

		transfer[0].tx_buf = command;
		transfer[0].len = actual_len + 4;
		spi_message_add_tail(&transfer[0], &message);

		ret = jz_spi_norflash_wait_till_ready(flash);
		if (ret) {
			printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
			printk("wait timeout !\n");
			mutex_unlock(&flash->lock);
			return ret;
		}

		ret = spi_sync(flash->spi, &message);
		if(ret) {
			printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
			printk("write error !\n");
			return ret;
		}

		*retlen += (message.actual_length - 4);
	}

	for (i = actual_len; i < len; i += mtd->writesize) {
		ret = jz_spi_norflash_write_enable(flash);
		if (ret) {
			printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
			printk("write enable error !\n");
			mutex_unlock(&flash->lock);
			return ret;
		}

		spi_message_init(&message);
		memset(&transfer, 0, sizeof(transfer));

		command[0] = SPINOR_OP_PP;
		command[1] = (to + i) >> 16;
		command[2] = (to + i) >> 8;
		command[3] = (to + i);

		if(len - i < mtd->writesize)
			write_len = len - i;
		else
			write_len = mtd->writesize;

		for(j = 0; j < write_len; j++) {
			command[j + 4] = buf[i + j];
		}

		transfer[0].tx_buf = command;
		transfer[0].len = write_len + 4;
		spi_message_add_tail(&transfer[0], &message);

		ret = jz_spi_norflash_wait_till_ready(flash);
		if (ret) {
			printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
			printk("wait timeout !\n");
			mutex_unlock(&flash->lock);
			return ret;
		}

		ret = spi_sync(flash->spi, &message);
		if(ret) {
			printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
			printk("write error !\n");
			return ret;
		}

		*retlen += (message.actual_length - 4);
	}

	mutex_unlock(&flash->lock);

	if(*retlen != len)
		printk("ret len error!\n");

	return 0;
}

static  int jz_spi_norflash_match_device(struct spi_device *spi,int chip_id)
{
	int ret;
	unsigned int id = 0;
	unsigned char send_command[1], recv_command[3];
	struct spi_message message;
	struct spi_transfer transfer[2];

	spi_message_init(&message);
	memset(&transfer, 0, sizeof(transfer));

	send_command[0] = SPINOR_OP_RDID;

	transfer[0].tx_buf = send_command;
	transfer[0].len = sizeof(send_command);
	spi_message_add_tail(&transfer[0], &message);

	transfer[1].rx_buf = recv_command;
	transfer[1].len = sizeof(recv_command);
	spi_message_add_tail(&transfer[1], &message);

	ret = spi_sync(spi, &message);
	if (ret) {
		printk("error reading device id\n");
		return EINVAL;
	}

	id = (recv_command[0] << 16) | (recv_command[1] << 8) | recv_command[2];

	if(id == chip_id){
		printk("the spi mtd chip id is %x\n",id);
	}else{
		printk("unknow chip id %x\n",id);
		return EINVAL;
	}

	return 0;
}

static int jz_spi_norflash_probe(struct spi_device *spi)
{
	int ret;
	const char *jz_probe_types[] = {"cmdlinepart"};
	struct jz_spi_norflash *flash;
	struct spi_nor_platform_data *pdata = spi->dev.platform_data;
	int chip_id = 0;
	int num_partition_info = 0;

	jz_mtd_partition = pdata->mtd_partition;
	num_partition_info = pdata->num_partition_info;
	chip_id = pdata->id;
	flash = kzalloc(sizeof(struct jz_spi_norflash), GFP_KERNEL);
	if (!flash) {
		printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
		printk("kzalloc() error !\n");
		return -ENOMEM;
	}

	ret = jz_spi_norflash_match_device(spi,chip_id);
	if (ret ) {
		printk("unknow id ,the id not match the spi bsp config\n");
		return -ENODEV;
	}

	flash->spi = spi;
	mutex_init(&flash->lock);
	dev_set_drvdata(&spi->dev, flash);

	flash->mtd.name		= dev_name(&spi->dev);
	flash->mtd.owner	= THIS_MODULE;
	flash->mtd.type		= MTD_NORFLASH;
	flash->mtd.flags	= MTD_CAP_NORFLASH;
	flash->mtd.erasesize	= pdata->erasesize;
	flash->mtd.writesize	= pdata->pagesize;
	flash->mtd.size		= pdata->chipsize;
	flash->mtd._erase	= jz_spi_norflash_erase;
	flash->mtd._read	= jz_spi_norflash_read;
	flash->mtd._write 	= jz_spi_norflash_write;

	ret = mtd_device_parse_register(&flash->mtd, jz_probe_types, NULL, jz_mtd_partition, num_partition_info);
	if (ret) {
		kfree(flash);
		dev_set_drvdata(&spi->dev, NULL);
		return -ENODEV;
	}

	printk("SPI NOR MTD LOAD OK\n");
	return 0;
}

static int jz_spi_norflash_remove(struct spi_device *spi)
{
	int ret;
	struct jz_spi_norflash *flash;

	flash = dev_get_drvdata(&spi->dev);
	if(!flash)
		return 0;

	ret = mtd_device_unregister(&flash->mtd);
	if (!ret) {
		kfree(flash);
		dev_set_drvdata(&spi->dev, NULL);
	}

	return ret;
}

static int jz_spi_norflash_suspend(struct spi_device *spi, pm_message_t mesg)
{
	return 0;
}

static int jz_spi_norflash_resume(struct spi_device *spi)
{
	return 0;
}

static struct spi_driver jz_spi_norflash_driver = {
	.driver = {
		.name	= "jz_spi_norflash",
		.owner	= THIS_MODULE,
	},
	.id_table	= jz_id_table,
	.probe		= jz_spi_norflash_probe,
	.remove		= jz_spi_norflash_remove,
	.suspend	= jz_spi_norflash_suspend,
	.resume		= jz_spi_norflash_resume,
	.shutdown	= NULL, //forever start
};

static int __init jz_spi_norflash_driver_init(void)
{
	return spi_register_driver(&jz_spi_norflash_driver);
}
static void __exit jz_spi_norflash_driver_exit(void)
{
	spi_unregister_driver(&jz_spi_norflash_driver);
}

//module_init(jz_spi_norflash_driver_init);
fs_initcall(jz_spi_norflash_driver_init);
module_exit(jz_spi_norflash_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tiger <bohu.liang@ingenic.com>");
MODULE_DESCRIPTION("MTD SPI driver for ingenic");
