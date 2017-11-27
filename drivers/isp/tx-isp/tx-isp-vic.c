#include <linux/delay.h>
#include <media/v4l2-common.h>
#include <linux/v4l2-mediabus.h>
#include <linux/mm.h>
#include <linux/clk.h>
#include "tx-isp-vic.h"

//#ifdef CONFIG_PRODUCT_FPGA
#if 0
static int isp_vic_init_clk(struct tx_isp_vic_driver *vsd)
{
	return 0;
}
static long isp_vic_set_clk(struct tx_isp_vic_driver *vsd, int state)
{
	return 0;
}
static int isp_vic_release_clk(struct tx_isp_vic_driver *vsd)
{
	return 0;
}
#else
static int isp_vic_init_clk(struct tx_isp_vic_driver *vsd)
{
	struct clk **clks = NULL;
	struct v4l2_device *v4l2_dev = vsd->sd.v4l2_dev;
	struct tx_isp_subdev_platform_data *pdata = vsd->pdata;
	struct tx_isp_subdev_clk_info *info = pdata->clks;
	unsigned int num = pdata->clk_num;
	int ret = 0;
	int i;

	vsd->clk_num = pdata->clk_num;
	if(!vsd->clk_num){
		vsd->clks = NULL;
		return ISP_SUCCESS;
	}

	clks = (struct clk **)kzalloc(sizeof(clks) * num, GFP_KERNEL);
	if(!clks){
		v4l2_err(v4l2_dev, "%s[%d] Failed to allocate core's clks\n",__func__,__LINE__);
		ret = -ENOMEM;
		goto exit;
	}
	for (i = 0; i < num; i++) {
		clks[i] = clk_get(vsd->dev, info[i].name);
		if (IS_ERR(clks[i])) {
			v4l2_err(v4l2_dev, "Failed to get %s clock %ld\n", info[i].name, PTR_ERR(clks[i]));
			ret = PTR_ERR(clks[i]);
			goto failed_to_get_clk;
		}
		if(info[i].rate != DUMMY_CLOCK_RATE) {
			ret = clk_set_rate(clks[i], info[i].rate);
			if(ret){
				v4l2_err(v4l2_dev, "Failed to set %s clock rate(%ld)\n", info[i].name, info[i].rate);
				goto failed_to_get_clk;
			}
		}
	}
	vsd->clks = clks;
	return ISP_SUCCESS;
failed_to_get_clk:
	while(--i >= 0){
		clk_put(clks[i]);
	}
	kfree(clks);
exit:
	return ret;
}

static long isp_vic_set_clk(struct tx_isp_vic_driver *vsd, int state)
{
	struct clk **clks = vsd->clks;
	int i;

	if(state){
		for(i = 0; i < vsd->clk_num; i++)
			clk_enable(clks[i]);
	}else{
		for(i = vsd->clk_num - 1; i >=0; i--)
			clk_disable(clks[i]);
	}
	return ISP_SUCCESS;
}


static int isp_vic_release_clk(struct tx_isp_vic_driver *vsd)
{
	struct clk **clks = vsd->clks;
	int i = 0;

	if(clks == NULL)
		return 0;
	for (i = 0; i < vsd->clk_num; i++)
		clk_put(clks[i]);

	kfree(clks);
	return 0;
}
#endif
void dump_vic_reg(struct tx_isp_vic_driver *vsd)
{
	printk("BT_DVP_CFG : %08x\n", tx_isp_vic_readl(vsd,VIC_DB_CFG));
#if VIC_SUPPORT_MIPI
	printk("INTERFACE_TYPE : %08x\n", tx_isp_vic_readl(vsd,VIC_INTF_TYPE));
	printk("IDI_TYPE_TYPE : %08x\n", tx_isp_vic_readl(vsd,VIC_IDI_TYPE));
#endif
	printk("RESOLUTION : %08x\n", tx_isp_vic_readl(vsd,VIC_RESOLUTION));
	printk("AB_VALUE : %08x\n", tx_isp_vic_readl(vsd,VIC_AB_VALUE));
	printk("GLOBAL_CONFIGURE : %08x\n", tx_isp_vic_readl(vsd,VIC_GLOBAL_CFG));
	printk("CONTROL : %08x\n", tx_isp_vic_readl(vsd,VIC_CONTROL));
	printk("PIXEL : 0x%08x\n", tx_isp_vic_readl(vsd,VIC_PIXEL));
	printk("LINE : 0x%08x\n", tx_isp_vic_readl(vsd,VIC_LINE));
	printk("VIC_STATE : 0x%08x\n", tx_isp_vic_readl(vsd,VIC_STATE));
	printk("OFIFO_COUNT : 0x%08x\n", tx_isp_vic_readl(vsd,VIC_OFIFO_COUNT));
	printk("0x3c :0x%08x\n",tx_isp_vic_readl(vsd,0x3c));
	printk("0x40 :0x%08x\n",tx_isp_vic_readl(vsd,0x40));
	printk("0x44 :0x%08x\n",tx_isp_vic_readl(vsd,0x44));
	printk("0x54 :0x%08x\n",tx_isp_vic_readl(vsd,0x54));
	printk("0x58 :0x%08x\n",tx_isp_vic_readl(vsd,0x58));
	printk("0x58 :0x%08x\n",tx_isp_vic_readl(vsd,0x5c));
	printk("0x60 :0x%08x\n",tx_isp_vic_readl(vsd,0x60));
	printk("0x64 :0x%08x\n",tx_isp_vic_readl(vsd,0x64));
	printk("0x68 :0x%08x\n",tx_isp_vic_readl(vsd,0x68));
	printk("0x6c :0x%08x\n",tx_isp_vic_readl(vsd,0x6c));
	printk("0x70 :0x%08x\n",tx_isp_vic_readl(vsd,0x70));
	printk("0x74 :0x%08x\n",tx_isp_vic_readl(vsd,0x74));
	printk("0x04 :0x%08x\n",tx_isp_vic_readl(vsd,0x04));
	printk("0x08 :0x%08x\n",tx_isp_vic_readl(vsd,0x08));
	printk("VIC_EIGHTH_CB :0x%08x\n",tx_isp_vic_readl(vsd,VIC_EIGHTH_CB));
	printk("CB_MODE0 :0x%08x\n",tx_isp_vic_readl(vsd,CB_MODE0));
}
static struct tx_isp_vic_driver *dump_vsd = NULL;
void check_vic_error(void){
	while(1){
		dump_vic_reg(dump_vsd);
	}
}

static int tx_isp_vic_start(struct tx_isp_vic_driver *vsd)
{
	struct tx_isp_video_in *vin = &vsd->vin;
	int ret = ISP_SUCCESS;
	unsigned int input_cfg = 0;

	if(vin->attr->dbus_type == TX_SENSOR_DATA_INTERFACE_MIPI){
#if VIC_SUPPORT_MIPI
		tx_isp_vic_writel(vsd,VIC_INTF_TYPE, INTF_TYPE_MIPI);//MIPI
		switch(vin->mbus.code){
			case V4L2_MBUS_FMT_SBGGR8_1X8:
			case V4L2_MBUS_FMT_SGBRG8_1X8:
			case V4L2_MBUS_FMT_SGRBG8_1X8:
			case V4L2_MBUS_FMT_SRGGB8_1X8:
				tx_isp_vic_writel(vsd,VIC_IDI_TYPE,MIPI_RAW8);//RAW8
				break;
			case 	V4L2_MBUS_FMT_SBGGR10_1X10:
			case	V4L2_MBUS_FMT_SGBRG10_1X10:
			case	V4L2_MBUS_FMT_SGRBG10_1X10:
			case	V4L2_MBUS_FMT_SRGGB10_1X10:
				tx_isp_vic_writel(vsd,VIC_IDI_TYPE,MIPI_RAW10);//RAW10
				break;
			case	V4L2_MBUS_FMT_SBGGR12_1X12:
			case	V4L2_MBUS_FMT_SGBRG12_1X12:
			case	V4L2_MBUS_FMT_SGRBG12_1X12:
			case	V4L2_MBUS_FMT_SRGGB12_1X12:
				tx_isp_vic_writel(vsd,VIC_IDI_TYPE,MIPI_RAW12);//RAW12
				break;
			case V4L2_MBUS_FMT_RGB555_2X8_PADHI_LE:
				tx_isp_vic_writel(vsd,VIC_IDI_TYPE,MIPI_RGB555);//RGB555
				break;
			case V4L2_MBUS_FMT_RGB565_2X8_LE:
				tx_isp_vic_writel(vsd,VIC_IDI_TYPE,MIPI_RGB565);//RGB565
				break;
			case 	V4L2_MBUS_FMT_UYVY8_1_5X8 :
			case	V4L2_MBUS_FMT_VYUY8_1_5X8 :
			case	V4L2_MBUS_FMT_YUYV8_1_5X8 :
			case	V4L2_MBUS_FMT_YVYU8_1_5X8 :
			case	V4L2_MBUS_FMT_YUYV8_1X16 :
				tx_isp_vic_writel(vsd,VIC_IDI_TYPE,MIPI_YUV422);//YUV422
				break;

		}
		ret = (vin->mbus.width<< 16) | (vin->mbus.height);
		tx_isp_vic_writel(vsd,VIC_RESOLUTION, ret);
		tx_isp_vic_writel(vsd,VIC_GLOBAL_CFG, ISP_PRESET_MODE1);//ISP_PRESET_MODE1);
		tx_isp_vic_writel(vsd,VIC_CONTROL, REG_ENABLE);
		while(tx_isp_vic_readl(vsd,VIC_CONTROL));
		tx_isp_vic_writel(vsd, VIC_CONTROL, VIC_SRART);
		//dump_vic_reg(dump_vsd);
#endif
	}else if(vin->attr->dbus_type == TX_SENSOR_DATA_INTERFACE_DVP){
#if VIC_SUPPORT_MIPI
		tx_isp_vic_writel(vsd,VIC_INTF_TYPE, INTF_TYPE_DVP);//DVP
#endif
		switch(vin->mbus.code){
			case V4L2_MBUS_FMT_SBGGR8_1X8:
			case V4L2_MBUS_FMT_SGBRG8_1X8:
			case V4L2_MBUS_FMT_SGRBG8_1X8:
			case V4L2_MBUS_FMT_SRGGB8_1X8:
				if (vin->attr->dvp.gpio == DVP_PA_LOW_8BIT) {
					input_cfg = DVP_RAW8; //RAW8 low_align
				} else if (vin->attr->dvp.gpio == DVP_PA_HIGH_8BIT) {
					input_cfg = DVP_RAW8|DVP_RAW_ALIG;//RAW8 high_align
				}else{
					printk("%s[%d] VIC failed to config DVP mode!(8bits-sensor)\n",__func__,__LINE__);
					ret = -1;
				}
				break;
			case V4L2_MBUS_FMT_SBGGR10_1X10:
			case V4L2_MBUS_FMT_SGBRG10_1X10:
			case V4L2_MBUS_FMT_SGRBG10_1X10:
			case V4L2_MBUS_FMT_SRGGB10_1X10:
				if (vin->attr->dvp.gpio == DVP_PA_LOW_10BIT) {
					input_cfg = DVP_RAW10; //RAW10 low_align
				} else if (vin->attr->dvp.gpio == DVP_PA_HIGH_10BIT) {
					input_cfg = DVP_RAW10|DVP_RAW_ALIG;//RAW10 high_align
				}else{
					printk("%s[%d] VIC failed to config DVP mode!(10bits-sensor)\n",__func__,__LINE__);
					ret = -1;
				}
				break;
			case V4L2_MBUS_FMT_SBGGR12_1X12:
			case V4L2_MBUS_FMT_SGBRG12_1X12:
			case V4L2_MBUS_FMT_SGRBG12_1X12:
			case V4L2_MBUS_FMT_SRGGB12_1X12:
				input_cfg = DVP_RAW12;
				break;
			case V4L2_MBUS_FMT_RGB565_2X8_LE:
				input_cfg = DVP_RGB565_16BIT;
				break;
			case V4L2_MBUS_FMT_BGR565_2X8_LE:
				input_cfg = DVP_BRG565_16BIT;
				break;
			case V4L2_MBUS_FMT_UYVY8_1_5X8 :
			case V4L2_MBUS_FMT_VYUY8_1_5X8 :
			case V4L2_MBUS_FMT_YUYV8_1_5X8 :
			case V4L2_MBUS_FMT_YVYU8_1_5X8 :
			case V4L2_MBUS_FMT_YUYV8_1X16 :
				input_cfg = DVP_YUV422_8BIT;
				break;

		}
		if(vin->attr->dvp.mode == SENSOR_DVP_SONY_MODE)
			input_cfg |= DVP_SONY_MODE;
		tx_isp_vic_writel(vsd,VIC_DB_CFG, input_cfg);

		if(vin->attr->dvp.blanking.hblanking)
			tx_isp_vic_writel(vsd, VIC_INPUT_HSYNC_BLANKING, vin->mbus.width + (vin->attr->dvp.blanking.hblanking << 16));
		if(vin->attr->dvp.blanking.vblanking)
			tx_isp_vic_writel(vsd, VIC_INPUT_VSYNC_BLANKING, vin->attr->dvp.blanking.vblanking);

		ret = (vin->mbus.width<< 16) | (vin->mbus.height);
		tx_isp_vic_writel(vsd,VIC_RESOLUTION, ret);
		/*printk(" hblanking = %d vblanking = %d \n", vin->attr->dvp.blanking.hblanking, vin->attr->dvp.blanking.vblanking);*/
		/*printk("************RESOLUTION : %08x***************\n", tx_isp_vic_readl(vsd,VIC_RESOLUTION));*/
		tx_isp_vic_writel(vsd,VIC_GLOBAL_CFG, ISP_PRESET_MODE1);
		tx_isp_vic_writel(vsd,VIC_CONTROL, REG_ENABLE);
		tx_isp_vic_writel(vsd, VIC_CONTROL, VIC_SRART);
		/*dump_vic_reg(dump_vsd);*/
	}else{
		tx_isp_vic_writel(vsd,VIC_DB_CFG, DVP_RAW12);//RAW12
#if VIC_SUPPORT_MIPI
		tx_isp_vic_writel(vsd,VIC_INTF_TYPE, INTF_TYPE_DVP);//DVP
#endif
		ret = (vin->mbus.width<< 16) | (vin->mbus.height);
		tx_isp_vic_writel(vsd,VIC_RESOLUTION, ret);
		tx_isp_vic_writel(vsd,VIC_GLOBAL_CFG, ISP_PRESET_MODE1);
		tx_isp_vic_writel(vsd,CB_MODE0, 0x80008000);
		tx_isp_vic_writel(vsd, VIC_CONTROL, VIC_SRART);
	};
	return ret;
}

static int vic_core_reset(struct v4l2_subdev *sd, u32 val)
{
/*reset*/
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}
static long vic_core_ops_private_ioctl(struct tx_isp_vic_driver *vsd, struct isp_private_ioctl *ctl)
{
	long ret = ISP_SUCCESS;
	switch(ctl->cmd){
		case TX_ISP_PRIVATE_IOCTL_MODULE_CLK:
			ret = isp_vic_set_clk(vsd, ctl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
			tx_isp_vic_writel(vsd, VIC_CONTROL, VIC_RESET);
//			memcpy(&vsd->vin, (void *)(ctl->value), sizeof(struct tx_isp_video_in));
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
			ret = tx_isp_vic_start(vsd);
			break;
		case TX_ISP_PRIVATE_IOCTL_SYNC_VIDEO_IN:
			if(ctl->value)
				memcpy(&vsd->vin, (void *)(ctl->value), sizeof(struct tx_isp_video_in));
			else
				memset(&vsd->vin, 0, sizeof(struct tx_isp_video_in));

			break;
		default:
			break;
	}
	return ret;
}
static long vic_core_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_vic_driver *vsd = sd_to_tx_isp_vic_driver(sd);
	long ret = ISP_SUCCESS;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = vic_core_ops_private_ioctl(vsd, arg);
			break;
		default:
			break;
	}
	return ret;
}

static void vic_interrupts_enable(struct v4l2_subdev *sd)
{
	struct tx_isp_notify_argument arg;
	/* vic frd interrupt */
	arg.value = 0x10000;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_ENABLE_IRQ, &arg);
}

static int vic_core_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct tx_isp_vic_driver *vsd = sd_to_tx_isp_vic_driver(sd);
	int ret = 0;
	if (enable) {
		ret = tx_isp_vic_start(vsd);
		/*printk("vic------------start\n");*/
	}else {
		tx_isp_vic_writel(vsd, VIC_CONTROL, GLB_SAFE_RST);
	//	dump_vic_reg(vsd);
		/*printk("vic--------------stop\n");*/
	}
	return ret;
}

static int isp_vic_interrupt_service_routine(struct v4l2_subdev *sd, u32 status, bool *handled)
{
	struct tx_isp_vic_driver *vd = sd_to_tx_isp_vic_driver(sd);
	/*vic frd interrupt */
	if (0x10000&status) {
		vd->vic_frd_c++;
	}
	return ISP_SUCCESS;
}

static int vic_core_ops_init(struct v4l2_subdev *sd, u32 on)
{
	/* enable vic interrups */
	vic_interrupts_enable(sd);
	return ISP_SUCCESS;
}

static const struct v4l2_subdev_core_ops vic_core_ops = {
	.init = vic_core_ops_init,
	.reset = vic_core_reset,
	.ioctl = vic_core_ops_ioctl,
	.interrupt_service_routine = isp_vic_interrupt_service_routine,
};

static const struct v4l2_subdev_video_ops vic_video_ops = {
	.s_stream = vic_core_s_stream,
};

static const struct v4l2_subdev_ops vic_subdev_ops = {
	.core = &vic_core_ops,
	.video = &vic_video_ops,
};


static int vic_link_setup(struct media_entity *entity,
			  const struct media_pad *local,
			  const struct media_pad *remote, u32 flags)
{
	return ISP_SUCCESS;
}
/* media operations */
static const struct media_entity_operations vic_media_ops = {
	.link_setup = vic_link_setup,
};

/* vic frd info */
static int isp_vic_frd_show(struct seq_file *m, void *v)
{
	int len = 0;
	struct tx_isp_vic_driver *vd = (struct tx_isp_vic_driver*)(m->private);
	len += seq_printf(m ," %d\n", vd->vic_frd_c);
	return len;
}
static int dump_isp_vic_frd_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, isp_vic_frd_show, PDE_DATA(inode),8192);
}
static const struct file_operations isp_vic_frd_fops ={
	.read = seq_read,
	.open = dump_isp_vic_frd_open,
	.llseek = seq_lseek,
	.release = single_release,
};

int register_tx_isp_vic_device(struct platform_device *pdev, struct v4l2_device *v4l2_dev)
{
	struct tx_isp_subdev_platform_data *pdata = pdev->dev.platform_data;
	struct tx_isp_vic_driver *vsd = NULL;
	struct resource *res = NULL;
	struct v4l2_subdev *sd = NULL;
	struct media_pad *pads = NULL;
	struct media_entity *me = NULL;
	struct proc_dir_entry *proc;
	int ret;

	if(!pdata){
		v4l2_err(v4l2_dev, "The platform_data of csi is NULL!\n");
		ret = -ISP_ERROR;
		goto exit;
	};
	vsd = (struct tx_isp_vic_driver *)kzalloc(sizeof(*vsd), GFP_KERNEL);
	if(!vsd){
		v4l2_err(v4l2_dev, "Failed to allocate sensor device\n");
		ret = -ISP_ERROR;
		goto exit;
	}
	vsd->pdata = pdata;
	vsd->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res){
		res = request_mem_region(res->start,
				res->end - res->start + 1, dev_name(&pdev->dev));
		if (!res) {
			v4l2_err(v4l2_dev, "Not enough memory for resources\n");
			ret = -EBUSY;
			goto mem_region_failed;
		}
		vsd->base = ioremap(res->start, res->end - res->start + 1);
		if (!vsd->base) {
			v4l2_err(v4l2_dev, "Unable to ioremap registers!\n");
			ret = -ENXIO;
			goto ioremap_failed;
		}
	}
	vsd->res = res;
	sd = &vsd->sd;
	pads = vsd->pads;
	me = &sd->entity;

	v4l2_subdev_init(sd, &vic_subdev_ops);
	strlcpy(sd->name, "TX-ISP-VIC-SUBDEV ", sizeof(sd->name));

	sd->grp_id = pdata->grp_id ;	/* group ID for isp subdevs */
	v4l2_set_subdevdata(sd, vsd);

	pads[TX_ISP_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	pads[TX_ISP_PAD_LINK].flags = MEDIA_PAD_FL_SINK;

	me->ops = &vic_media_ops;
//	me->parent = v4l2_dev->mdev;
	ret = media_entity_init(me, TX_ISP_PADS_NUM, pads, 0);
	if (ret < 0){
		v4l2_err(v4l2_dev, "Failed to init media entity!\n");
		ret = -ISP_ERROR;
		goto entity_init_failed;
	}
	ret = v4l2_device_register_subdev(v4l2_dev, sd);
	if (ret < 0){
		v4l2_err(v4l2_dev, "Failed to register vic-subdev!\n");
		ret = -ISP_ERROR;
		goto register_failed;
	}

	ret = isp_vic_init_clk(vsd);
	if(ret < 0){
		v4l2_err(v4l2_dev, "Failed to init isp's clks!\n");
		ret = -ISP_ERROR;
	}
	dump_vsd=vsd;

	/* creat the node of printing isp info */
	proc = jz_proc_mkdir("vic");
	if (!proc) {
		vsd->proc_vic = NULL;
		v4l2_err(v4l2_dev, "create dev_attr_isp_info failed!\n");
	} else {
		vsd->proc_vic = proc;
	}
	proc_create_data("isp_vic_frd", S_IRUGO, proc, &isp_vic_frd_fops, (void *)vsd);

	return ISP_SUCCESS;
register_failed:
	media_entity_cleanup(me);
entity_init_failed:
	if(vsd->base)
		iounmap(vsd->base);
ioremap_failed:
	if(res)
		release_mem_region(res->start,res->end - res->start + 1);
mem_region_failed:
	kfree(vsd);
exit:
	return ret;
}

void release_tx_isp_vic_device(struct v4l2_subdev *sd)
{
	struct tx_isp_vic_driver *vsd = v4l2_get_subdevdata(sd);
	struct resource *res = vsd->res;
	isp_vic_release_clk(vsd);
	media_entity_cleanup(&sd->entity);

	v4l2_device_unregister_subdev(sd);

	iounmap(vsd->base);
	release_mem_region(res->start,res->end - res->start + 1);
	if (vsd->proc_vic) {
		proc_remove(vsd->proc_vic);
	}
	kfree(vsd);
}
