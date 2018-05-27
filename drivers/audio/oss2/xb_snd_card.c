/**
 * xb_snd_card.c
 *
 */
#include <linux/module.h>
#include <mach/jzsnd.h>
#include "interface/xb_snd_dsp.h"
#include "devices/ex_codecs/codec_i2c_dev.h"
#include "devices/ex_codecs/es8374_codec.h"

static LIST_HEAD(ddata_head);

unsigned int snd_debug_level = 1;
module_param(snd_debug_level, int, S_IRUGO);
MODULE_PARM_DESC(snd_debug_level, "The debug level of sound driver");

int codec_type = 0;
module_param(codec_type, int, S_IRUGO);
MODULE_PARM_DESC(codec_type, "control using internal codec or external codec,0 --internal codec, 1 -- external codec");

/*###########################################################*\
 * support functions
 \*###########################################################*/
static inline struct snd_dev_data *get_ddata_by_minor(int minor)
{
	struct snd_dev_data *ddata = NULL;

	list_for_each_entry(ddata, &ddata_head, list) {
		if (ddata && (ddata->minor == minor)) {
			return ddata;
		}
	}

	snd_error_print("no device has minor %d !\n", minor);
	return NULL;
}

/*###########################################################*\
 * file operations
 \*###########################################################*/
/********************************************************\
 * llseek
\********************************************************/
static loff_t xb_snd_llseek(struct file *file, loff_t offset, int origin)
{
	loff_t ret = 0;
	int dev = iminor(file->f_path.dentry->d_inode);
	struct snd_dev_data *ddata = get_ddata_by_minor(dev);

	switch (dev & 0x0f) {
	case SND_DEV_DSP:
		ret = xb_snd_dsp_llseek(file, offset, origin, ddata);
		break;
	}

	return ret;
}

/********************************************************\
 * read
\********************************************************/
static ssize_t xb_snd_read(struct file *file, char __user * buffer,
			   size_t count, loff_t * ppos)
{
	ssize_t ret = -EIO;
	int dev = iminor(file->f_path.dentry->d_inode);
	struct snd_dev_data *ddata = get_ddata_by_minor(dev);

	if (ddata->is_suspend)
		return -EIO;

	if (count == 0)
		return 0;

	switch (dev & 0x0f) {
	case SND_DEV_DSP:
		ret = xb_snd_dsp_read(file, buffer, count, ppos, ddata);
		break;
	}

	return ret;
}

/********************************************************\
 * write
\********************************************************/
static ssize_t xb_snd_write(struct file *file, const char __user * buffer,
			    size_t count, loff_t * ppos)
{
	ssize_t ret = -EIO;
	int dev = iminor(file->f_path.dentry->d_inode);
	struct snd_dev_data *ddata = get_ddata_by_minor(dev);

	if (ddata->is_suspend)
		return -EIO;

	if (count == 0)
		return 0;

	switch (dev & 0x0f) {
	case SND_DEV_DSP:
		ret = xb_snd_dsp_write(file, buffer, count, ppos, ddata);
		break;
	}

	return ret;
}

/********************************************************\
 * poll
\********************************************************/
static unsigned int xb_snd_poll(struct file *file, poll_table * wait)
{
	unsigned int ret = 0;
	int dev = iminor(file->f_path.dentry->d_inode);
	struct snd_dev_data *ddata = get_ddata_by_minor(dev);

	switch (dev & 0x0f) {
	case SND_DEV_DSP:
		ret = xb_snd_dsp_poll(file, wait, ddata);
		break;
	}

	return ret;
}

/********************************************************\
 * ioctl
\********************************************************/
static long xb_snd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = -EIO;
	int dev = iminor(file->f_path.dentry->d_inode);
	struct snd_dev_data *ddata = get_ddata_by_minor(dev);
	switch (dev & 0x0f) {
	case SND_DEV_DSP:
		ret = xb_snd_dsp_ioctl(file, cmd, arg, ddata);
		break;
	}

	return ret;
}

/********************************************************\
 * open
\********************************************************/
static int xb_snd_open(struct inode *inode, struct file *file)
{
	int ret = -ENXIO;
	int dev = iminor(inode);
	struct snd_dev_data *ddata = get_ddata_by_minor(dev);

	switch (dev & 0x0f) {
	case SND_DEV_DSP:
		ret = xb_snd_dsp_open(inode, file, ddata);
		break;
	}

	return ret;
}

/********************************************************\
 * release
\********************************************************/
static int xb_snd_release(struct inode *inode, struct file *file)
{
	int ret = -EINVAL;
	int dev = iminor(file->f_path.dentry->d_inode);
	struct snd_dev_data *ddata = get_ddata_by_minor(dev);

	switch (dev & 0x0f) {
	case SND_DEV_DSP:
		ret = xb_snd_dsp_release(inode, file, ddata);
		break;
	}

	return ret;
}

const struct file_operations xb_snd_fops = {
	.owner = THIS_MODULE,
	.llseek = xb_snd_llseek,
	.read = xb_snd_read,
	.write = xb_snd_write,
	.poll = xb_snd_poll,
	.unlocked_ioctl = xb_snd_ioctl,
	.open = xb_snd_open,
	.release = xb_snd_release,
};

/*########################################################*\
 * devices driver
 \*########################################################*/

/********************************************************\
 * xb_snd_suspend
\********************************************************/
static int xb_snd_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = 0;
	struct snd_dev_data *ddata = pdev->dev.platform_data;
	if((ddata->minor & 0x0f) == SND_DEV_DSP)
		ret = xb_snd_dsp_suspend(ddata);
	if(ret)
		return ret;
	if (ddata && ddata->suspend)
		ret = ddata->suspend(pdev, state);
	return ret;
}

/********************************************************\
 * xb_snd_resume
\********************************************************/
static int xb_snd_resume(struct platform_device *pdev)
{
	int ret = 0;
	struct snd_dev_data *ddata = pdev->dev.platform_data;
	if (ddata && ddata->resume)
		ret = ddata->resume(pdev);
	if(ret)
		return ret;
	if((ddata->minor & 0x0f) == SND_DEV_DSP)
		ret = xb_snd_dsp_suspend(ddata);
	return ret;
}

/********************************************************\
 * xb_snd_shutdown
\********************************************************/
static void xb_snd_shutdown(struct platform_device *pdev)
{
	struct snd_dev_data *ddata = pdev->dev.platform_data;
	if (ddata && ddata->shutdown)
		ddata->shutdown(pdev);
}

extern struct snd_dev_data i2s_data;

static struct snd_dev_data *pdata[] = {
	&i2s_data,
	NULL,
};

/********************************************************\
 * xb_snd_probe
\********************************************************/
static int xb_snd_probe(struct platform_device *pdev)
{
	int ret = -EINVAL;
	struct snd_dev_data *ddata = NULL;
	int index = 0;

	while(pdata[index]){
		if(pdata[index]->minor == pdev->id){
			ddata = pdata[index];
			pdev->dev.platform_data = ddata;
			break;
		}
		index++;
	}
	if (ddata) {
		/* check minor */
		if (ddata->minor > MAX_SND_MINOR)
			return -EINVAL;
		ddata->dev = &(pdev->dev);
		if (ddata->init) {
			if (ddata->init(pdev)) {
				snd_error_print("device init error!\n");
				return -1;
			}
		}
		/* device probe */
		switch (ddata->minor & 0x0f) {
		case SND_DEV_DSP:
			ret = xb_snd_dsp_probe(ddata);
			break;

		default:
			snd_error_print("SOUND ERROR:Unkown snd device.\n");
			return -EINVAL;
		}

		if (ret < 0) {
			snd_error_print("SOUND ERROR:Snd device register error.\n");
			return ret;
		}
		/* register device */
		ret = register_sound_special(&xb_snd_fops, ddata->minor);
		if (ret != ddata->minor) {
			snd_error_print("SOUND ERROR: register sound device error! minor = %d\n", ddata->minor);
			return ret;
		}
		list_add(&ddata->list, &ddata_head);
	}

	return 0;
}

static int xb_snd_remove(struct platform_device *pdev)
{
	int ret = -EINVAL;
	struct snd_dev_data *ddata = pdev->dev.platform_data;

	if (ddata) {
		/* device probe */
		switch (ddata->minor & 0x0f) {
		case SND_DEV_DSP:
			ret = xb_snd_dsp_remove(ddata);
			/*ret = 0;*/
			break;

		default:
			snd_error_print("SOUND ERROR:Unkown snd device.\n");
			return -EINVAL;
		}

		if (ret < 0) {
			snd_error_print("SOUND ERROR:Snd device remove error.\n");
			return ret;
		}
		/* register device */
		unregister_sound_special(ddata->minor);

		ddata->dev = &(pdev->dev);
		if (ddata->exit) {
			if (ddata->exit(pdev)) {
				snd_error_print("SOUND ERROR: device release error!\n");
				return -1;
			}
		}
		list_del(&ddata->list);
	}

	return 0;

}
struct platform_device_id xb_snd_driver_ids[] = {
	{
		.name = DEV_DSP_NAME,
		.driver_data = 0,
	},
};

__refdata static struct platform_driver xb_snd_driver = {
	.probe = xb_snd_probe,
	.remove = xb_snd_remove,
	.driver = {
		.name = "dsp",
		.owner = THIS_MODULE,
	},
	.id_table = xb_snd_driver_ids,
	.suspend = xb_snd_suspend,
	.resume = xb_snd_resume,
	.shutdown = xb_snd_shutdown,
};

static int __init xb_snd_init(void)
{
#ifdef CONFIG_JZ_EXTERNAL_CODEC_V12
	codec_i2c_drv_init(1);
#endif
	return platform_driver_register(&xb_snd_driver);
}

static void __exit xb_snd_exit(void)
{
#ifdef CONFIG_JZ_EXTERNAL_CODEC_V12
	codec_i2c_drv_exit(1);
#endif
	platform_driver_unregister(&xb_snd_driver);
}

module_init(xb_snd_init);
module_exit(xb_snd_exit);
MODULE_AUTHOR("Niky <xianghui.shen@ingenic.com>");
MODULE_DESCRIPTION("TXX codec driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("2.0");
