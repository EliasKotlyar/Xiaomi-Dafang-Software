/* *  jz_t10_codec.c - Linux kernel modules for audio codec
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/input-polldev.h>
#include <linux/input.h>

#include "../xb47xx_i2s_v12.h"
#include "jz_t10_codec.h"

#define CODEC_PROC_DEBUG
#ifdef CODEC_PROC_DEBUG
unsigned char * sbuff = NULL;
#define SBUFF_SIZE		128
#endif

static int spk_gpio =  GPIO_PB(31);
module_param(spk_gpio, int, S_IRUGO);
MODULE_PARM_DESC(spk_gpio, "Speaker GPIO NUM");

static int spk_level = 1;
module_param(spk_level, int, S_IRUGO);
MODULE_PARM_DESC(spk_level, "Speaker active level: -1 disable, 0 low active, 1 high active");

static int hpsense_level = 1;
module_param(hpsense_level, int, S_IRUGO);
MODULE_PARM_DESC(hpsense_level, "Hpsense active level");

static int hook_level = -1;
module_param(hook_level, int, S_IRUGO);
MODULE_PARM_DESC(hook_level, "Hook active level");

static struct snd_codec_data codec_data = {
	.codec_sys_clk = 0 ,
	.codec_dmic_clk = 0,
	/* volume */
	.replay_volume_base = 0,
	.record_volume_base = 0,
	.record_digital_volume_base = 23,
	.replay_digital_volume_base = 0,
	/* default route */
	.replay_def_route = {.route = SND_ROUTE_REPLAY_DACRL_TO_ALL,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_ENABLE},
	.record_def_route = {.route = SND_ROUTE_RECORD_MIC1_AN1,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_ENABLE},
	/* device <-> route map */
	.record_headset_mic_route = {.route = SND_ROUTE_RECORD_MIC1_SIN_AN2,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_ENABLE},
	.record_buildin_mic_route = {.route = SND_ROUTE_RECORD_MIC1_AN1,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_ENABLE},
	.replay_headset_route = {.route = SND_ROUTE_REPLAY_DACRL_TO_HPRL,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_DISABLE},
	.replay_speaker_route = {.route = SND_ROUTE_REPLAY_DACRL_TO_ALL,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_ENABLE},
	.replay_headset_and_speaker_route = {.route = SND_ROUTE_REPLAY_DACRL_TO_ALL,
						.gpio_hp_mute_stat = STATE_DISABLE,
						.gpio_spk_en_stat = STATE_ENABLE},
	/* linein route */
	.record_linein_route = {.route = SND_ROUTE_RECORD_LINEIN1_AN2_SIN_TO_ADCL_AND_LINEIN2_AN3_SIN_TO_ADCR,
						.gpio_hp_mute_stat = STATE_DISABLE,
						.gpio_spk_en_stat = STATE_ENABLE,
	},

	.record_linein1_route = {.route = SND_ROUTE_RECORD_LINEIN1_DIFF_AN2,
						.gpio_hp_mute_stat = STATE_DISABLE,
						.gpio_spk_en_stat = STATE_ENABLE,
	},
	.record_linein2_route = {.route = SND_ROUTE_RECORD_LINEIN2_SIN_AN3,
						.gpio_hp_mute_stat = STATE_DISABLE,
						.gpio_spk_en_stat = STATE_ENABLE,
	},
};

static struct snd_codec_data *codec_platform_data = &codec_data;
struct codec_operation *codec_ope = NULL;
extern int i2s_register_codec_2(struct codec_info * codec_dev);

int codec_reg_set(unsigned int reg, int start, int end, int val)
{
	int ret = 0;
	int i = 0, mask = 0;
	unsigned int oldv = 0;
	unsigned int new = 0;
	for (i = 0;  i < (end - start + 1); i++) {
		mask += (1 << (start + i));
	}

	oldv = codec_reg_read(codec_ope, reg);
	new = oldv & (~mask);
	new |= val << start;
	codec_reg_write(codec_ope, reg, new);

	if (new != codec_reg_read(codec_ope, reg)) {
		printk("%s(%d):codec write  0x%08x error!!\n",__func__, __LINE__ ,reg);
		printk("new = 0x%08x, read val = 0x%08x\n", new, codec_reg_read(codec_ope, reg));
		return -1;
	}

	return ret;
}

int init_codec(void)
{
	int i = 0;

	codec_reg_set(TS_CODEC_CGR_00, 0, 1, 0);
	msleep(10);
	codec_reg_set(TS_CODEC_CGR_00, 0, 1, 0x3);
	msleep(10);

	/* Choose DAC I2S Master Mode */
	codec_reg_set(TS_CODEC_CMCR_03, 0, 7, 0x3e);
	codec_reg_set(TS_CODEC_CDCR1_04, 0, 7, 0x10);//DAC I2S interface is I2S Mode.
	codec_reg_set(TS_CODEC_CDCR2_05, 0, 7, 0xe);

	/* codec precharge */
	codec_reg_set(TS_CODEC_CHCR_27, 6, 7, 1);
	msleep(20);
#ifdef CONFIG_SOC_T20
	codec_reg_set(TS_CODEC_CCR_28, 7, 7, 1);
	msleep(20);
	for (i = 0; i < 6; i++) {
		codec_reg_set(TS_CODEC_CCR_28, 0, 6, 0x3f >> (6 - i));
		msleep(30);
	}
	msleep(20);
	codec_reg_set(TS_CODEC_CCR_28, 0, 6, 0x3f);
#else	/* CONFIG_SOC_T10 */
	codec_reg_set(TS_CODEC_CCR_28, 7, 7, 0);
	msleep(20);
	codec_reg_set(TS_CODEC_CCR_28, 0, 6, 0x3f);
	msleep(20);
	for (i = 0; i < 6; i++) {
		codec_reg_set(TS_CODEC_CCR_28, 0, 6, 0x3f >> i);
		msleep(30);
	}
	msleep(20);
	codec_reg_set(TS_CODEC_CCR_28, 0, 6, 0x0);
#endif
	msleep(20);

	return 0;
}

static int codec_set_buildin_mic(void)
{
	/* ADC valid data length */
	codec_reg_set(TS_CODEC_CACR_02, 5, 6, 0x0);
	msleep(10);
	/* Choose ADC I2S interface mode */
	/*codec_reg_set(TS_CODEC_CACR_02, 3, 4, ADC_I2S_INTERFACE_LJ_MODE);*/
	codec_reg_set(TS_CODEC_CACR_02, 3, 4, 0x2);
	msleep(10);
	/* Choose ADC chn */
	codec_reg_set(TS_CODEC_CACR_02, 0, 0, 0x1);
	msleep(10);

	/* Choose ADC I2S Master Mode */
	codec_reg_set(TS_CODEC_CMCR_03, 0, 7, 0x3e);
	msleep(10);

	/* set sample rate */
	codec_reg_set(TS_CODEC_CSRR_44, 0, 2, 0x7);
	msleep(10);

	/* set record volume */
	codec_reg_set(TS_CODEC_CACR2_23, 0, 4, 0xc);
	msleep(10);

	/* MIC mode: 1: Signal-ended input , 0: Full Diff input */
	codec_reg_set(TS_CODEC_CACR2_23, 5, 5, 1);
	msleep(10);
	/* enable current source of ADC module, enable mic bias */
	codec_reg_set(TS_CODEC_CAACR_21, 7, 7, 1);
	msleep(10);
	codec_reg_set(TS_CODEC_CAACR_21, 6, 6, 1);
	msleep(10);
	/* set MIC Bias is 1.6 * vcc */
	codec_reg_set(TS_CODEC_CAACR_21, 0, 2, 6);
	msleep(10);
	/* enable ref voltage for ADC */
	codec_reg_set(TS_CODEC_CAMPCR_24, 7, 7, 1);
	msleep(10);
	/* enable BST mode */
	codec_reg_set(TS_CODEC_CMICCR_22, 6, 6, 1);
	msleep(10);
	/* enable ALC mode */
	codec_reg_set(TS_CODEC_CMICCR_22, 0, 0, 1);
	msleep(10);
	/* enable ADC clk and ADC amplifier */
	codec_reg_set(TS_CODEC_CAMPCR_24, 6, 6, 1);
	msleep(10);
	codec_reg_set(TS_CODEC_CAMPCR_24, 5, 5, 1);
	msleep(10);
	/* make ALC in work state */
	codec_reg_set(TS_CODEC_CMICCR_22, 1, 1, 1);
	msleep(10);
	/* make BST in work state */
	codec_reg_set(TS_CODEC_CMICCR_22, 4, 4, 1);
	msleep(10);
	/* enable zero-crossing detection */
	codec_reg_set(TS_CODEC_CAACR_21, 5, 5, 1);
	msleep(10);
	/* record ALC gain 6dB */
	codec_reg_set(TS_CODEC_CACR2_23, 0, 4, 0x13);
	msleep(10);
	/* record fix alg gain 20dB */
	codec_reg_set(TS_CODEC_CMICCR_22, 5, 5, 1);
	msleep(10);

	return 0;
}

static int codec_set_speaker(void)
{
	int val = -1;

	codec_reg_set(TS_CODEC_CAACR_21, 6, 7, 0x3);
	msleep(10);
	/* enable current source */
	codec_reg_set(TS_CODEC_CAR_25, 6, 6, 1);
	msleep(10);
	/* enable reference voltage */
	codec_reg_set(TS_CODEC_CAR_25, 5, 5, 1);
	msleep(10);
	/* enable POP sound control */
	codec_reg_set(TS_CODEC_CHCR_27, 6, 7, 2);
	msleep(10);
	/* enable ADC */
	codec_reg_set(TS_CODEC_CAR_25, 3, 3, 0x1);
	msleep(10);
	codec_reg_set(TS_CODEC_CAR_25, 2, 2, 0x1);
	msleep(10);
	codec_reg_set(TS_CODEC_CAR_25, 1, 1, 0x1);
	msleep(10);
	codec_reg_set(TS_CODEC_CAR_25, 0, 0, 0x1);
	msleep(10);
	/* enable HP OUT */
	codec_reg_set(TS_CODEC_CHR_26, 7, 7, 0x1);
	msleep(10);
	codec_reg_set(TS_CODEC_CHR_26, 6, 6, 0x1);
	msleep(10);
	codec_reg_set(TS_CODEC_CHR_26, 5, 5, 0x1);
	msleep(10);

	/* set sample rate */
	codec_reg_set(TS_CODEC_CSRR_44, 0, 2, 0x7);
	msleep(10);

	/* set replay volume */
	codec_reg_set(TS_CODEC_CHCR_27, 0, 4, 0x18);

	if (codec_platform_data->gpio_spk_en.gpio > 0) {
		val = gpio_get_value(codec_platform_data->gpio_spk_en.gpio);
		printk(KERN_DEBUG"----%s %d: gpio pb31 = %d\n", __func__, __LINE__, val);
		gpio_direction_output(codec_platform_data->gpio_spk_en.gpio, codec_platform_data->gpio_spk_en.active_level);
		val = gpio_get_value(codec_platform_data->gpio_spk_en.gpio);
		printk(KERN_DEBUG"----%s %d: gpio pb31 = %d\n", __func__, __LINE__, val);
	}

	return 0;
}

unsigned int cur_out_device = -1;
static int codec_set_device(enum snd_device_t device)
{
	int ret = 0;
	int iserror = 0;

	printk("codec_set_device %d \n",device);
	switch (device) {
	case SND_DEVICE_SPEAKER:
		ret = codec_set_speaker();
		printk("%s: set device: speaker...\n", __func__);
		break;

	case SND_DEVICE_BUILDIN_MIC:
		printk("%s: set device: MIC...\n", __func__);
		ret = codec_set_buildin_mic();
		break;
	default:
		iserror = 1;
		printk("JZ CODEC: Unkown ioctl argument %d in SND_SET_DEVICE\n",device);
	};

	if (!iserror)
		cur_out_device = device;

	return ret;
}

static int codec_set_sample_rate(unsigned int rate)
{
	int i = 0;

	unsigned int mrate[8] = {8000, 12000, 16000, 24000, 32000, 44100, 48000, 96000};
	unsigned int rate_fs[8] = {7, 6, 5, 4, 3, 2, 1, 0};
	for (i = 0; i < 8; i++) {
		if (rate <= mrate[i])
			break;
	}
	if (i == 8)
		i = 0;
	codec_reg_set(TS_CODEC_CSRR_44, 0, 2, rate_fs[i]);
	return 0;
}

static int codec_turn_off(int mode)
{
	int ret = 0;
	if (mode & CODEC_RMODE) {
		/*close record*/
		codec_reg_set(TS_CODEC_CAMPCR_24, 5, 7, 0x0);
	}
	if (mode & CODEC_WMODE) {
		/*close replay*/
		codec_reg_set(TS_CODEC_CAR_25, 0, 3, 0x0);
		if (codec_platform_data->gpio_spk_en.gpio > 0)
			gpio_direction_output(codec_platform_data->gpio_spk_en.gpio,
					      !codec_platform_data->gpio_spk_en.active_level);
	}

	return ret;
}

void codec_set_play_volume(int * vol)
{
	int volume = 0;

	if (*vol < 0) *vol = 0;
	if (*vol > 0x1f) *vol = 0x1f;

	volume = *vol;
	printk("%s %d: codec set play volume = 0x%02x\n",__func__ , __LINE__,  volume);
	codec_reg_set(TS_CODEC_CHCR_27, 0, 4, volume);
}

void codec_set_record_volume(int * vol)
{
	/******************************************\
	| vol :0	1		2	..	30  31	       |
	| gain:-18  -16.5  -15	..	27  28.5 (+DB) |
	\******************************************/
	int volume = 0;
	if (*vol < 0) *vol = 0;
	if (*vol > 31) *vol = 31;
	volume = *vol;

	codec_reg_set(TS_CODEC_CACR2_23, 0, 4, volume);
}

int dump_codec_regs(void)
{
	int i = 0;
	printk("\nDump T10 Codec register:\n");
#if 0
	int codec_reg_buf[] = {TS_CODEC_CGR_00,	   TS_CODEC_CACR_02,   TS_CODEC_CMCR_03,    TS_CODEC_CDCR1_04,
						   TS_CODEC_CDCR2_05,  TS_CODEC_CADR_07,   TS_CODEC_CGAINR_0a,  TS_CODEC_CDPR_0e,
						   TS_CODEC_CDDPR2_0f, TS_CODEC_CDDPR1_10, TS_CODEC_CDDPR0_11,  TS_CODEC_CAACR_21,
						   TS_CODEC_CMICCR_22, TS_CODEC_CACR2_23,  TS_CODEC_CAMPCR_24,  TS_CODEC_CAR_25,
						   TS_CODEC_CHR_26,    TS_CODEC_CHCR_27,   TS_CODEC_CCR_28,	    TS_CODEC_CMR_40,
						   TS_CODEC_CTR_41,    TS_CODEC_CAGCCR_42, TS_CODEC_CPGR_43,    TS_CODEC_CSRR_44,
						   TS_CODEC_CALMR_45,  TS_CODEC_CAHMR_46,  TS_CODEC_CALMINR_47, TS_CODEC_CAHMINR_48,
						   TS_CODEC_CAFG_49,   TS_CODEC_CCAGVR_4c, 0xffff};
	for(i = 0; codec_reg_buf[i] != 0xffff; i++) {
		printk("reg = 0x%04x [0x%04x]:		val = 0x%04x\n", codec_reg_buf[i], codec_reg_buf[i]/4, codec_reg_read(codec_ope, codec_reg_buf[i]));
	}
#endif
	for (i = 0; i < 0x140; i += 4) {
		printk("reg = 0x%04x [0x%04x]:		val = 0x%04x\n", i, i/4, codec_reg_read(codec_ope, i));
	}
	printk("Dump T10 Codec register End.\n");

	return i;
}

static int codec_codec_ctl(struct codec_info *codec_dev, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	/*DUMP_IOC_CMD("enter");*/
	{
		switch (cmd) {
		case CODEC_INIT:
			ret = init_codec();
			break;

		case CODEC_TURN_OFF:
			printk("%s: set CODEC_TURN_OFF...\n", __func__);
			ret = codec_turn_off(arg);
			break;

		case CODEC_SHUTDOWN:
			printk("%s: set CODEC_SHUTDOWN...\n", __func__);
			break;

		case CODEC_RESET:
			break;

		case CODEC_SUSPEND:
			break;

		case CODEC_RESUME:
			break;

		case CODEC_ANTI_POP:
			break;

		case CODEC_SET_DEFROUTE:
			break;

		case CODEC_SET_DEVICE:
			printk(KERN_DEBUG"%s: set device...\n", __func__);
			ret = codec_set_device(*(enum snd_device_t *)arg);
			break;

		case CODEC_SET_STANDBY:
			break;

		case CODEC_SET_REPLAY_RATE:
		case CODEC_SET_RECORD_RATE:
			printk(KERN_DEBUG"%s: set sample rate...\n", __func__);
			ret = codec_set_sample_rate(*(enum snd_device_t *)arg);
			break;
		case CODEC_SET_MIC_VOLUME:
			break;
		case CODEC_SET_RECORD_VOLUME:
			codec_set_record_volume((int *)arg);
			break;
		case CODEC_SET_RECORD_CHANNEL:
			break;
		case CODEC_SET_REPLAY_VOLUME:
			codec_set_play_volume((int *)arg);
			break;
		case CODEC_SET_REPLAY_CHANNEL:
			printk(KERN_DEBUG"%s: set repaly channel...\n", __func__);
			break;
		case CODEC_DAC_MUTE:
			break;
		case CODEC_ADC_MUTE:
			break;
		case CODEC_DUMP_REG:
			ret = dump_codec_regs();
			break;
		case CODEC_CLR_ROUTE:
			printk("%s: set CODEC_CLR_ROUTE...\n", __func__);
			break;
		case CODEC_DEBUG:
			break;
		default:
			break;
		}
	}

	/*DUMP_IOC_CMD("leave");*/

	return ret;
}

#ifdef CODEC_PROC_DEBUG
static ssize_t codec_proc_read(struct file *filp, char __user * buff, size_t len, loff_t * offset)
{
	printk(KERN_INFO "[+codec_proc] call codec_proc_read()\n");

	len = strlen(sbuff);
	if (*offset >= len) {
		return 0;
	}
	len -= *offset;
	if (copy_to_user(buff, sbuff + *offset, len)) {
		return -EFAULT;
	}
	*offset += len;
	printk("%s: sbuff = %s\n", __func__, sbuff);

	return len;
}

static ssize_t codec_proc_write(struct file *filp, const char __user * buff, size_t len, loff_t * offset)
{
	int i = 0;
	int ret = 0;
	int control[4] = {0};
	unsigned char *p = NULL;
	char *after = NULL;

	printk("%s: set codec proc:\n", __func__);
	memset(sbuff, 0, SBUFF_SIZE);
	len = len < SBUFF_SIZE ? len : SBUFF_SIZE;
	if (copy_from_user(sbuff, buff, len)) {
		printk(KERN_INFO "[+codec_proc]: copy_from_user() error!\n");
		return -EFAULT;
	}
	p = sbuff;
	control[0] = simple_strtoul(p, &after, 0);
	printk("control[0] = 0x%08x, after = %s\n", control[0], after);
	for (i = 1; i < 4; i++) {
		if (after[0] == ' ')
			after++;
		p = after;
		control[i] = simple_strtoul(p, &after, 0);
		printk("control[%d] = 0x%08x, after = %s\n", i, control[i], after);
	}

	if (control[0] == 99) {
		printk("read reg:0x%04x = 0x%04x\n",control[1], codec_reg_read(codec_ope, control[1]));
	} else {
		printk("reg   :0x%02x\n", control[0]);
		printk("start :0x%02x\n", control[1]);
		printk("end   :0x%02x\n", control[2]);
		printk("val   :0x%02x\n", control[3]);
		codec_reg_set(control[0], control[1], control[2], control[3]);
	}

	if (control[0] == 100) {
		ret = dump_codec_regs();
	}

	return len;
}

static const struct file_operations codec_devices_fileops = {
	.owner		= THIS_MODULE,
	.read		= codec_proc_read,
	.write		= codec_proc_write,
};
#endif

static int jz_codec_register(struct platform_device *pdev)
{
	codec_ope = (struct codec_operation *)kzalloc(sizeof(struct codec_operation), GFP_KERNEL);
	if (!codec_ope) {
		dev_err(&pdev->dev, "alloc codec mem_region failed!\n");
		return -ENOMEM;
	}

	codec_ope->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!codec_ope->res) {
		dev_err(&pdev->dev, "failed to get dev resources\n");
		return -EINVAL;
	}

	codec_ope->res = request_mem_region(codec_ope->res->start,
			codec_ope->res->end - codec_ope->res->start + 1,
			pdev->name);
	if (!codec_ope->res) {
		dev_err(&pdev->dev, "failed to request regs memory region");
		return -EINVAL;
	}
	codec_ope->iomem = ioremap(codec_ope->res->start, resource_size(codec_ope->res));
	if (!codec_ope->iomem) {
		dev_err(&pdev->dev, "failed to remap regs memory region\n");
		return -EINVAL;
	}
//	printk("%s, codec iomem is :0x%08x\n", __func__, (unsigned int)codec_ope->iomem);

	codec_platform_data->gpio_spk_en.gpio = spk_gpio;
	codec_platform_data->gpio_spk_en.active_level = spk_level;
	codec_platform_data->hpsense_active_level = hpsense_level;
	codec_platform_data->hook_active_level = hook_level;

	if (codec_platform_data->gpio_spk_en.gpio != -1 ) {
		if (gpio_request(codec_platform_data->gpio_spk_en.gpio,"gpio_spk_en") < 0) {
			gpio_free(codec_platform_data->gpio_spk_en.gpio);
			gpio_request(codec_platform_data->gpio_spk_en.gpio,"gpio_spk_en");
			printk(KERN_DEBUG"request spk en gpio %d error!\n", codec_platform_data->gpio_spk_en.gpio);
		}
		printk(KERN_DEBUG"request spk en gpio %d ok!\n", codec_platform_data->gpio_spk_en.gpio);
	}

	pdev->dev.platform_data = codec_platform_data;
	platform_set_drvdata(pdev, &codec_ope);

//	printk("%s: probe() done\n", __func__);
	return 0;
}

static inline int jz_codec_release(void)
{
	iounmap(codec_ope->iomem);
	release_mem_region(codec_ope->res->start,
			codec_ope->res->end - codec_ope->res->start + 1);
	if (codec_platform_data->gpio_spk_en.gpio > 0)
		gpio_free(codec_platform_data->gpio_spk_en.gpio);

	return 0;
}

static int codec_match(struct device *dev, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	int ret = 0;

	if(!get_device(dev))
		return -ENODEV;
	if(!strncmp(pdev->name, "jz-codec", sizeof("jz-codec"))){
			ret = jz_codec_register(pdev);
	}
	put_device(dev);
	return ret;
}

#define T10_INTERNAL_CODEC_CLOCK 12000000
int jz_codec_init(void)
{
	int retval;

	struct codec_info * codec_dev;

	codec_dev = kzalloc(sizeof(struct codec_info), GFP_KERNEL);
	if(!codec_dev) {
		printk("alloc codec device error\n");
		return -1;
	}
	sprintf(codec_dev->name, "internal_codec");
	codec_dev->codec_ctl_2 = codec_codec_ctl;
	codec_dev->codec_clk = T10_INTERNAL_CODEC_CLOCK;
	codec_dev->codec_mode = CODEC_MASTER;

#ifdef CODEC_PROC_DEBUG
	{
		struct proc_dir_entry *proc_codec_dir;
		struct proc_dir_entry *entry;

		sbuff = kmalloc(SBUFF_SIZE, GFP_KERNEL);
		proc_codec_dir = proc_mkdir("codecs", NULL);
		if (!proc_codec_dir)
			return -ENOMEM;

		entry = proc_create("codec", 0, proc_codec_dir,
				&codec_devices_fileops);
		if (!entry) {
			printk("%s: create proc_create error!\n", __func__);
			return -1;
		}
		codec_dev->proc_codec_dir = proc_codec_dir;
		codec_dev->entry		= entry;
	}
#endif

	i2s_register_codec_2(codec_dev);
	retval = bus_for_each_dev(&platform_bus_type, NULL, NULL, codec_match);
	if (retval) {
    	printk("%s[%d]: Failed to register codec driver!\n",__func__,__LINE__);
		return retval;
	}

	if(codec_ope){
		codec_ope->priv = codec_dev;
	}
	return retval;
}

void jz_codec_exit(void)
{
	struct codec_info * codec_dev;
	jz_codec_release();
	if(!codec_ope || !codec_ope->priv)
		return;

	codec_dev = codec_ope->priv;

	i2s_release_codec_2(codec_dev);
#ifdef CODEC_PROC_DEBUG
	proc_remove(codec_dev->entry);
	proc_remove(codec_dev->proc_codec_dir);
	kfree(sbuff);
	sbuff = NULL;
#endif
	kfree(codec_dev);
	kfree(codec_ope);
	codec_ope = NULL;
}
