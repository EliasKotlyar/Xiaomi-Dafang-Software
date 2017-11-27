#include <linux/delay.h>
#include <media/v4l2-common.h>
#include <linux/v4l2-mediabus.h>
#include <linux/mm.h>

#include "tx-isp-csi.h"
#include "tx-isp-debug.h"

//#ifdef CONFIG_PRODUCT_FPGA
#if TX_ISP_EXIST_CSI_DEVICE
static int isp_csi_init_clk(struct tx_isp_csi_driver *csd)
{
	struct clk **clks = NULL;
	struct v4l2_device *v4l2_dev = csd->sd.v4l2_dev;
	struct tx_isp_subdev_platform_data *pdata = csd->pdata;
	struct tx_isp_subdev_clk_info *info = pdata->clks;
	unsigned int num = pdata->clk_num;
	int ret = 0;
	int i;

	csd->clk_num = pdata->clk_num;
	if(!csd->clk_num){
		csd->clks = NULL;
		return ISP_SUCCESS;
	}

	clks = (struct clk **)kzalloc(sizeof(clks) * num, GFP_KERNEL);
	if(!clks){
		v4l2_err(v4l2_dev, "%s[%d] Failed to allocate core's clks\n",__func__,__LINE__);
		ret = -ENOMEM;
		goto exit;
	}
	for (i = 0; i < num; i++) {
		clks[i] = clk_get(csd->dev, info[i].name);
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
	csd->clks = clks;
	return ISP_SUCCESS;
failed_to_get_clk:
	while(--i >= 0){
		clk_put(clks[i]);
	}
	kfree(clks);
exit:
	return ret;
}

static long isp_csi_set_clk(struct tx_isp_csi_driver *csd, int state)
{
	struct clk **clks = csd->clks;
	int i;

	if(state){
		for(i = 0; i < csd->clk_num; i++)
			clk_enable(clks[i]);
	}else{
		for(i = csd->clk_num - 1; i >=0; i--)
			clk_disable(clks[i]);
	}
	return ISP_SUCCESS;
}


static int isp_csi_release_clk(struct tx_isp_csi_driver *csd)
{
	struct clk **clks = csd->clks;
	int i = 0;

	if(clks == NULL)
		return 0;
	for (i = 0; i < csd->clk_num; i++)
		clk_put(clks[i]);

	kfree(clks);
	return 0;
}
#else
static int isp_csi_init_clk(struct tx_isp_csi_driver *csd)
{
	return 0;
}
static long isp_csi_set_clk(struct tx_isp_csi_driver *csd, int state)
{
	return 0;
}
static int isp_csi_release_clk(struct tx_isp_csi_driver *csd)
{
	return 0;
}
#endif

void dump_csi_reg(struct tx_isp_csi_driver *csd)
{
	ISP_PRINT(ISP_INFO_LEVEL,"****>>>>> dump csi reg <<<<<******\n");
	ISP_PRINT(ISP_INFO_LEVEL,"**********VERSION =%08x\n", csi_core_read(csd, VERSION));
	ISP_PRINT(ISP_INFO_LEVEL,"**********N_LANES =%08x\n", csi_core_read(csd, N_LANES));
	ISP_PRINT(ISP_INFO_LEVEL,"**********PHY_SHUTDOWNZ = %08x\n", csi_core_read(csd, PHY_SHUTDOWNZ));
	ISP_PRINT(ISP_INFO_LEVEL,"**********DPHY_RSTZ = %08x\n", csi_core_read(csd, DPHY_RSTZ));
	ISP_PRINT(ISP_INFO_LEVEL,"**********CSI2_RESETN =%08x\n", csi_core_read(csd, CSI2_RESETN));
	ISP_PRINT(ISP_INFO_LEVEL,"**********PHY_STATE = %08x\n", csi_core_read(csd, PHY_STATE));
	ISP_PRINT(ISP_INFO_LEVEL,"**********DATA_IDS_1 = %08x\n", csi_core_read(csd, DATA_IDS_1));
	ISP_PRINT(ISP_INFO_LEVEL,"**********DATA_IDS_2 = %08x\n", csi_core_read(csd, DATA_IDS_2));
	ISP_PRINT(ISP_INFO_LEVEL,"**********ERR1 = %08x\n", csi_core_read(csd, ERR1));
	ISP_PRINT(ISP_INFO_LEVEL,"**********ERR2 = %08x\n", csi_core_read(csd, ERR2));
	ISP_PRINT(ISP_INFO_LEVEL,"**********MASK1 =%08x\n", csi_core_read(csd, MASK1));
	ISP_PRINT(ISP_INFO_LEVEL,"**********MASK2 =%08x\n", csi_core_read(csd, MASK2));
	ISP_PRINT(ISP_INFO_LEVEL,"**********PHY_TST_CTRL0 = %08x\n", csi_core_read(csd, PHY_TST_CTRL0));
	ISP_PRINT(ISP_INFO_LEVEL,"**********PHY_TST_CTRL1 = %08x\n", csi_core_read(csd, PHY_TST_CTRL1));
}
static struct tx_isp_csi_driver *dump_csd = NULL;
void check_csi_error(void) {

	unsigned int temp1, temp2;
	while(1){
		dump_csi_reg(dump_csd);
		temp1 = csi_core_read(dump_csd,ERR1);
		temp2 = csi_core_read(dump_csd,ERR2);
		if(temp1 != 0)
			ISP_PRINT(ISP_INFO_LEVEL,"error-------- 1:0x%08x\n", temp1);
		if(temp2 != 0)
			ISP_PRINT(ISP_INFO_LEVEL,"error-------- 2:0x%08x\n", temp2);
	}
}

static unsigned char csi_core_write_part(struct tx_isp_csi_driver *csd,unsigned int address,
						unsigned int data, unsigned char shift, unsigned char width)
{
        unsigned int mask = (1 << width) - 1;
        unsigned int temp = csi_core_read(csd,address);
        temp &= ~(mask << shift);
        temp |= (data & mask) << shift;
        csi_core_write(csd,address, temp);

	return 0;
}

static unsigned char  csi_event_disable(struct tx_isp_csi_driver *csd,unsigned int  mask, unsigned char err_reg_no)
{
	switch (err_reg_no)
	{
		case 1:
			csi_core_write(csd,MASK1, mask | csi_core_read(csd, MASK1));
			break;
		case 2:
			csi_core_write(csd,MASK2, mask | csi_core_read(csd, MASK2));
			break;
		default:
			return ERR_OUT_OF_BOUND;
	}

	return 0;
}

unsigned char csi_set_on_lanes(struct tx_isp_csi_driver *csd,unsigned char lanes)
{
	ISP_PRINT(ISP_INFO_LEVEL,"%s:----------> lane num: %d\n", __func__, lanes);
	return csi_core_write_part(csd,N_LANES, (lanes - 1), 0, 2);
}

static void mipi_csih_dphy_test_data_in(struct tx_isp_csi_driver *csd,unsigned char test_data)
{
        csi_core_write(csd,PHY_TST_CTRL1, test_data);
}

static void mipi_csih_dphy_test_en(struct tx_isp_csi_driver *csd,unsigned char on_falling_edge)
{
        csi_core_write_part(csd,PHY_TST_CTRL1, on_falling_edge, 16, 1);
}

static void mipi_csih_dphy_test_clock(struct tx_isp_csi_driver *csd,int value)
{
        csi_core_write_part(csd,PHY_TST_CTRL0, value, 1, 1);
}

static void mipi_csih_dphy_test_data_out(void)
{
	//ISP_PRINT(ISP_INFO_LEVEL,"%s --------:%08x\n", __func__, csi_core_read(PHY_TST_CTRL1));
}

static void mipi_csih_dphy_write(struct tx_isp_csi_driver *csd,unsigned char address, unsigned char * data, unsigned char data_length)
{
        unsigned i = 0;
        if (data != 0)
        {
                /* set the TESTCLK input high in preparation to latch in the desired test mode */
                mipi_csih_dphy_test_clock(csd,1);
                /* set the desired test code in the input 8-bit bus TESTDIN[7:0] */
                mipi_csih_dphy_test_data_in(csd,address);
                /* set TESTEN input high  */
                mipi_csih_dphy_test_en(csd,1);
		//mdelay(1);
                /* drive the TESTCLK input low; the falling edge captures the chosen test code into the transceiver */
                mipi_csih_dphy_test_clock(csd,0);
		//mdelay(1);
                /* set TESTEN input low to disable further test mode code latching  */
                mipi_csih_dphy_test_en(csd,0);
                /* start writing MSB first */
                for (i = data_length; i > 0; i--)
                {       /* set TESTDIN[7:0] to the desired test data appropriate to the chosen test mode */
                        mipi_csih_dphy_test_data_in(csd,data[i - 1]);
		//	mdelay(1);
                        /* pulse TESTCLK high to capture this test data into the macrocell; repeat these two steps as necessary */
                        mipi_csih_dphy_test_clock(csd,1);
		//	mdelay(1);
                        mipi_csih_dphy_test_clock(csd,0);
                }
        }
	mipi_csih_dphy_test_data_out();
}
#if 0
static void csi_phy_configure(struct tx_isp_csi_driver *csd)
{
	unsigned char data[4];

	csi_core_write_part(csd,PHY_TST_CTRL0, 0, 0, 1);
//	udelay(5);
        csi_core_write(csd,PHY_TST_CTRL1, 0);
	csi_core_write_part(csd,PHY_TST_CTRL0, 0, 16, 1);

//	mipi_csih_dphy_test_data_in(0);
//	mipi_csih_dphy_test_en(0);
//	mipi_csih_dphy_test_clock(0);
        csi_core_write_part(csd,PHY_TST_CTRL0, 0, 1, 1);
	data[0]=0x00;
	//data[0]=0x06; /*448Mbps*/
//data[0]=0x13;
	mipi_csih_dphy_write(csd,0x44,data, 1);

	data[0]=0x1e;
	mipi_csih_dphy_write(csd,0xb0,data, 1);

	data[0]=0x1;
	mipi_csih_dphy_write(csd,0xb1,data, 1);

}
#endif
static int csi_phy_ready(struct tx_isp_csi_driver *csd)
{
	int ready;
	int ret;
	// TODO: phy0: lane0 is ready. need to be update for other lane
	ready = csi_core_read(csd,PHY_STATE);

#if 1
	ISP_PRINT(ISP_INFO_LEVEL,"%s:phy state ready:0x%08x\n", __func__, ready);
#endif
	ret = ready & (1 << 10 );
	switch(csd->vin.attr->mipi.lans){
	case 2:
		ret = ret && (ready & (1<<5));
	case 1:
		ret = ret && (ready & (1<<4));
		break;
	default:
		printk("Do not support lane num more than 2!\n");
	}

	return !!ret;
}

#if 0
static int csi_phy_init(void)
{
	//ISP_PRINT(ISP_INFO_LEVEL,"csi_phy_init being called ....\n");
	return 0;
}
#endif
static int csi_phy_release(struct tx_isp_csi_driver *csd)
{
	csi_core_write_part(csd,CSI2_RESETN, 0, 0, 1);
	csi_core_write_part(csd,CSI2_RESETN, 1, 0, 1);
	return 0;
}

static long csi_phy_start(struct tx_isp_csi_driver *csd)
{
	long ret = ISP_SUCCESS;
#if 1
	unsigned int i;
	int retries = 30;
	unsigned char data[4];
	csi_set_on_lanes(csd, csd->vin.attr->mipi.lans);
	/*reset phy*/
	csi_core_write_part(csd,PHY_SHUTDOWNZ, 0, 0, 1);
	csi_core_write_part(csd,DPHY_RSTZ, 0, 0, 1);
	csi_core_write_part(csd,CSI2_RESETN, 0, 0, 1);
/*csi_phy_configure*/
	udelay(5);
	csi_core_write_part(csd,PHY_TST_CTRL0, 0, 0, 1);
        csi_core_write(csd,PHY_TST_CTRL1, 0);
	csi_core_write_part(csd,PHY_TST_CTRL1, 0, 16, 1);
        csi_core_write_part(csd,PHY_TST_CTRL0, 0, 1, 1);
	data[0]=0x00;
	//data[0]=0x06; /*448Mbps*/
//data[0]=0x13;
	mipi_csih_dphy_write(csd,0x44,data, 1);

	data[0]=0x1e;
	mipi_csih_dphy_write(csd,0xb0,data, 1);

	data[0]=0x1;
	mipi_csih_dphy_write(csd,0xb1,data, 1);
	mdelay(10);
	csi_core_write_part(csd,PHY_SHUTDOWNZ, 1, 0, 1);
	//udelay(10);
	mdelay(10);
	csi_core_write_part(csd,DPHY_RSTZ, 1, 0, 1);
	mdelay(10);
	//udelay(10);
	csi_core_write_part(csd,CSI2_RESETN, 1, 0, 1);

	csi_event_disable(csd,0xffffffff, 1);
	csi_event_disable(csd,0xffffffff, 2);

	/* wait for phy ready */
	for (i = 0; i < retries; i++) {
		if (csi_phy_ready(csd))
			break;
		udelay(2);
	}
//	while(1){
//	dump_csi_reg(csd);
//	}
	if(i >= retries){
		ret = -ISP_ERROR;
		printk("csi init failure!");
	}
#endif
	return ret;
}
int csi_phy_stop(struct tx_isp_csi_driver *csd)
{

	ISP_PRINT(ISP_INFO_LEVEL,"csi_phy_stop being called \n");
	/*reset phy*/
	csi_core_write_part(csd,PHY_SHUTDOWNZ, 0, 0, 1);
	csi_core_write_part(csd,DPHY_RSTZ, 0, 0, 1);
	csi_core_write_part(csd,CSI2_RESETN, 0, 0, 1);

	return 0;
}

static long isp_csi_ops_private_ioctl(struct tx_isp_csi_driver *csd, struct isp_private_ioctl *ctl)
{
	long ret = ISP_SUCCESS;
	switch(ctl->cmd){
		case TX_ISP_PRIVATE_IOCTL_MODULE_CLK:
			ret = isp_csi_set_clk(csd, ctl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
			if(csd->vin.attr->dbus_type == TX_SENSOR_DATA_INTERFACE_MIPI){
	//			memcpy(&csd->vin, (void *)(ctl->value), sizeof(struct tx_isp_video_in));
				ret = csi_phy_stop(csd);
			}
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
			if(csd->vin.attr->dbus_type == TX_SENSOR_DATA_INTERFACE_MIPI){
				ret = csi_phy_start(csd);
			}
			break;
		case TX_ISP_PRIVATE_IOCTL_SYNC_VIDEO_IN:
			if(ctl->value)
				memcpy(&csd->vin, (void *)(ctl->value), sizeof(struct tx_isp_video_in));
			else
				memset(&csd->vin, 0, sizeof(struct tx_isp_video_in));
			break;
		case TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_ENUM_FMT:
			//ret = isp_core_enum_fmt(core, ctl->value);
			break;
		default:
			break;
	}
	return ret;
}
static long isp_csi_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_csi_driver *csd = sd_to_tx_isp_csi_driver(sd);
	long ret = ISP_SUCCESS;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = isp_csi_ops_private_ioctl(csd, arg);
			break;
		default:
			break;
	}
	return ret;
}
static int csi_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int csi_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
/* enable clk*/
//	struct tx_isp_video_in *vic_attr = (struct tx_isp_video_in *)ctrl.value;
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int csi_init(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}
static int csi_s_power(struct v4l2_subdev *sd, int on)
{
	/*printk("functiong:%s,line:%d\n", __func__, __LINE__);*/
	return 0;
}

static int csi_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct tx_isp_csi_driver *csd = sd_to_tx_isp_csi_driver(sd);
	int ret = ISP_SUCCESS;
	if(csd->vin.attr->dbus_type == TX_SENSOR_DATA_INTERFACE_MIPI){
		if (enable) {
			ret = csi_phy_start(csd);//sd--lanes ???????
			/*printk("csi------------start\n");*/
		}
		else {
			ret = csi_phy_stop(csd);
			/*printk("csi--------------stop\n");*/
		}
	}else{
		/*printk("the sensor is not mipi bus\n");*/
	}
	return ret;
}

static const struct v4l2_subdev_internal_ops csi_internal_ops = {
};

static const struct v4l2_subdev_core_ops csi_core_ops = {
//	.g_chip_ident = csi_g_chip_ident,
	.g_ctrl = csi_g_ctrl,
	.s_ctrl = csi_s_ctrl,
//	.queryctrl = csi_queryctrl,
//	.reset = csi_reset,
	.init = csi_init,
	.s_power = csi_s_power,
	.ioctl = isp_csi_ops_ioctl,
/* #ifdef CONFIG_VIDEO_ADV_DEBUG */
/* 	.g_register = csi_g_register, */
/* 	.s_register = csi_s_register, */
/* #endif */
};

static const struct v4l2_subdev_video_ops csi_video_ops = {
	.s_stream = csi_s_stream,
};

static const struct v4l2_subdev_ops csi_subdev_ops = {
	.core = &csi_core_ops,
	.video = &csi_video_ops,
};

/* media operations */
static const struct media_entity_operations csi_media_ops = {
//	.link_setup = csi_link_setup,
};

int register_tx_isp_csi_device(struct platform_device *pdev, struct v4l2_device *v4l2_dev)
{
	struct tx_isp_subdev_platform_data *pdata = pdev->dev.platform_data;
	struct tx_isp_csi_driver *csd = NULL;
	struct resource *res = NULL;
	struct v4l2_subdev *sd = NULL;
	struct media_pad *pads = NULL;
	struct media_entity *me = NULL;
	int ret = ISP_SUCCESS;

	if(!pdata){
		v4l2_err(v4l2_dev, "The platform_data of csi is NULL!\n");
		ret = -ISP_ERROR;
		goto exit;
	};
	csd = (struct tx_isp_csi_driver *)kzalloc(sizeof(*csd), GFP_KERNEL);
	if(!csd){
		v4l2_err(v4l2_dev, "Failed to allocate sensor device\n");
		ret = -ISP_ERROR;
		goto exit;
	}
	csd->pdata = pdata;
	csd->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	res = request_mem_region(res->start,
			res->end - res->start + 1, dev_name(&pdev->dev));
	if (!res) {
		v4l2_err(v4l2_dev, "Not enough memory for resources\n");
		ret = -EBUSY;
		goto mem_region_failed;
	}
	csd->base = ioremap(res->start, res->end - res->start + 1);
	if (!csd->base) {
		v4l2_err(v4l2_dev, "Unable to ioremap registers!\n");
		ret = -ENXIO;
		goto ioremap_failed;
	}
	csd->res = res;
	sd = &csd->sd;
	pads = csd->pads;
	me = &sd->entity;

	v4l2_subdev_init(sd, &csi_subdev_ops);
	sd->internal_ops = &csi_internal_ops;
	strlcpy(sd->name, "TX-ISP-CSI-SUBDEV ", sizeof(sd->name));

	sd->grp_id = pdata->grp_id ;	/* group ID for isp subdevs */
	v4l2_set_subdevdata(sd, csd);

	pads[TX_ISP_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	pads[TX_ISP_PAD_LINK].flags = MEDIA_PAD_FL_SINK;

	me->ops = &csi_media_ops;
//	me->parent = v4l2_dev->mdev;
	ret = media_entity_init(me, TX_ISP_PADS_NUM, pads, 0);
	if (ret < 0){
		v4l2_err(v4l2_dev, "Failed to init media entity!\n");
		ret = -ISP_ERROR;
		goto entity_init_failed;
	}
	ret = v4l2_device_register_subdev(v4l2_dev, sd);
	if (ret < 0){
		v4l2_err(v4l2_dev, "Failed to register csi-subdev!\n");
		ret = -ISP_ERROR;
		goto register_failed;
	}

	ret = isp_csi_init_clk(csd);
	if(ret < 0){
		v4l2_err(v4l2_dev, "Failed to init isp's clks!\n");
		ret = -ISP_ERROR;
	}
	dump_csd = csd;
	return ISP_SUCCESS;
register_failed:
	media_entity_cleanup(me);
entity_init_failed:
	iounmap(csd->base);
ioremap_failed:
	release_mem_region(res->start, res->end - res->start + 1);
mem_region_failed:
	kfree(csd);
exit:
	return ret;
}

void release_tx_isp_csi_device(struct v4l2_subdev *sd)
{
	struct tx_isp_csi_driver *csd = v4l2_get_subdevdata(sd);
	struct resource *res = csd->res;
	csi_phy_release(csd);
	isp_csi_release_clk(csd);
	media_entity_cleanup(&sd->entity);

	v4l2_device_unregister_subdev(sd);

	iounmap(csd->base);
	release_mem_region(res->start,res->end - res->start + 1);
	kfree(csd);
}
