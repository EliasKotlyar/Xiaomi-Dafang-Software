#include <linux/videodev2.h>
#include <linux/delay.h>
#include "apical_command_api.h"
#include <apical-isp/apical_isp_config.h>
#include <apical-isp/apical_math.h>
#include "tx-isp-core-tuning.h"
#include "../tx-isp-debug.h"

/** the kernel command line whether the mem of WDR and Temper exist. **/
extern unsigned long ispmem_base;
extern unsigned long ispmem_size;
extern system_tab stab ;

static inline int wb_value_v4l2_to_apical(int val)
{
	int ret = 0;
	switch(val){
		case V4L2_WHITE_BALANCE_MANUAL:
			ret = AWB_MANUAL;
			break;
		case V4L2_WHITE_BALANCE_AUTO:
			ret = AWB_AUTO;
			break;
		case V4L2_WHITE_BALANCE_INCANDESCENT:
			ret = AWB_INCANDESCENT;
			break;
		case V4L2_WHITE_BALANCE_FLUORESCENT:
			ret = AWB_FLOURESCENT;
			break;
		case V4L2_WHITE_BALANCE_FLUORESCENT_H:
			ret = AWB_WARM_FLOURESCENT;
			break;
		case V4L2_WHITE_BALANCE_HORIZON:
			ret = AWB_TWILIGHT;
			break;
		case V4L2_WHITE_BALANCE_DAYLIGHT:
			ret = AWB_DAY_LIGHT;
			break;
		case V4L2_WHITE_BALANCE_CLOUDY:
			ret = AWB_CLOUDY;
			break;
		case V4L2_WHITE_BALANCE_SHADE:
			ret = AWB_SHADE;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static int apical_isp_wb_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *wb_mode = tuning->ctrls.wb_mode;
	apical_api_control_t api;
	struct isp_core_awb_attr *attr = &tuning->ctrls.awb_attr;
	struct isp_core_mwb_attr *mattr = &tuning->ctrls.mwb_attr;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	api.type = TALGORITHMS;
	api.dir = COMMAND_SET;
	switch(control->id){
		case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
			api.id = AWB_MODE_ID;
			api.value = wb_value_v4l2_to_apical(wb_mode->val);
			status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			break;
		case IMAGE_TUNING_CID_AWB_ATTR:
			copy_from_user(attr, (const void __user *)control->value, sizeof(*attr));
			/* sets the lowest color temperature that the AWB algorithm can select */
			api.id = AWB_RANGE_LOW_ID;
			api.value = attr->low_color_temp / 100;
			status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			/* sets the highest color temperature that the AWB algorithm can select */
			api.id = AWB_RANGE_HIGH_ID;
			api.value = attr->high_color_temp / 100;
			status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			/*
			* select which zones are used to gather AWB statistics.
			* the region of interest is defined as rectangle with top-left coordinates(startx, starty)
			* and bottom-right coordinates(endx, endy).
			*/
			api.id = AWB_ROI_ID;
			api.value = attr->zone_sel.val;
			status = apical_command(api.type, api.id, api.value, api.dir, &reason);
#if 0
			/* config the weight of every zone  */
			for(i = 0; i < WEIGHT_ZONE_NUM; i++)
#endif
			break;
		case IMAGE_TUNING_CID_MWB_ATTR:
			copy_from_user(mattr, (const void __user *)control->value, sizeof(*mattr));
			if(wb_mode->cur.val == V4L2_WHITE_BALANCE_MANUAL){
				api.id = AWB_RGAIN_ID;
				api.value = mattr->red_gain;
				status = apical_command(api.type, api.id, api.value, api.dir, &reason);
				api.id = AWB_BGAIN_ID;
				api.value = mattr->blue_gain;
				status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			}
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static inline int ae_value_v4l2_to_apical(int val)
{
	int ret = 0;
	switch(val){
		case V4L2_EXPOSURE_AUTO:
			ret = AE_AUTO;
			break;
		case V4L2_EXPOSURE_MANUAL:
			ret = AE_FULL_MANUAL;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}
static int apical_isp_ae_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *exp_mode = tuning->ctrls.exp_mode;
	struct v4l2_ctrl *ctrl = NULL;
	apical_api_control_t api;
	struct isp_core_ae_attr *attr = &tuning->ctrls.ae_attr;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	api.type = TALGORITHMS;
	api.dir = COMMAND_SET;

	switch(control->id){
		case V4L2_CID_EXPOSURE_AUTO:
			if(exp_mode->is_new){
				api.id = AE_MODE_ID;
				api.value = ae_value_v4l2_to_apical(exp_mode->val);
				status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			}

			/* set absolut exposure */
			ctrl = tuning->ctrls.manual_exp;
			if(ctrl->is_new && exp_mode->cur.val == V4L2_EXPOSURE_MANUAL){
					api.id = AE_EXPOSURE_ID;
					api.value = ctrl->val;
					status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			}
			/* set exposure gain */
			ctrl = tuning->ctrls.exp_gain;
			if(ctrl->is_new){
				api.id = AE_GAIN_ID;
				api.value = ctrl->val;
				status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			}
			/* set exposure compensation */
			ctrl = tuning->ctrls.exp_compensation;
			if(ctrl->is_new){
				api.id = AE_COMPENSATION_ID;
				api.value = ctrl->val;
				status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			}
			break;
		case IMAGE_TUNING_CID_AE_ATTR:
			copy_from_user(attr, (const void __user *)control->value, sizeof(*attr));
			api.id = AE_ROI_ID;
			api.value = attr->zone_sel.val;
			status = apical_command(api.type, api.id, api.value, api.dir, &reason);
#if 0
			/* config the weight of every zone  */
			for(i = 0; i < WEIGHT_ZONE_NUM; i++)
#endif
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return 0;
}
static int apical_isp_ae_g_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	apical_api_control_t api;
	struct isp_core_ae_attr *attr = &tuning->ctrls.ae_attr;
	unsigned char status = 0;
	int reason = 0;

	api.type = TALGORITHMS;
	api.dir = COMMAND_GET;

	/* get exposure gain */
	api.id = AE_GAIN_ID;
	api.value = -1;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	attr->gain = api.value;
	/* get exposure compensation */
	api.id = AE_COMPENSATION_ID;
	api.value = -1;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	attr->comp = api.value;
	/* get exposure ROI */
	api.id = AE_ROI_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	attr->zone_sel.val = api.value;
	copy_to_user((void __user *)control->value, (const void *)attr, sizeof(*attr));
#if 0
	/* config the weight of every zone  */
	for(i = 0; i < WEIGHT_ZONE_NUM; i++)
#endif
	return 0;
}

/* the format of return value is 8.8 */
static inline int apical_isp_g_totalgain(struct tx_isp_core_device *core, struct v4l2_control *control)
{
//	struct video_device *video = core->tun;
//	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	/* input format 27.5, output format 24.8 */
	unsigned int total_gain;
	total_gain =  stab.global_sensor_analog_gain  + stab.global_sensor_digital_gain + stab.global_isp_digital_gain;

	total_gain = math_exp2(total_gain, 5, 8);

	copy_to_user((void __user *)control->value, (const void *)&total_gain, sizeof(unsigned int));
	return ISP_SUCCESS;
}
static inline int af_value_v4l2_to_apical(int val)
{
	return val + AF_AUTO_SINGLE;
}
static int apical_isp_af_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *af_mode = tuning->ctrls.af_mode;
	apical_api_control_t api;
	struct isp_core_af_attr *attr = &tuning->ctrls.af_attr;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	api.type = TALGORITHMS;
	api.dir = COMMAND_SET;
	switch(control->id){
		case IMAGE_TUNING_CID_CUSTOM_AF_MODE:
			api.id = AF_MODE_ID;
			api.value = af_value_v4l2_to_apical(af_mode->val);
			status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			break;
		case IMAGE_TUNING_CID_AF_ATTR:
			copy_from_user(attr, (const void __user *)control->value, sizeof(*attr));
			api.id = AF_RANGE_LOW_ID;
			api.value = attr->af_low_range;
			status = apical_command(api.type, api.id, api.value, api.dir, &reason);

			api.id = AF_RANGE_HIGH_ID;
			api.value = attr->af_high_range;
			status = apical_command(api.type, api.id, api.value, api.dir, &reason);

			api.id = AF_ROI_ID;
			api.value = attr->zone_sel.val;
			status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			apical_isp_metering_af_threshold_write_write(attr->af_threshold);
			apical_isp_metering_af_threshold_alt_write_write(attr->af_threshold_alt);
			apical_isp_metering_af_np_offset_write(attr->af_np_offset);
			apical_isp_metering_af_intensity_norm_mode_write(attr->af_intensity_mode);
			apical_isp_metering_af_metrics_shift_write(attr->af_metrics_shift);
			break;
		default:
			ret = -EPERM;
			break;
	}
	return ret;
}

static inline int apical_isp_3alock_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *aefwb = tuning->ctrls.aefwb_lock;
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	api.type = TALGORITHMS;
	api.dir = COMMAND_SET;
	api.id = AE_FREEZE_ID;
	if(aefwb->val & V4L2_LOCK_EXPOSURE)
		api.value = FREEZE;
	else
		api.value = UNFREEZE;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);

	api.id = AWB_FREEZE_ID;
	if(aefwb->val & V4L2_LOCK_WHITE_BALANCE)
		api.value = FREEZE;
	else
		api.value = UNFREEZE;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);

	return ret;
}

static inline int apical_isp_vflip_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	int ret = ISP_SUCCESS;
#if 1
	core->vflip_state = tuning->ctrls.vflip->val;
#else
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	api.type = TIMAGE;
	api.dir = COMMAND_SET;
	api.id = ORIENTATION_VFLIP_ID;
	api.value = tuning->ctrls.vflip->val ? ENABLE : DISABLE;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
#endif
	return ret;
}

static inline int apical_isp_hflip_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
//	struct apical_isp_contrl *contrl = &core->contrl;
	int ret = ISP_SUCCESS;
	core->hflip_state = tuning->ctrls.hflip->val;
#if 0
	unsigned char color = apical_isp_top_rggb_start_read();
	if(tuning->ctrls.hflip->val){
		color ^= 1;
	}else{
		if(contrl->pattern != color){
			color ^= 1;
		}
	}
//	printk("$$$$ %s %d pattern = %d color = %d $$$$$\n", __func__,__LINE__,contrl->pattern,color);
	apical_isp_top_rggb_start_write(color);
	apical_isp_top_bypass_mirror_write(tuning->ctrls.hflip->val ?0:1);
#endif
	return ret;
}

static int apical_isp_sinter_dns_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *sinter = tuning->ctrls.sinter;

	/* enable the module mean that disable bypass function */
	apical_isp_top_bypass_sinter_write(sinter->val?0:1);
	return ISP_SUCCESS;
}

static int apical_isp_sinter_dns_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_sinter_attr *attr = &(tuning->ctrls.sinter_attr);
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	copy_from_user(attr, (const void __user*)control->value, sizeof(*attr));

	api.type = TALGORITHMS;
	api.dir = COMMAND_SET;
	api.id = SINTER_MODE_ID;
	api.value = attr->type == ISPCORE_MODULE_AUTO ? AUTO : MANUAL;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);

	if(attr->type == ISPCORE_MODULE_MANUAL){
		api.id = SINTER_STRENGTH_ID;
		api.value = attr->manual_strength;
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	}
	if(status != ISP_SUCCESS)
		ret = -ISP_ERROR;
	return ret;
}
#if 0
static int apical_isp_temper_dns_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct apical_isp_contrl *contrl = &core->contrl;
	struct v4l2_ctrl *temper = tuning->ctrls.temper;

	if(tuning->temper_paddr == 0){
		return -EPERM;
	}
	printk("###### %s %d #######\n",__func__,__LINE__);
//	while(core->frame_state)
//		mdelay(1);
	printk("###### %s %d #######\n",__func__,__LINE__);
	if(temper->val == ISPCORE_MODULE_ENABLE){
		apical_isp_temper_frame_buffer_active_width_write(contrl->inwidth);
		apical_isp_temper_frame_buffer_active_height_write(contrl->inheight);
		apical_isp_temper_frame_buffer_line_offset_write(contrl->inwidth * 4);
		apical_isp_temper_frame_buffer_bank0_base_write(tuning->temper_paddr);
		apical_isp_temper_frame_buffer_bank1_base_write(tuning->temper_paddr);
	//	apical_isp_temper_frame_buffer_bank1_base_write(tuning->temper_paddr + (tuning->temper_buffer_size >> 1));
		apical_isp_temper_frame_buffer_frame_write_cancel_write(0);
		apical_isp_temper_frame_buffer_frame_read_cancel_write(0);
		apical_isp_temper_frame_buffer_frame_write_on_write(1);
		apical_isp_temper_frame_buffer_frame_read_on_write(1);
	}else{
		apical_isp_temper_frame_buffer_frame_write_cancel_write(1);
		apical_isp_temper_frame_buffer_frame_read_cancel_write(1);
		apical_isp_temper_frame_buffer_frame_write_on_write(0);
		apical_isp_temper_frame_buffer_frame_read_on_write(0);
	}
	/* enable the module mean that disable bypass function */
	apical_isp_temper_frame_buffer_axi_port_enable_write(temper->val);
	apical_isp_top_bypass_temper_write(temper->val?0:1);
	apical_isp_temper_enable_write(temper->val);
//	printk("##[%s %d] width = %d height = %d \n", __func__, __LINE__,APICAL_READ_32(0xa10),APICAL_READ_32(0xa14));
//	printk("##[%s %d] bank0 = 0x%08x bank1 = 0x%08x \n", __func__, __LINE__,APICAL_READ_32(0xa18),APICAL_READ_32(0xa1c));
//	printk("##[%s %d] axi port = 0x%08x temper enable = 0x%08x \n", __func__, __LINE__,APICAL_READ_32(0xa28),APICAL_READ_32(0x2c0));
//	printk("##[%s %d] 0xa00 = 0x%08x 0xa24 = 0x%08x \n", __func__, __LINE__,APICAL_READ_32(0xa00),APICAL_READ_32(0xa24));

	return ISP_SUCCESS;
}
#else
static int apical_isp_temper_dns_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct apical_isp_contrl *contrl = &core->contrl;
	struct v4l2_ctrl *temper = tuning->ctrls.temper;
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	api.type = TSYSTEM;
	api.dir = COMMAND_SET;
	api.id = SYSTEM_MANUAL_TEMPER;
	switch(temper->val){
		case ISPCORE_TEMPER_MODE_DISABLE:
			api.value = OFF;
			break;
		case ISPCORE_TEMPER_MODE_AUTO:
		case ISPCORE_TEMPER_MODE_MANUAL:
			if(tuning->temper_paddr == 0){
				return -EPERM;
			}
			apical_isp_temper_temper2_mode_write(1);
			apical_isp_temper_frame_buffer_active_width_write(contrl->inwidth);
			apical_isp_temper_frame_buffer_active_height_write(contrl->inheight);
			apical_isp_temper_frame_buffer_line_offset_write(contrl->inwidth * 4);
			api.value = temper->val == ISPCORE_TEMPER_MODE_AUTO ? 0 : 1;
			break;
		default:
			ret = -ISP_ERROR;
			break;
	}
	if(ret != ISP_SUCCESS)
		return ret;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if(status != ISP_SUCCESS){
		printk("##[%s %d] failed to temper set command(status = %d)\n", __func__, __LINE__,status);
		ret = -ISP_ERROR;
	}
	/* enable the module mean that disable bypass function */
	apical_isp_temper_frame_buffer_axi_port_enable_write(temper->val?1:0);
	apical_isp_top_bypass_temper_write(temper->val?0:1);
	apical_isp_temper_enable_write(temper->val?1:0);
//	printk("##[%s %d] width = %d height = %d \n", __func__, __LINE__,APICAL_READ_32(0xa10),APICAL_READ_32(0xa14));
//	printk("##[%s %d] bank0 = 0x%08x bank1 = 0x%08x \n", __func__, __LINE__,APICAL_READ_32(0xa18),APICAL_READ_32(0xa1c));
//	printk("##[%s %d] axi port = 0x%08x temper enable = 0x%08x \n", __func__, __LINE__,APICAL_READ_32(0xa28),APICAL_READ_32(0x2c0));
//	printk("##[%s %d] 0xa00 = 0x%08x 0xa24 = 0x%08x \n", __func__, __LINE__,APICAL_READ_32(0xa00),APICAL_READ_32(0xa24));

	return ISP_SUCCESS;
}
#endif

static int apical_isp_temper_dns_s_strength(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_temper_attr *attr = &(tuning->ctrls.temper_attr);
	struct v4l2_ctrl *temper = tuning->ctrls.temper;
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	attr->manual_strength = control->value;
	if(temper->val == ISPCORE_TEMPER_MODE_MANUAL){
		api.type = TSYSTEM;
		api.dir = COMMAND_SET;
		api.id = SYSTEM_TEMPER_THRESHOLD_TARGET;
		api.value = attr->manual_strength;
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	}
	if(status != ISP_SUCCESS)
		ret = -ISP_ERROR;
	return ret;
}

static int apical_isp_temper_dns_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_temper_attr *attr = &(tuning->ctrls.temper_attr);
	struct v4l2_ctrl *temper = tuning->ctrls.temper;
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	copy_from_user(attr, (const void __user*)control->value, sizeof(*attr));
	if(temper->val == ISPCORE_TEMPER_MODE_MANUAL){
		api.type = TALGORITHMS;
		api.dir = COMMAND_SET;
		api.id = TEMPER_STRENGTH_ID;
		api.value = attr->manual_strength;
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	}
	if(status != ISP_SUCCESS)
		ret = -ISP_ERROR;
	return ret;
}

static int apical_isp_temper_dns_g_strength(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	int ret = 0;
	return ret;
}

static int apical_isp_temper_dns_g_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_temper_attr *attr = &(tuning->ctrls.temper_attr);
	struct v4l2_ctrl *temper = tuning->ctrls.temper;
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	if(temper->val == ISPCORE_TEMPER_MODE_MANUAL){
		api.type = TALGORITHMS;
		api.dir = COMMAND_GET;
		api.id = TEMPER_STRENGTH_ID;
		api.value = -1;
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
		if(status != ISP_SUCCESS){
			printk("##[%s %d] failed to temper set command(status = %d)\n", __func__, __LINE__,status);
			ret = -ISP_ERROR;
		}else{
			attr->manual_strength = api.value;
			copy_to_user((void __user*)control->value, (const void *)attr, sizeof(*attr));
		}
	}
	return ret;
}

/* Iridix modules */
static inline int rawdrc_value_v4l2_to_apical(int val)
{
	int ret = 0;
	switch(val){
		case 0:
			ret = MANUAL;
			break;
		case 1:
			ret = UNLIMIT;
			break;
		case 2:
			ret = HIGH;
			break;
		case 3:
			ret = MEDIUM;
			break;
		case 4:
			ret = LOW;
			break;
		case 5:
			ret = OFF;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static int apical_isp_drc_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *drc = tuning->ctrls.raw_drc;
	struct isp_core_drc_attr *attr = &tuning->ctrls.drc_attr;
	ISP_CORE_MODE_DN_E dn = tuning->ctrls.daynight;
	LookupTable** table = NULL;
	TXispPrivParamManage *param = core->param;
	apical_api_control_t api;
	unsigned char status = ISP_SUCCESS;
	int reason = 0;
	int ret = ISP_SUCCESS;

	if(drc->is_new){
/*
		api.type = TALGORITHMS;
		api.dir = COMMAND_SET;
		api.id = IRIDIX_MODE_ID;
		api.value = rawdrc_value_v4l2_to_apical(drc->val);
	//	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
*/
		stab.global_manual_iridix = drc->val == ISPMODULE_DRC_MANUAL ? 1 : 0;

		if(param == NULL)
			goto set_drc_val;

		if(dn == ISP_CORE_RUNING_MODE_DAY_MODE)
			table = param->isp_param[TX_ISP_PRIV_PARAM_DAY_MODE].calibrations;
		else
			table = param->isp_param[TX_ISP_PRIV_PARAM_NIGHT_MODE].calibrations;
		if(drc->val == ISPMODULE_DRC_MANUAL){
			stab.global_minimum_iridix_strength = 0;
			stab.global_maximum_iridix_strength = 255;
		}else{
			api.type = TIMAGE;
			api.dir = COMMAND_GET;
			api.id = WDR_MODE_ID;
			api.value = -1;
			status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			if(status != ISP_SUCCESS) {
				ISP_PRINT(ISP_WARNING_LEVEL,"Get WDR mode failure!reture value is %d,reason is %d\n",status,reason);
			}

			if (reason == IMAGE_WDR_MODE_LINEAR) {
				stab.global_maximum_iridix_strength = *(uint8_t *)(table[_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_LINEAR]->ptr);
			} else if (reason == IMAGE_WDR_MODE_FS_HDR) {
				stab.global_maximum_iridix_strength = *(uint8_t *)(table[_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_WDR]->ptr);
			} else {
				printk("No this mode! Iridix set failure!\n");
			}
			stab.global_minimum_iridix_strength = *(uint8_t *)(table[_CALIBRATION_IRIDIX_MIN_MAX_STR]->ptr);
		}
set_drc_val:
		apical_isp_top_bypass_iridix_write((drc->val == 5)?1:0);
		attr->mode = drc->val;
	}
	return ret;
}

static inline int apical_isp_drc_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_drc_attr *attr = &tuning->ctrls.drc_attr;
	apical_api_control_t api;
	unsigned char status = ISP_SUCCESS;
	int reason = 0;

	copy_from_user(attr, (const void __user *)control->value, sizeof(*attr));
	api.type = TALGORITHMS;
	api.dir = COMMAND_SET;
	api.id = IRIDIX_STRENGTH_ID;
	api.value = attr->strength;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
#if 0
	apical_isp_iridix_strength_write(attr->strength);
	apical_isp_iridix_slope_max_write(attr->slope_max);
	apical_isp_iridix_slope_min_write(attr->slope_min);
	apical_isp_iridix_black_level_write(attr->black_level);
	apical_isp_iridix_white_level_write(attr->white_level);
#endif
	return ISP_SUCCESS;
}

static inline int apical_isp_drc_g_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_drc_attr *attr = &tuning->ctrls.drc_attr;

	attr->strength = apical_isp_iridix_strength_read();
	attr->slope_max = apical_isp_iridix_slope_max_read();
	attr->slope_min = apical_isp_iridix_slope_min_read();
	attr->black_level = apical_isp_iridix_black_level_read();
	attr->white_level = apical_isp_iridix_white_level_read();
	copy_to_user((void __user*)control->value, (const void *)attr, sizeof(*attr));

	return 0;
}

/* WDR module */
static int apical_isp_wdr_lut_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
//	struct video_device *video = core->tun;
//	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
//	struct v4l2_ctrl *ctrl = tuning->ctrls.wdr_lut;

	/* enable the module mean that disable bypass function */
//	apical_isp_top_bypass_gamma_fe_write(sinter->val?0:1);
//	APICAL_WRITE_32(0x188, );
	return ISP_SUCCESS;
}

static inline int apical_isp_noise_profile_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_noise_profile_attr *attr = &tuning->ctrls.np_attr;

	copy_from_user(attr, (const void __user *)control->value, sizeof(*attr));
	return ISP_SUCCESS;
}
static inline int apical_isp_wdr_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct apical_isp_contrl *contrl = &core->contrl;
	struct v4l2_ctrl *wdr = tuning->ctrls.wdr;
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	if(tuning->wdr_paddr == 0){
		return -EPERM;
	}

	api.type = TIMAGE;
	api.dir = COMMAND_SET;
	api.id = WDR_MODE_ID;
	if(wdr->val == ISPCORE_MODULE_DISABLE){
//		APICAL_WRITE_32(0x240, 0x000300b1); //disable frame buffers
//		APICAL_WRITE_32(0x188, 0x0);
		api.value = IMAGE_WDR_MODE_LINEAR;
	}else{
//		apical_isp_frame_stitch_frame_buffer_active_width_write(contrl->inwidth);
//		apical_isp_frame_stitch_frame_buffer_active_height_write(contrl->inheight);
		apical_isp_frame_stitch_frame_buffer_line_offset_write(contrl->inwidth * 4);
//		APICAL_WRITE_32(0x240, 0x00030041); //enable frame buffers
//		APICAL_WRITE_32(0x188, 0x3);
		api.value = IMAGE_WDR_MODE_FS_HDR;
	}
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	/* enable the moudle [WDR companded frontend lookup table] */
	apical_isp_top_bypass_gamma_fe_write(wdr->val?0:1);
	/* enable the module mean that disable bypass function */
	apical_isp_frame_stitch_frame_buffer_axi_port_enable_write(wdr->val?1:0);
	apical_isp_top_bypass_frame_stitch_write(wdr->val?0:1);
	/** if enable isp wdr, set 1; if enable sensor wdr, set 0 **/
	apical_isp_top_gamma_fe_position_write(wdr->val?1:0);
	apical_isp_input_port_field_mode_write(wdr->val?1:0);
	if(status != ISP_SUCCESS)
		ret = -ISP_ERROR;
	return ret;
}

static inline int apical_isp_wdr_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_wdr_attr *attr = &tuning->ctrls.wdr_attr;

	copy_from_user(attr, (const void __user *)control->value, sizeof(*attr));
	apical_isp_frame_stitch_short_thresh_write(attr->short_thresh);
	apical_isp_frame_stitch_long_thresh_write(attr->long_thresh);
	apical_isp_frame_stitch_exposure_ratio_write(attr->exp_ratio);
	apical_isp_frame_stitch_stitch_correct_write(attr->stitch_correct);
	apical_isp_frame_stitch_stitch_error_thresh_write(attr->stitch_err_thresh);
	apical_isp_frame_stitch_stitch_error_limit_write(attr->stitch_err_limit);
	apical_isp_frame_stitch_black_level_long_write(attr->black_level_long);
	apical_isp_frame_stitch_black_level_short_write(attr->black_level_short);
	apical_isp_frame_stitch_black_level_out_write(attr->black_level_out);
	return ISP_SUCCESS;
}


static int apical_isp_bypass_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *isp_process = tuning->ctrls.isp_process;
	struct v4l2_mbus_framefmt *mbus = &core->vin.mbus;

	if(isp_process->val == ISPCORE_MODULE_ENABLE){
		switch(mbus->code){
		case V4L2_MBUS_FMT_RGB565_2X8_LE:
		case V4L2_MBUS_FMT_RGB888_3X8_LE:
			apical_isp_top_isp_processing_bypass_mode_write(0);
			apical_isp_top_isp_raw_bypass_write(1);
			break;
		case V4L2_MBUS_FMT_YUYV8_1X16:
			apical_isp_top_isp_processing_bypass_mode_write(3);
			apical_isp_top_isp_raw_bypass_write(1);
			break;
		default:
			apical_isp_top_isp_processing_bypass_mode_write(0);
			apical_isp_top_isp_raw_bypass_write(0);
			break;
		}
		apical_isp_top_ds1_dma_source_write(0);
		core->bypass = TX_ISP_FRAME_CHANNEL_BYPASS_ISP_DISABLE;
	}else{
		apical_isp_top_isp_processing_bypass_mode_write(2);
		apical_isp_top_isp_raw_bypass_write(1);
		apical_isp_top_ds1_dma_source_write(1);
		core->bypass = TX_ISP_FRAME_CHANNEL_BYPASS_ISP_ENABLE;
	}
	return ISP_SUCCESS;
}

static int apical_isp_freeze_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *freeze_fw = tuning->ctrls.freeze_fw;

	apical_ext_system_freeze_firmware_write(freeze_fw->val);

	return ISP_SUCCESS;
}


static inline int apical_isp_test_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *test = tuning->ctrls.test_pattern;
	int ret = ISP_SUCCESS;
#if 0
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	api.type = TSYSTEM;
	api.dir = COMMAND_SET;
	if(test->val == 0){
		api.id = TEST_PATTERN_ENABLE_ID;
		api.value = OFF;
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
		return ret;
	}
	api.id = TEST_PATTERN_MODE_ID;
	api.value = test->val - 1;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);

	api.id = TEST_PATTERN_ENABLE_ID;
	api.value = ON;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
#else
	if(test->val == 0){
		apical_isp_top_bypass_video_test_gen_write(1);
		apical_isp_video_test_gen_test_pattern_off_on_write(0);
		return ret;
	}
	apical_isp_video_test_gen_pattern_type_write(test->val-1);
	apical_isp_video_test_gen_test_pattern_off_on_write(1);
	apical_isp_top_bypass_video_test_gen_write(0);
#endif
	return ret;
}

static int apical_isp_blacklevel_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *bl = tuning->ctrls.black_level;

	apical_isp_top_bypass_sensor_offset_write(bl->val?0:1);
	return ISP_SUCCESS;
}

/* mve */
static inline int apical_isp_dis_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	api.type = TALGORITHMS;
	api.dir = COMMAND_SET;
	api.id = DIS_MODE_ID;
	api.value = tuning->ctrls.dis->val;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	return ret;
}

static inline int flicker_value_v4l2_to_apical(int val)
{
	int ret = 0;
	switch(val){
	case V4L2_CID_POWER_LINE_FREQUENCY_DISABLED:
		ret = 0;
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY_50HZ:
		ret = 50;
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY_60HZ:
		ret = 60;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static inline int apical_isp_flicker_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;
	api.type = TALGORITHMS;
	api.dir = COMMAND_SET;
	api.id = ANTIFLICKER_MODE_ID;
	api.value = flicker_value_v4l2_to_apical(tuning->ctrls.flicker->val);
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	stab.global_antiflicker_enable = api.value ?1 :0;
	return ret;
}

/* lens shading module */
static int apical_isp_lens_shad_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *shad = tuning->ctrls.lens_shad;

	apical_isp_mesh_shading_enable_write(shad->val);
	apical_isp_top_bypass_mesh_shading_write(shad->val?0:1);
	return ISP_SUCCESS;
}

static int apical_isp_lens_shad_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_shading_attr *attr = &tuning->ctrls.shad_attr;
	int i, base;

	copy_from_user(attr, (const void __user *)control->value, sizeof(*attr));
	apical_isp_mesh_shading_mesh_alpha_mode_write(attr->mesh_mode);
	apical_isp_mesh_shading_mesh_scale_write(attr->mesh_scale);
	apical_isp_mesh_shading_mesh_page_r_write(attr->r_page);
	apical_isp_mesh_shading_mesh_page_g_write(attr->g_page);
	apical_isp_mesh_shading_mesh_page_b_write(attr->b_page);
	apical_isp_mesh_shading_mesh_alpha_bank_r_write(attr->mesh_alpha_bank_r);
	apical_isp_mesh_shading_mesh_alpha_bank_g_write(attr->mesh_alpha_bank_g);
	apical_isp_mesh_shading_mesh_alpha_bank_b_write(attr->mesh_alpha_bank_b);
	apical_isp_mesh_shading_mesh_alpha_r_write(attr->mesh_alpha_r);
	apical_isp_mesh_shading_mesh_alpha_g_write(attr->mesh_alpha_g);
	apical_isp_mesh_shading_mesh_alpha_b_write(attr->mesh_alpha_b);
//	apical_isp_mesh_shading_mesh_strength_write(attr->mesh_strength);
	if(attr->update_page){
		base = 0x4000;
		for(i = 0; i < 1024; i++){
			APICAL_WRITE_32(base, attr->page0.regs[i]);
			base += 4;
		}
		for(i = 0; i < 1024; i++){
			APICAL_WRITE_32(base, attr->page1.regs[i]);
			base += 4;
		}
		for(i = 0; i < 1024; i++){
			APICAL_WRITE_32(base, attr->page2.regs[i]);
			base += 4;
		}
		apical_isp_mesh_shading_mesh_reload_write(1);
	}

	return ISP_SUCCESS;
}

/* Raw Frontend */
static inline int apical_isp_ge_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_green_eq_attr *attr = &tuning->ctrls.ge_attr;

	copy_from_user(attr, (const void __user *)control->value, sizeof(*attr));
	if(attr->mode){
		apical_isp_raw_frontend_ge_strength_write(attr->strength);
		apical_isp_raw_frontend_ge_threshold_write(attr->threshold);
		apical_isp_raw_frontend_ge_slope_write(attr->slope);
		apical_isp_raw_frontend_ge_sens_write(attr->sensitivity);
	}
	apical_isp_raw_frontend_ge_enable_write(attr->mode);
	return ISP_SUCCESS;
}


static int apical_isp_dynamic_dp_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *ddp = tuning->ctrls.dynamic_dp;
	struct v4l2_ctrl *ge = tuning->ctrls.green_eq;
	unsigned char bypass_module = ddp->val | ge->val;

	if(ddp->is_new){
		apical_isp_raw_frontend_dp_enable_write(ddp->val);
	}

	if(ge->is_new){
		apical_isp_raw_frontend_ge_enable_write(ge->val);
	}

	apical_isp_top_bypass_raw_frontend_write(bypass_module?0:1);
	return ISP_SUCCESS;
}

static int apical_isp_dynamic_dp_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_dynamic_defect_pixel_attr *attr = &tuning->ctrls.ddp_attr;

	copy_from_user(attr, (const void __user *)control->value, sizeof(*attr));
	if(attr->mode){
		apical_isp_raw_frontend_dark_disable_write(attr->dark_pixels);
		apical_isp_raw_frontend_bright_disable_write(attr->bright_pixels);
		apical_isp_raw_frontend_dp_threshold_write(attr->d_threshold);
		apical_isp_raw_frontend_dp_slope_write(attr->d_slope);
		apical_isp_raw_frontend_hpdev_threshold_write(attr->hpdev_thresh);
		apical_isp_raw_frontend_line_thresh_write(attr->line_thresh);
		apical_isp_raw_frontend_hp_blend_write(attr->hp_blend);
	}
	apical_isp_raw_frontend_dp_enable_write(attr->mode);

	return ISP_SUCCESS;
}

static int apical_isp_static_dp_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *sdp = tuning->ctrls.static_dp;

	apical_isp_top_bypass_defect_pixel_write(sdp->val?0:1);
	return ISP_SUCCESS;
}

static int apical_isp_static_dp_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_static_defect_pixel_attr *attr = &tuning->ctrls.sdp_attr;

	copy_from_user(attr, (const void __user *)control->value, sizeof(*attr));

	return ISP_SUCCESS;
}

static int apical_isp_antifog_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *fog = tuning->ctrls.fog;
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	api.type = TALGORITHMS;
	api.dir = COMMAND_SET;
	api.value = ANTIFOG_STRONG;
	if(fog->val){
		/* set present mode */
		api.id = ANTIFOG_PRESET_ID;
		switch(fog->val){
			case 3:
				api.value = ANTIFOG_WEAK;
				break;
			case 2:
				api.value = ANTIFOG_MEDIUM;
				break;
			case 1:
				api.value = ANTIFOG_STRONG;
				break;
			default:
				break;
		}
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
		/* enable anti fog modules */
		api.id = ANTIFOG_MODE_ID;
		api.value = ANTIFOG_ENABLE;
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	}else{
		/* disable anti fog modules */
		api.id = ANTIFOG_MODE_ID;
		api.value = ANTIFOG_DISABLE;
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	}

	return ret;
}


static inline int scene_value_v4l2_to_apical(int val)
{
	int ret = 0;
	switch(val){
		case V4L2_SCENE_MODE_NONE:
			ret = AUTO;
			break;
		case V4L2_SCENE_MODE_BEACH_SNOW:
			ret = BEACH_SNOW;
			break;
		case V4L2_SCENE_MODE_CANDLE_LIGHT:
			ret = CANDLE;
			break;
		case V4L2_SCENE_MODE_DAWN_DUSK:
			ret = DAWN;
			break;
		case V4L2_SCENE_MODE_FIREWORKS:
			ret = FIREWORKS;
			break;
		case V4L2_SCENE_MODE_LANDSCAPE:
			ret = LANDSCAPE;
			break;
		case V4L2_SCENE_MODE_NIGHT:
			ret = NIGHT;
			break;
		case V4L2_SCENE_MODE_PARTY_INDOOR:
			ret = INDOOR;
			break;
		case V4L2_SCENE_MODE_PORTRAIT:
			ret = PORTRAIT;
			break;
		case V4L2_SCENE_MODE_SPORTS:
			ret = MOTION;
			break;
		case V4L2_SCENE_MODE_SUNSET:
			ret = SUNSET;
			break;
		case V4L2_SCENE_MODE_TEXT:
			ret = TEXT;
			break;
		case V4L2_SCENE_MODE_TEXT + 1:
			ret = NIGHT_PORTRAIT;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static inline int apical_isp_scene_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	return ISP_SUCCESS;
	api.type = TSCENE_MODES;
	api.dir = COMMAND_SET;
	api.id = SCENE_MODE_ID;
	api.value = scene_value_v4l2_to_apical(tuning->ctrls.scene->val);
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	return ret;
}

static inline int colorfx_value_v4l2_to_apical(int val)
{
	int ret = 0;
	switch(val){
		case V4L2_COLORFX_NONE:
			ret = NORMAL;
			break;
		case V4L2_COLORFX_BW:
			ret = BLACK_AND_WHITE;
			break;
		case V4L2_COLORFX_SEPIA:
			ret = SEPIA;
			break;
		case V4L2_COLORFX_NEGATIVE:
			ret = NEGATIVE;
			break;
		case V4L2_COLORFX_VIVID:
			ret = VIVID;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static inline int apical_isp_colorfx_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	api.type = TSCENE_MODES;
	api.dir = COMMAND_SET;
	api.id = COLOR_MODE_ID;
	api.value = colorfx_value_v4l2_to_apical(tuning->ctrls.colorfx->val);
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	return ret;
}

static inline int apical_isp_day_or_night_g_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct image_tuning_ctrls *ctrls = &(tuning->ctrls);
	unsigned int  dn= ctrls->daynight;
	copy_to_user((void __user *)control->value, (const void *)&dn, sizeof(unsigned int));

	return ISP_SUCCESS;
}


int apical_isp_day_or_night_s_ctrl_internal(struct tx_isp_core_device *core)
{
	struct video_device *video = core->tun;
	TXispPrivParamManage *param = core->param;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct image_tuning_ctrls *ctrls = &(tuning->ctrls);
	int ret = ISP_SUCCESS;
	LookupTable** table = NULL;
	TXispPrivCustomerParamer *customer = NULL;
	unsigned int tmp_top = 0;
	apical_api_control_t api;
	unsigned int reason = 0;
	unsigned int status = 0;

	ISP_CORE_MODE_DN_E dn = ctrls->daynight;
	if(!param){
		v4l2_err(tuning->video->v4l2_dev,"Can't get the parameters of isp tuning!\n");
		return -ISP_ERROR;
	}
	{
		tmp_top = APICAL_READ_32(0x40);
		if(dn == ISP_CORE_RUNING_MODE_DAY_MODE){
#if TX_ISP_EXIST_FR_CHANNEL
			apical_isp_fr_cs_conv_clip_min_uv_write(0);
			apical_isp_fr_cs_conv_clip_max_uv_write(1023);
#endif
			apical_isp_ds1_cs_conv_clip_min_uv_write(0);
			apical_isp_ds1_cs_conv_clip_max_uv_write(1023);
#if TX_ISP_EXIST_DS2_CHANNEL
			apical_isp_ds2_cs_conv_clip_min_uv_write(0);
			apical_isp_ds2_cs_conv_clip_max_uv_write(1023);
#endif
			table = param->isp_param[TX_ISP_PRIV_PARAM_DAY_MODE].calibrations;
			customer = &param->customer[TX_ISP_PRIV_PARAM_DAY_MODE];
			/* tmp_top |= param->customer[TX_ISP_PRIV_PARAM_NIGHT_MODE].top; */
		}else{
#if TX_ISP_EXIST_FR_CHANNEL
			apical_isp_fr_cs_conv_clip_min_uv_write(512);
			apical_isp_fr_cs_conv_clip_max_uv_write(512);
#endif
			apical_isp_ds1_cs_conv_clip_min_uv_write(512);
			apical_isp_ds1_cs_conv_clip_max_uv_write(512);
#if TX_ISP_EXIST_DS2_CHANNEL
			apical_isp_ds2_cs_conv_clip_min_uv_write(512);
			apical_isp_ds2_cs_conv_clip_max_uv_write(512);
#endif
			table = param->isp_param[TX_ISP_PRIV_PARAM_NIGHT_MODE].calibrations;
			customer = &param->customer[TX_ISP_PRIV_PARAM_NIGHT_MODE];
			/* tmp_top |= param->customer[TX_ISP_PRIV_PARAM_DAY_MODE].top; */
		}
		if(table && customer){
			tmp_top = (tmp_top | 0x0c02da6c) & (~(customer->top));
			if(TX_ISP_EXIST_FR_CHANNEL == 0)
				tmp_top |= 0x00fc0000;
			/* dynamic calibration */
			apical_api_calibration(CALIBRATION_NP_LUT_MEAN,COMMAND_SET, table[_CALIBRATION_NP_LUT_MEAN]->ptr,
					       table[_CALIBRATION_NP_LUT_MEAN]->rows * table[_CALIBRATION_NP_LUT_MEAN]->cols
					       * table[_CALIBRATION_NP_LUT_MEAN]->width, &ret);
			apical_api_calibration(CALIBRATION_EVTOLUX_PROBABILITY_ENABLE,COMMAND_SET, table[ _CALIBRATION_EVTOLUX_PROBABILITY_ENABLE]->ptr,
					       table[ _CALIBRATION_EVTOLUX_PROBABILITY_ENABLE]->rows * table[ _CALIBRATION_EVTOLUX_PROBABILITY_ENABLE]->cols
					       * table[_CALIBRATION_EVTOLUX_PROBABILITY_ENABLE]->width, &ret);
			apical_api_calibration(CALIBRATION_AE_EXPOSURE_AVG_COEF,COMMAND_SET, table[ _CALIBRATION_AE_EXPOSURE_AVG_COEF]->ptr,
					       table[ _CALIBRATION_AE_EXPOSURE_AVG_COEF]->rows * table[ _CALIBRATION_AE_EXPOSURE_AVG_COEF]->cols
					       * table[ _CALIBRATION_AE_EXPOSURE_AVG_COEF]->width, &ret);
			apical_api_calibration(CALIBRATION_IRIDIX_AVG_COEF,COMMAND_SET, table[_CALIBRATION_IRIDIX_AVG_COEF]->ptr,
					       table[_CALIBRATION_IRIDIX_AVG_COEF]->rows * table[_CALIBRATION_IRIDIX_AVG_COEF]->cols
					       * table[_CALIBRATION_IRIDIX_AVG_COEF]->width, &ret);
			apical_api_calibration(CALIBRATION_AF_MIN_TABLE,COMMAND_SET, table[_CALIBRATION_AF_MIN_TABLE]->ptr,
					       table[_CALIBRATION_AF_MIN_TABLE]->rows * table[_CALIBRATION_AF_MIN_TABLE]->cols
					       * table[_CALIBRATION_AF_MIN_TABLE]->width, &ret);
			apical_api_calibration(CALIBRATION_AF_MAX_TABLE,COMMAND_SET, table[_CALIBRATION_AF_MAX_TABLE]->ptr,
					       table[_CALIBRATION_AF_MAX_TABLE]->rows * table[_CALIBRATION_AF_MAX_TABLE]->cols
					       * table[_CALIBRATION_AF_MAX_TABLE]->width, &ret);
			apical_api_calibration(CALIBRATION_AF_WINDOW_RESIZE_TABLE,COMMAND_SET, table[_CALIBRATION_AF_WINDOW_RESIZE_TABLE]->ptr,
					       table[_CALIBRATION_AF_WINDOW_RESIZE_TABLE]->rows * table[_CALIBRATION_AF_WINDOW_RESIZE_TABLE]->cols
					       * table[_CALIBRATION_AF_WINDOW_RESIZE_TABLE]->width, &ret);
			apical_api_calibration(CALIBRATION_EXP_RATIO_TABLE,COMMAND_SET, table[_CALIBRATION_EXP_RATIO_TABLE]->ptr,
					       table[_CALIBRATION_EXP_RATIO_TABLE]->rows * table[_CALIBRATION_EXP_RATIO_TABLE]->cols
					       * table[_CALIBRATION_EXP_RATIO_TABLE]->width, &ret);
			apical_api_calibration(CALIBRATION_CCM_ONE_GAIN_THRESHOLD,COMMAND_SET, table[_CALIBRATION_CCM_ONE_GAIN_THRESHOLD]->ptr,
					       table[_CALIBRATION_CCM_ONE_GAIN_THRESHOLD]->rows * table[_CALIBRATION_CCM_ONE_GAIN_THRESHOLD]->cols
					       * table[_CALIBRATION_CCM_ONE_GAIN_THRESHOLD]->width, &ret);
			apical_api_calibration(CALIBRATION_FLASH_RG,COMMAND_SET, table[_CALIBRATION_FLASH_RG]->ptr,
					       table[_CALIBRATION_FLASH_RG]->rows * table[_CALIBRATION_FLASH_RG]->cols
					       * table[_CALIBRATION_FLASH_RG]->width, &ret);
			apical_api_calibration(CALIBRATION_FLASH_BG,COMMAND_SET, table[_CALIBRATION_FLASH_BG]->ptr,
					       table[_CALIBRATION_FLASH_BG]->rows * table[_CALIBRATION_FLASH_BG]->cols
					       * table[_CALIBRATION_FLASH_BG]->width, &ret);
			apical_api_calibration(CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_LINEAR,COMMAND_SET, table[_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_LINEAR]->ptr,
					       table[_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_LINEAR]->rows * table[_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_LINEAR]->cols
					       * table[_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_WDR,COMMAND_SET, table[_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_WDR]->ptr,
					       table[_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_WDR]->rows * table[_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_WDR]->cols
					       * table[_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_WDR]->width, &ret);
			apical_api_calibration(CALIBRATION_IRIDIX_BLACK_PRC,COMMAND_SET, table[_CALIBRATION_IRIDIX_BLACK_PRC]->ptr,
					       table[_CALIBRATION_IRIDIX_BLACK_PRC]->rows * table[_CALIBRATION_IRIDIX_BLACK_PRC]->cols
					       * table[_CALIBRATION_IRIDIX_BLACK_PRC]->width, &ret);
			apical_api_calibration(CALIBRATION_IRIDIX_GAIN_MAX,COMMAND_SET, table[_CALIBRATION_IRIDIX_GAIN_MAX]->ptr,
					       table[_CALIBRATION_IRIDIX_GAIN_MAX]->rows * table[_CALIBRATION_IRIDIX_GAIN_MAX]->cols
					       * table[_CALIBRATION_IRIDIX_GAIN_MAX]->width, &ret);
			apical_api_calibration(CALIBRATION_IRIDIX_MIN_MAX_STR,COMMAND_SET, table[_CALIBRATION_IRIDIX_MIN_MAX_STR]->ptr,
					       table[_CALIBRATION_IRIDIX_MIN_MAX_STR]->rows * table[_CALIBRATION_IRIDIX_MIN_MAX_STR]->cols
					       * table[_CALIBRATION_IRIDIX_MIN_MAX_STR]->width, &ret);
			apical_api_calibration(CALIBRATION_IRIDIX_EV_LIM_FULL_STR,COMMAND_SET, table[_CALIBRATION_IRIDIX_EV_LIM_FULL_STR]->ptr,
					       table[_CALIBRATION_IRIDIX_EV_LIM_FULL_STR]->rows * table[_CALIBRATION_IRIDIX_EV_LIM_FULL_STR]->cols
					       * table[_CALIBRATION_IRIDIX_EV_LIM_FULL_STR]->width, &ret);
			apical_api_calibration(CALIBRATION_IRIDIX_EV_LIM_NO_STR_LINEAR,COMMAND_SET, table[_CALIBRATION_IRIDIX_EV_LIM_NO_STR_LINEAR]->ptr,
					       table[_CALIBRATION_IRIDIX_EV_LIM_NO_STR_LINEAR]->rows * table[_CALIBRATION_IRIDIX_EV_LIM_NO_STR_LINEAR]->cols
					       * table[_CALIBRATION_IRIDIX_EV_LIM_NO_STR_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_IRIDIX_EV_LIM_NO_STR_FS_HDR,COMMAND_SET, table[_CALIBRATION_IRIDIX_EV_LIM_NO_STR_FS_HDR]->ptr,
					       table[_CALIBRATION_IRIDIX_EV_LIM_NO_STR_FS_HDR]->rows * table[_CALIBRATION_IRIDIX_EV_LIM_NO_STR_FS_HDR]->cols
					       * table[_CALIBRATION_IRIDIX_EV_LIM_NO_STR_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_AE_CORRECTION_LINEAR,COMMAND_SET, table[_CALIBRATION_AE_CORRECTION_LINEAR]->ptr,
					       table[_CALIBRATION_AE_CORRECTION_LINEAR]->rows * table[_CALIBRATION_AE_CORRECTION_LINEAR]->cols
					       * table[_CALIBRATION_AE_CORRECTION_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_AE_CORRECTION_FS_HDR,COMMAND_SET, table[_CALIBRATION_AE_CORRECTION_FS_HDR]->ptr,
					       table[_CALIBRATION_AE_CORRECTION_FS_HDR]->rows * table[_CALIBRATION_AE_CORRECTION_FS_HDR]->cols
					       * table[_CALIBRATION_AE_CORRECTION_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_AE_EXPOSURE_CORRECTION,COMMAND_SET, table[_CALIBRATION_AE_EXPOSURE_CORRECTION]->ptr,
					       table[_CALIBRATION_AE_EXPOSURE_CORRECTION]->rows * table[_CALIBRATION_AE_EXPOSURE_CORRECTION]->cols
					       * table[_CALIBRATION_AE_EXPOSURE_CORRECTION]->width, &ret);
			apical_api_calibration(CALIBRATION_SINTER_STRENGTH_LINEAR,COMMAND_SET, table[_CALIBRATION_SINTER_STRENGTH_LINEAR]->ptr,
					       table[_CALIBRATION_SINTER_STRENGTH_LINEAR]->rows * table[_CALIBRATION_SINTER_STRENGTH_LINEAR]->cols
					       * table[_CALIBRATION_SINTER_STRENGTH_LINEAR]->width, &ret);

			apical_api_calibration(CALIBRATION_SINTER_STRENGTH_FS_HDR,COMMAND_SET, table[_CALIBRATION_SINTER_STRENGTH_FS_HDR]->ptr,
					       table[_CALIBRATION_SINTER_STRENGTH_FS_HDR]->rows * table[_CALIBRATION_SINTER_STRENGTH_FS_HDR]->cols
					       * table[_CALIBRATION_SINTER_STRENGTH_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SINTER_STRENGTH1_LINEAR,COMMAND_SET, table[_CALIBRATION_SINTER_STRENGTH1_LINEAR]->ptr,
					       table[_CALIBRATION_SINTER_STRENGTH1_LINEAR]->rows * table[_CALIBRATION_SINTER_STRENGTH1_LINEAR]->cols
					       * table[_CALIBRATION_SINTER_STRENGTH1_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SINTER_STRENGTH1_FS_HDR,COMMAND_SET, table[_CALIBRATION_SINTER_STRENGTH1_FS_HDR]->ptr,
					       table[_CALIBRATION_SINTER_STRENGTH1_FS_HDR]->rows * table[_CALIBRATION_SINTER_STRENGTH1_FS_HDR]->cols
					       * table[_CALIBRATION_SINTER_STRENGTH1_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SINTER_THRESH1_LINEAR,COMMAND_SET, table[_CALIBRATION_SINTER_THRESH1_LINEAR]->ptr,
					       table[_CALIBRATION_SINTER_THRESH1_LINEAR]->rows * table[_CALIBRATION_SINTER_THRESH1_LINEAR]->cols
					       * table[_CALIBRATION_SINTER_THRESH1_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SINTER_THRESH1_FS_HDR,COMMAND_SET, table[ _CALIBRATION_SINTER_THRESH1_FS_HDR]->ptr,
					       table[ _CALIBRATION_SINTER_THRESH1_FS_HDR]->rows * table[ _CALIBRATION_SINTER_THRESH1_FS_HDR]->cols
					       * table[ _CALIBRATION_SINTER_THRESH1_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SINTER_THRESH4_LINEAR,COMMAND_SET, table[ _CALIBRATION_SINTER_THRESH4_LINEAR]->ptr,
					       table[ _CALIBRATION_SINTER_THRESH4_LINEAR]->rows * table[ _CALIBRATION_SINTER_THRESH4_LINEAR]->cols
					       * table[ _CALIBRATION_SINTER_THRESH4_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SINTER_THRESH4_FS_HDR,COMMAND_SET, table[ _CALIBRATION_SINTER_THRESH4_FS_HDR]->ptr,
					       table[_CALIBRATION_SINTER_THRESH4_FS_HDR]->rows * table[ _CALIBRATION_SINTER_THRESH4_FS_HDR]->cols
					       * table[_CALIBRATION_SINTER_THRESH4_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHARP_ALT_D_LINEAR,COMMAND_SET, table[ _CALIBRATION_SHARP_ALT_D_LINEAR]->ptr,
					       table[_CALIBRATION_SHARP_ALT_D_LINEAR]->rows * table[_CALIBRATION_SHARP_ALT_D_LINEAR]->cols
					       * table[_CALIBRATION_SHARP_ALT_D_LINEAR]->width, &ret);

			apical_api_calibration(CALIBRATION_SHARP_ALT_D_FS_HDR,COMMAND_SET, table[_CALIBRATION_SHARP_ALT_D_FS_HDR]->ptr,
					       table[_CALIBRATION_SHARP_ALT_D_FS_HDR]->rows * table[_CALIBRATION_SHARP_ALT_D_FS_HDR]->cols
					       * table[_CALIBRATION_SHARP_ALT_D_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHARP_ALT_UD_LINEAR,COMMAND_SET, table[_CALIBRATION_SHARP_ALT_UD_LINEAR]->ptr,
					       table[_CALIBRATION_SHARP_ALT_UD_LINEAR]->rows * table[_CALIBRATION_SHARP_ALT_UD_LINEAR]->cols
					       * table[_CALIBRATION_SHARP_ALT_UD_LINEAR]->width, &ret);

			apical_api_calibration(CALIBRATION_SHARP_ALT_UD_FS_HDR,COMMAND_SET, table[ _CALIBRATION_SHARP_ALT_UD_FS_HDR]->ptr,
					       table[_CALIBRATION_SHARP_ALT_UD_FS_HDR]->rows * table[_CALIBRATION_SHARP_ALT_UD_FS_HDR]->cols
					       * table[_CALIBRATION_SHARP_ALT_UD_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHARPEN_FR_LINEAR,COMMAND_SET, table[ _CALIBRATION_SHARPEN_FR_LINEAR]->ptr,
					       table[_CALIBRATION_SHARPEN_FR_LINEAR]->rows * table[_CALIBRATION_SHARPEN_FR_LINEAR]->cols
					       * table[_CALIBRATION_SHARPEN_FR_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHARPEN_FR_WDR,COMMAND_SET, table[_CALIBRATION_SHARPEN_FR_WDR]->ptr,
					       table[ _CALIBRATION_SHARPEN_FR_WDR]->rows * table[ _CALIBRATION_SHARPEN_FR_WDR]->cols
					       * table[ _CALIBRATION_SHARPEN_FR_WDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHARPEN_DS1_LINEAR,COMMAND_SET, table[_CALIBRATION_SHARPEN_DS1_LINEAR]->ptr,
					       table[_CALIBRATION_SHARPEN_DS1_LINEAR]->rows * table[_CALIBRATION_SHARPEN_DS1_LINEAR]->cols
					       * table[_CALIBRATION_SHARPEN_DS1_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHARPEN_DS1_WDR,COMMAND_SET, table[_CALIBRATION_SHARPEN_DS1_WDR]->ptr,
					       table[_CALIBRATION_SHARPEN_DS1_WDR]->rows * table[_CALIBRATION_SHARPEN_DS1_WDR]->cols
					       * table[_CALIBRATION_SHARPEN_DS1_WDR]->width, &ret);
			apical_api_calibration(CALIBRATION_DEMOSAIC_NP_OFFSET_LINEAR,COMMAND_SET, table[_CALIBRATION_DEMOSAIC_NP_OFFSET_LINEAR]->ptr,
					       table[_CALIBRATION_DEMOSAIC_NP_OFFSET_LINEAR]->rows * table[_CALIBRATION_DEMOSAIC_NP_OFFSET_LINEAR]->cols
					       * table[_CALIBRATION_DEMOSAIC_NP_OFFSET_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_DEMOSAIC_NP_OFFSET_FS_HDR,COMMAND_SET, table[_CALIBRATION_DEMOSAIC_NP_OFFSET_FS_HDR]->ptr,
					       table[_CALIBRATION_DEMOSAIC_NP_OFFSET_FS_HDR]->rows * table[_CALIBRATION_DEMOSAIC_NP_OFFSET_FS_HDR]->cols
					       * table[_CALIBRATION_DEMOSAIC_NP_OFFSET_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_MESH_SHADING_STRENGTH,COMMAND_SET, table[_CALIBRATION_MESH_SHADING_STRENGTH]->ptr,
					       table[_CALIBRATION_MESH_SHADING_STRENGTH]->rows * table[_CALIBRATION_MESH_SHADING_STRENGTH]->cols
					       * table[_CALIBRATION_MESH_SHADING_STRENGTH]->width, &ret);
			apical_api_calibration(CALIBRATION_SATURATION_STRENGTH_LINEAR,COMMAND_SET, table[_CALIBRATION_SATURATION_STRENGTH_LINEAR]->ptr,
					       table[_CALIBRATION_SATURATION_STRENGTH_LINEAR]->rows * table[_CALIBRATION_SATURATION_STRENGTH_LINEAR]->cols
					       * table[_CALIBRATION_SATURATION_STRENGTH_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_TEMPER_STRENGTH,COMMAND_SET, table[_CALIBRATION_TEMPER_STRENGTH]->ptr,
					       table[_CALIBRATION_TEMPER_STRENGTH]->rows * table[_CALIBRATION_TEMPER_STRENGTH]->cols
					       * table[_CALIBRATION_TEMPER_STRENGTH]->width, &ret);

			apical_api_calibration(CALIBRATION_STITCHING_ERROR_THRESH,COMMAND_SET, table[_CALIBRATION_STITCHING_ERROR_THRESH]->ptr,
					       table[_CALIBRATION_STITCHING_ERROR_THRESH]->rows * table[_CALIBRATION_STITCHING_ERROR_THRESH]->cols
					       * table[_CALIBRATION_STITCHING_ERROR_THRESH]->width, &ret);
			apical_api_calibration(CALIBRATION_DP_SLOPE_LINEAR,COMMAND_SET, table[_CALIBRATION_DP_SLOPE_LINEAR]->ptr,
					       table[_CALIBRATION_DP_SLOPE_LINEAR]->rows * table[_CALIBRATION_DP_SLOPE_LINEAR]->cols
					       * table[_CALIBRATION_DP_SLOPE_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_DP_SLOPE_FS_HDR,COMMAND_SET, table[_CALIBRATION_DP_SLOPE_FS_HDR]->ptr,
					       table[_CALIBRATION_DP_SLOPE_FS_HDR]->rows * table[_CALIBRATION_DP_SLOPE_FS_HDR]->cols
					       * table[_CALIBRATION_DP_SLOPE_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_DP_THRESHOLD_LINEAR,COMMAND_SET, table[_CALIBRATION_DP_THRESHOLD_LINEAR]->ptr,
					       table[_CALIBRATION_DP_THRESHOLD_LINEAR]->rows * table[_CALIBRATION_DP_THRESHOLD_LINEAR]->cols
					       * table[_CALIBRATION_DP_THRESHOLD_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_DP_THRESHOLD_FS_HDR,COMMAND_SET, table[_CALIBRATION_DP_THRESHOLD_FS_HDR]->ptr,
					       table[_CALIBRATION_DP_THRESHOLD_FS_HDR]->rows * table[_CALIBRATION_DP_THRESHOLD_FS_HDR]->cols
					       * table[_CALIBRATION_DP_THRESHOLD_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_AE_BALANCED_LINEAR,COMMAND_SET, table[_CALIBRATION_AE_BALANCED_LINEAR]->ptr,
					       table[_CALIBRATION_AE_BALANCED_LINEAR]->rows * table[_CALIBRATION_AE_BALANCED_LINEAR]->cols
					       * table[_CALIBRATION_AE_BALANCED_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_AE_BALANCED_WDR,COMMAND_SET, table[_CALIBRATION_AE_BALANCED_WDR]->ptr,
					       table[_CALIBRATION_AE_BALANCED_WDR]->rows * table[_CALIBRATION_AE_BALANCED_WDR]->cols
					       * table[_CALIBRATION_AE_BALANCED_WDR]->width, &ret);
			apical_api_calibration(CALIBRATION_IRIDIX_STRENGTH_TABLE,COMMAND_SET, table[_CALIBRATION_IRIDIX_STRENGTH_TABLE]->ptr,
					       table[_CALIBRATION_IRIDIX_STRENGTH_TABLE]->rows * table[_CALIBRATION_IRIDIX_STRENGTH_TABLE]->cols
					       * table[_CALIBRATION_IRIDIX_STRENGTH_TABLE]->width, &ret);

			apical_api_calibration(CALIBRATION_RGB2YUV_CONVERSION,COMMAND_SET, table[_CALIBRATION_RGB2YUV_CONVERSION]->ptr,
					       table[_CALIBRATION_RGB2YUV_CONVERSION]->rows * table[_CALIBRATION_RGB2YUV_CONVERSION]->cols
					       * table[_CALIBRATION_RGB2YUV_CONVERSION]->width, &ret);

			/* static parameter */
			apical_api_calibration(CALIBRATION_EVTOLUX_EV_LUT_LINEAR, COMMAND_SET, table[_CALIBRATION_EVTOLUX_EV_LUT_LINEAR]->ptr,
					       table[_CALIBRATION_EVTOLUX_EV_LUT_LINEAR]->rows * table[_CALIBRATION_EVTOLUX_EV_LUT_LINEAR]->cols
					       * table[_CALIBRATION_EVTOLUX_EV_LUT_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_EVTOLUX_EV_LUT_FS_HDR, COMMAND_SET, table[_CALIBRATION_EVTOLUX_EV_LUT_FS_HDR]->ptr,
					       table[_CALIBRATION_EVTOLUX_EV_LUT_FS_HDR]->rows * table[_CALIBRATION_EVTOLUX_EV_LUT_FS_HDR]->cols
					       * table[_CALIBRATION_EVTOLUX_EV_LUT_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_EVTOLUX_LUX_LUT, COMMAND_SET, table[_CALIBRATION_EVTOLUX_LUX_LUT]->ptr,
					       table[_CALIBRATION_EVTOLUX_LUX_LUT]->rows * table[_CALIBRATION_EVTOLUX_LUX_LUT]->cols
					       * table[_CALIBRATION_EVTOLUX_LUX_LUT]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_A_R_LINEAR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_A_R_LINEAR]->ptr,
					       table[_CALIBRATION_SHADING_LS_A_R_LINEAR]->rows * table[_CALIBRATION_SHADING_LS_A_R_LINEAR]->cols
					       * table[_CALIBRATION_SHADING_LS_A_R_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_A_G_LINEAR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_A_G_LINEAR]->ptr,
					       table[_CALIBRATION_SHADING_LS_A_G_LINEAR]->rows * table[_CALIBRATION_SHADING_LS_A_G_LINEAR]->cols
					       * table[_CALIBRATION_SHADING_LS_A_G_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_A_B_LINEAR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_A_B_LINEAR]->ptr,
					       table[_CALIBRATION_SHADING_LS_A_B_LINEAR]->rows * table[_CALIBRATION_SHADING_LS_A_B_LINEAR]->cols
					       * table[_CALIBRATION_SHADING_LS_A_B_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_TL84_R_LINEAR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_TL84_R_LINEAR]->ptr,
					       table[_CALIBRATION_SHADING_LS_TL84_R_LINEAR]->rows * table[_CALIBRATION_SHADING_LS_TL84_R_LINEAR]->cols
					       * table[_CALIBRATION_SHADING_LS_TL84_R_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_TL84_G_LINEAR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_TL84_G_LINEAR]->ptr,
					       table[_CALIBRATION_SHADING_LS_TL84_G_LINEAR]->rows * table[_CALIBRATION_SHADING_LS_TL84_G_LINEAR]->cols
					       * table[_CALIBRATION_SHADING_LS_TL84_G_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_TL84_B_LINEAR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_TL84_B_LINEAR]->ptr,
					       table[_CALIBRATION_SHADING_LS_TL84_B_LINEAR]->rows * table[_CALIBRATION_SHADING_LS_TL84_B_LINEAR]->cols
					       * table[_CALIBRATION_SHADING_LS_TL84_B_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_D65_R_LINEAR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_D65_R_LINEAR]->ptr,
					       table[_CALIBRATION_SHADING_LS_D65_R_LINEAR]->rows * table[_CALIBRATION_SHADING_LS_D65_R_LINEAR]->cols
					       * table[_CALIBRATION_SHADING_LS_D65_R_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_D65_G_LINEAR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_D65_G_LINEAR]->ptr,
					       table[_CALIBRATION_SHADING_LS_D65_G_LINEAR]->rows * table[_CALIBRATION_SHADING_LS_D65_G_LINEAR]->cols
					       * table[_CALIBRATION_SHADING_LS_D65_G_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_D65_B_LINEAR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_D65_B_LINEAR]->ptr,
					       table[_CALIBRATION_SHADING_LS_D65_B_LINEAR]->rows * table[_CALIBRATION_SHADING_LS_D65_B_LINEAR]->cols
					       * table[_CALIBRATION_SHADING_LS_D65_B_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_A_R_WDR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_A_R_WDR]->ptr,
					       table[_CALIBRATION_SHADING_LS_A_R_WDR]->rows * table[_CALIBRATION_SHADING_LS_A_R_WDR]->cols
					       * table[_CALIBRATION_SHADING_LS_A_R_WDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_A_G_WDR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_A_G_WDR]->ptr,
					       table[_CALIBRATION_SHADING_LS_A_G_WDR]->rows * table[_CALIBRATION_SHADING_LS_A_G_WDR]->cols
					       * table[_CALIBRATION_SHADING_LS_A_G_WDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_A_B_WDR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_A_B_WDR]->ptr,
					       table[_CALIBRATION_SHADING_LS_A_B_WDR]->rows * table[_CALIBRATION_SHADING_LS_A_B_WDR]->cols
					       * table[_CALIBRATION_SHADING_LS_A_B_WDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_TL84_R_WDR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_TL84_R_WDR]->ptr,
					       table[_CALIBRATION_SHADING_LS_TL84_R_WDR]->rows * table[_CALIBRATION_SHADING_LS_TL84_R_WDR]->cols
					       * table[_CALIBRATION_SHADING_LS_TL84_R_WDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_TL84_G_WDR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_TL84_G_WDR]->ptr,
					       table[_CALIBRATION_SHADING_LS_TL84_G_WDR]->rows * table[_CALIBRATION_SHADING_LS_TL84_G_WDR]->cols
					       * table[_CALIBRATION_SHADING_LS_TL84_G_WDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_TL84_B_WDR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_TL84_B_WDR]->ptr,
					       table[_CALIBRATION_SHADING_LS_TL84_B_WDR]->rows * table[_CALIBRATION_SHADING_LS_TL84_B_WDR]->cols
					       * table[_CALIBRATION_SHADING_LS_TL84_B_WDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_D65_R_WDR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_D65_R_WDR]->ptr,
					       table[_CALIBRATION_SHADING_LS_D65_R_WDR]->rows * table[_CALIBRATION_SHADING_LS_D65_R_WDR]->cols
					       * table[_CALIBRATION_SHADING_LS_D65_R_WDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_D65_G_WDR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_D65_G_WDR]->ptr,
					       table[_CALIBRATION_SHADING_LS_D65_G_WDR]->rows * table[_CALIBRATION_SHADING_LS_D65_G_WDR]->cols
					       * table[_CALIBRATION_SHADING_LS_D65_G_WDR]->width, &ret);
			apical_api_calibration(CALIBRATION_SHADING_LS_D65_B_WDR, COMMAND_SET, table[_CALIBRATION_SHADING_LS_D65_B_WDR]->ptr,
					       table[_CALIBRATION_SHADING_LS_D65_B_WDR]->rows * table[_CALIBRATION_SHADING_LS_D65_B_WDR]->cols
					       * table[_CALIBRATION_SHADING_LS_D65_B_WDR]->width, &ret);
			apical_api_calibration(CALIBRATION_NOISE_PROFILE_LINEAR, COMMAND_SET, table[_CALIBRATION_NOISE_PROFILE_LINEAR]->ptr,
					       table[_CALIBRATION_NOISE_PROFILE_LINEAR]->rows * table[_CALIBRATION_NOISE_PROFILE_LINEAR]->cols
					       * table[_CALIBRATION_NOISE_PROFILE_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_DEMOSAIC_LINEAR, COMMAND_SET, table[_CALIBRATION_DEMOSAIC_LINEAR]->ptr,
					       table[_CALIBRATION_DEMOSAIC_LINEAR]->rows * table[_CALIBRATION_DEMOSAIC_LINEAR]->cols
					       * table[_CALIBRATION_DEMOSAIC_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_NOISE_PROFILE_FS_HDR, COMMAND_SET, table[_CALIBRATION_NOISE_PROFILE_FS_HDR]->ptr,
					       table[_CALIBRATION_NOISE_PROFILE_FS_HDR]->rows * table[_CALIBRATION_NOISE_PROFILE_FS_HDR]->cols
					       * table[_CALIBRATION_NOISE_PROFILE_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_DEMOSAIC_FS_HDR, COMMAND_SET, table[_CALIBRATION_DEMOSAIC_FS_HDR]->ptr,
					       table[_CALIBRATION_DEMOSAIC_FS_HDR]->rows * table[_CALIBRATION_DEMOSAIC_FS_HDR]->cols
					       * table[_CALIBRATION_DEMOSAIC_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_GAMMA_FE_0_FS_HDR, COMMAND_SET, table[_CALIBRATION_GAMMA_FE_0_FS_HDR]->ptr,
					       table[_CALIBRATION_GAMMA_FE_0_FS_HDR]->rows * table[_CALIBRATION_GAMMA_FE_0_FS_HDR]->cols
					       * table[_CALIBRATION_GAMMA_FE_0_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_GAMMA_FE_1_FS_HDR, COMMAND_SET, table[_CALIBRATION_GAMMA_FE_1_FS_HDR]->ptr,
					       table[_CALIBRATION_GAMMA_FE_1_FS_HDR]->rows * table[_CALIBRATION_GAMMA_FE_1_FS_HDR]->cols
					       * table[_CALIBRATION_GAMMA_FE_1_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_BLACK_LEVEL_R_LINEAR, COMMAND_SET, table[_CALIBRATION_BLACK_LEVEL_R_LINEAR]->ptr,
					       table[_CALIBRATION_BLACK_LEVEL_R_LINEAR]->rows * table[_CALIBRATION_BLACK_LEVEL_R_LINEAR]->cols
					       * table[_CALIBRATION_BLACK_LEVEL_R_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_BLACK_LEVEL_GR_LINEAR, COMMAND_SET, table[_CALIBRATION_BLACK_LEVEL_GR_LINEAR]->ptr,
					       table[_CALIBRATION_BLACK_LEVEL_GR_LINEAR]->rows * table[_CALIBRATION_BLACK_LEVEL_GR_LINEAR]->cols
					       * table[_CALIBRATION_BLACK_LEVEL_GR_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_BLACK_LEVEL_GB_LINEAR, COMMAND_SET, table[_CALIBRATION_BLACK_LEVEL_GB_LINEAR]->ptr,
					       table[_CALIBRATION_BLACK_LEVEL_GB_LINEAR]->rows * table[_CALIBRATION_BLACK_LEVEL_GB_LINEAR]->cols
					       * table[_CALIBRATION_BLACK_LEVEL_GB_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_BLACK_LEVEL_B_LINEAR, COMMAND_SET, table[_CALIBRATION_BLACK_LEVEL_B_LINEAR]->ptr,
					       table[_CALIBRATION_BLACK_LEVEL_B_LINEAR]->rows * table[_CALIBRATION_BLACK_LEVEL_B_LINEAR]->cols
					       * table[_CALIBRATION_BLACK_LEVEL_B_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_BLACK_LEVEL_R_FS_HDR, COMMAND_SET, table[_CALIBRATION_BLACK_LEVEL_R_FS_HDR]->ptr,
					       table[_CALIBRATION_BLACK_LEVEL_R_FS_HDR]->rows * table[_CALIBRATION_BLACK_LEVEL_R_FS_HDR]->cols
					       * table[_CALIBRATION_BLACK_LEVEL_R_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_BLACK_LEVEL_GR_FS_HDR, COMMAND_SET, table[_CALIBRATION_BLACK_LEVEL_GR_FS_HDR]->ptr,
					       table[_CALIBRATION_BLACK_LEVEL_GR_FS_HDR]->rows * table[_CALIBRATION_BLACK_LEVEL_GR_FS_HDR]->cols
					       * table[_CALIBRATION_BLACK_LEVEL_GR_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_BLACK_LEVEL_GB_FS_HDR, COMMAND_SET, table[_CALIBRATION_BLACK_LEVEL_GB_FS_HDR]->ptr,
					       table[_CALIBRATION_BLACK_LEVEL_GB_FS_HDR]->rows * table[_CALIBRATION_BLACK_LEVEL_GB_FS_HDR]->cols
					       * table[_CALIBRATION_BLACK_LEVEL_GB_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_BLACK_LEVEL_B_FS_HDR, COMMAND_SET, table[_CALIBRATION_BLACK_LEVEL_B_FS_HDR]->ptr,
					       table[_CALIBRATION_BLACK_LEVEL_B_FS_HDR]->rows * table[_CALIBRATION_BLACK_LEVEL_B_FS_HDR]->cols
					       * table[_CALIBRATION_BLACK_LEVEL_B_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_GAMMA_LINEAR, COMMAND_SET, table[_CALIBRATION_GAMMA_LINEAR]->ptr,
					       table[_CALIBRATION_GAMMA_LINEAR]->rows * table[_CALIBRATION_GAMMA_LINEAR]->cols
					       * table[_CALIBRATION_GAMMA_LINEAR]->width, &ret);
			apical_api_calibration(CALIBRATION_GAMMA_FS_HDR, COMMAND_SET, table[_CALIBRATION_GAMMA_FS_HDR]->ptr,
					       table[_CALIBRATION_GAMMA_FS_HDR]->rows * table[_CALIBRATION_GAMMA_FS_HDR]->cols
					       * table[_CALIBRATION_GAMMA_FS_HDR]->width, &ret);
			apical_api_calibration(CALIBRATION_IRIDIX_RGB2REC709, COMMAND_SET, table[_CALIBRATION_IRIDIX_RGB2REC709]->ptr,
					       table[_CALIBRATION_IRIDIX_RGB2REC709]->rows * table[_CALIBRATION_IRIDIX_RGB2REC709]->cols
					       * table[_CALIBRATION_IRIDIX_RGB2REC709]->width, &ret);
			apical_api_calibration(CALIBRATION_IRIDIX_REC709TORGB, COMMAND_SET, table[_CALIBRATION_IRIDIX_REC709TORGB]->ptr,
					       table[_CALIBRATION_IRIDIX_REC709TORGB]->rows * table[_CALIBRATION_IRIDIX_REC709TORGB]->cols
					       * table[_CALIBRATION_IRIDIX_REC709TORGB]->width, &ret);
			apical_api_calibration(CALIBRATION_IRIDIX_ASYMMETRY, COMMAND_SET, table[_CALIBRATION_IRIDIX_ASYMMETRY]->ptr,
					       table[_CALIBRATION_IRIDIX_ASYMMETRY]->rows * table[_CALIBRATION_IRIDIX_ASYMMETRY]->cols
					       * table[_CALIBRATION_IRIDIX_ASYMMETRY]->width, &ret);
			apical_api_calibration(CALIBRATION_DEFECT_PIXELS, COMMAND_SET, table[_CALIBRATION_DEFECT_PIXELS]->ptr,
					       table[_CALIBRATION_DEFECT_PIXELS]->rows * table[_CALIBRATION_DEFECT_PIXELS]->cols
					       * table[_CALIBRATION_DEFECT_PIXELS]->width, &ret);

			/* green equalization */
			apical_isp_raw_frontend_ge_strength_write(customer->ge_strength);
			apical_isp_raw_frontend_ge_threshold_write(customer->ge_threshold);
			apical_isp_raw_frontend_ge_slope_write(customer->ge_slope);
			apical_isp_raw_frontend_ge_sens_write(customer->ge_sensitivity);

			/* dpc configuration	 */
			apical_isp_raw_frontend_dp_enable_write(customer->dp_module);
			apical_isp_raw_frontend_hpdev_threshold_write(customer->hpdev_threshold);
			apical_isp_raw_frontend_line_thresh_write(customer->line_threshold);
			apical_isp_raw_frontend_hp_blend_write(customer->hp_blend);

			apical_isp_demosaic_vh_slope_write(customer->dmsc_vh_slope);
			apical_isp_demosaic_aa_slope_write(customer->dmsc_aa_slope);
			apical_isp_demosaic_va_slope_write(customer->dmsc_va_slope);
			apical_isp_demosaic_uu_slope_write(customer->dmsc_uu_slope);
			apical_isp_demosaic_sat_slope_write(customer->dmsc_sat_slope);
			apical_isp_demosaic_vh_thresh_write(customer->dmsc_vh_threshold);
			apical_isp_demosaic_aa_thresh_write(customer->dmsc_aa_threshold);
			apical_isp_demosaic_va_thresh_write(customer->dmsc_va_threshold);
			apical_isp_demosaic_uu_thresh_write(customer->dmsc_uu_threshold);
			apical_isp_demosaic_sat_thresh_write(customer->dmsc_sat_threshold);
			apical_isp_demosaic_vh_offset_write(customer->dmsc_vh_offset);
			apical_isp_demosaic_aa_offset_write(customer->dmsc_aa_offset);
			apical_isp_demosaic_va_offset_write(customer->dmsc_va_offset);
			apical_isp_demosaic_uu_offset_write(customer->dmsc_uu_offset);
			apical_isp_demosaic_sat_offset_write(customer->dmsc_sat_offset);
			apical_isp_demosaic_lum_thresh_write(customer->dmsc_luminance_thresh);
			apical_isp_demosaic_np_offset_write(customer->dmsc_np_offset);
			apical_isp_demosaic_dmsc_config_write(customer->dmsc_config);
			apical_isp_demosaic_ac_thresh_write(customer->dmsc_ac_threshold);
			apical_isp_demosaic_ac_slope_write(customer->dmsc_ac_slope);
			apical_isp_demosaic_ac_offset_write(customer->dmsc_ac_offset);
			apical_isp_demosaic_fc_slope_write(customer->dmsc_fc_slope);
			apical_isp_demosaic_fc_alias_slope_write(customer->dmsc_fc_alias_slope);
			apical_isp_demosaic_fc_alias_thresh_write(customer->dmsc_fc_alias_thresh);
			apical_isp_demosaic_np_off_write(customer->dmsc_np_off);
			apical_isp_demosaic_np_off_reflect_write(customer->dmsc_np_reflect);

			apical_isp_temper_recursion_limit_write(customer->temper_recursion_limit);
			apical_isp_frame_stitch_short_thresh_write(customer->wdr_short_thresh);
			apical_isp_frame_stitch_long_thresh_write(customer->wdr_long_thresh);
			apical_isp_frame_stitch_exposure_ratio_write(customer->wdr_expo_ratio_thresh);
			apical_isp_frame_stitch_stitch_correct_write(customer->wdr_stitch_correct);
			apical_isp_frame_stitch_stitch_error_thresh_write(customer->wdr_stitch_error_thresh);
			apical_isp_frame_stitch_stitch_error_limit_write(customer->wdr_stitch_error_limit);
			apical_isp_frame_stitch_black_level_out_write(customer->wdr_stitch_bl_long);
			apical_isp_frame_stitch_black_level_short_write(customer->wdr_stitch_bl_short);
			apical_isp_frame_stitch_black_level_long_write(customer->wdr_stitch_bl_output);

			/* Max ISP Digital Gain */
			api.type = TSYSTEM;
			api.dir = COMMAND_SET;
			api.value = customer->max_isp_dgain;
			api.id = SYSTEM_MAX_ISP_DIGITAL_GAIN;

			status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			if(status != ISP_SUCCESS) {
				ISP_PRINT(ISP_WARNING_LEVEL,"Custom set max isp digital gain failure!reture value is %d,reason is %d\n",status,reason);
			}

			/* Max Sensor Analog Gain */
			api.type = TSYSTEM;
			api.dir = COMMAND_SET;
			api.value = customer->max_sensor_again;
			api.id = SYSTEM_MAX_SENSOR_ANALOG_GAIN;

			status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			if(status != ISP_SUCCESS) {
				ISP_PRINT(ISP_WARNING_LEVEL,"Custom set max isp digital gain failure!reture value is %d,reason is %d\n",status,reason);
			}

			/* modify the node */
			api.type = TIMAGE;
			api.dir = COMMAND_GET;
			api.id = WDR_MODE_ID;
			api.value = -1;
			status = apical_command(api.type, api.id, api.value, api.dir, &reason);
			if(status != ISP_SUCCESS) {
				ISP_PRINT(ISP_WARNING_LEVEL,"Get WDR mode failure!reture value is %d,reason is %d\n",status,reason);
			}

			if (reason == IMAGE_WDR_MODE_LINEAR) {
				stab.global_minimum_sinter_strength = *((uint16_t *)(table[ _CALIBRATION_SINTER_STRENGTH_LINEAR]->ptr) + 1);
				stab.global_maximum_sinter_strength = *((uint16_t *)(table[ _CALIBRATION_SINTER_STRENGTH_LINEAR]->ptr) + table[_CALIBRATION_SINTER_STRENGTH_LINEAR]->rows * table[_CALIBRATION_SINTER_STRENGTH_LINEAR]->cols -1 );

				stab.global_maximum_directional_sharpening = *((uint16_t *)(table[ _CALIBRATION_SHARP_ALT_D_LINEAR]->ptr) + 1);
				stab.global_minimum_directional_sharpening = *((uint16_t *)(table[ _CALIBRATION_SHARP_ALT_D_LINEAR]->ptr) + table[_CALIBRATION_SHARP_ALT_D_LINEAR]->rows * table[_CALIBRATION_SHARP_ALT_D_LINEAR]->cols -1 );

				stab.global_maximum_un_directional_sharpening = *((uint16_t *)(table[ _CALIBRATION_SHARP_ALT_UD_LINEAR]->ptr) + 1);
				stab.global_minimum_un_directional_sharpening = *((uint16_t *)(table[ _CALIBRATION_SHARP_ALT_UD_LINEAR]->ptr) + table[_CALIBRATION_SHARP_ALT_UD_LINEAR]->rows * table[_CALIBRATION_SHARP_ALT_UD_LINEAR]->cols -1 );

				stab.global_maximum_iridix_strength = *(uint8_t *)(table[_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_LINEAR]->ptr);
			} else if (reason == IMAGE_WDR_MODE_FS_HDR) {
				stab.global_minimum_sinter_strength = *((uint16_t *)(table[ _CALIBRATION_SINTER_STRENGTH_FS_HDR]->ptr) + 1);
				stab.global_maximum_sinter_strength = *((uint16_t *)(table[ _CALIBRATION_SINTER_STRENGTH_FS_HDR]->ptr) + table[_CALIBRATION_SINTER_STRENGTH_LINEAR]->rows * table[_CALIBRATION_SINTER_STRENGTH_LINEAR]->cols -1 );

				stab.global_maximum_directional_sharpening = *((uint16_t *)(table[ _CALIBRATION_SHARP_ALT_D_FS_HDR]->ptr) + 1);
				stab.global_minimum_directional_sharpening = *((uint16_t *)(table[ _CALIBRATION_SHARP_ALT_D_FS_HDR]->ptr) + table[_CALIBRATION_SHARP_ALT_D_LINEAR]->rows * table[_CALIBRATION_SHARP_ALT_D_LINEAR]->cols -1 );

				stab.global_maximum_un_directional_sharpening = *((uint16_t *)(table[ _CALIBRATION_SHARP_ALT_UD_FS_HDR]->ptr) + 1);
				stab.global_minimum_un_directional_sharpening = *((uint16_t *)(table[ _CALIBRATION_SHARP_ALT_UD_FS_HDR]->ptr) + table[_CALIBRATION_SHARP_ALT_UD_LINEAR]->rows * table[_CALIBRATION_SHARP_ALT_UD_LINEAR]->cols -1 );

				stab.global_maximum_iridix_strength = *(uint8_t *)(table[_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_WDR]->ptr);
			}
			stab.global_minimum_temper_strength = *((uint16_t *)(table[ _CALIBRATION_TEMPER_STRENGTH]->ptr) + 1);
			stab.global_maximum_temper_strength = *((uint16_t *)(table[ _CALIBRATION_TEMPER_STRENGTH]->ptr) + table[_CALIBRATION_TEMPER_STRENGTH]->rows * table[_CALIBRATION_TEMPER_STRENGTH]->cols -1 );
			stab.global_minimum_iridix_strength = *(uint8_t *)(table[_CALIBRATION_IRIDIX_MIN_MAX_STR]->ptr);

			APICAL_WRITE_32(0x40, tmp_top);
			/* if it is T20,the FR is corresponding to DS2 in bin file. */
			if (customer->top & (1 << 19)){
#if TX_ISP_EXIST_FR_CHANNEL
				apical_isp_top_bypass_fr_gamma_rgb_write(0);
				apical_isp_fr_gamma_rgb_enable_write(1);
#endif
#if TX_ISP_EXIST_DS2_CHANNEL
				apical_isp_top_bypass_ds2_gamma_rgb_write(0);
				apical_isp_ds2_gamma_rgb_enable_write(1);
#endif
			} else {
#if TX_ISP_EXIST_FR_CHANNEL
				apical_isp_top_bypass_fr_gamma_rgb_write(1);
				apical_isp_fr_gamma_rgb_enable_write(0);
#endif
#if TX_ISP_EXIST_DS2_CHANNEL
				apical_isp_top_bypass_ds2_gamma_rgb_write(1);
				apical_isp_ds2_gamma_rgb_enable_write(0);
#endif
			}

			if ((customer->top) & (1 << 20)){
#if TX_ISP_EXIST_FR_CHANNEL
				apical_isp_top_bypass_fr_sharpen_write(0);
				apical_isp_fr_sharpen_enable_write(1);
#endif
#if TX_ISP_EXIST_DS2_CHANNEL
				apical_isp_top_bypass_ds2_sharpen_write(0);
				apical_isp_ds2_sharpen_enable_write(1);
#endif
			} else {
#if TX_ISP_EXIST_FR_CHANNEL
				apical_isp_fr_sharpen_enable_write(1);
				apical_isp_top_bypass_fr_sharpen_write(0);
#endif
#ifdef TX_ISP_EXIST_DS2_CHANNEL
				apical_isp_top_bypass_ds2_sharpen_write(1);
				apical_isp_ds2_sharpen_enable_write(0);
#endif
			}
			if ((customer->top) & (1 << 27))
				apical_isp_ds1_sharpen_enable_write(1);
			else
				apical_isp_ds1_sharpen_enable_write(0);
		}
		ctrls->daynight = dn;
	}
	return ret;
}


static inline int apical_isp_day_or_night_s_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	TXispPrivParamManage *param = core->param;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct image_tuning_ctrls *ctrls = &(tuning->ctrls);
	int ret = ISP_SUCCESS;

	ISP_CORE_MODE_DN_E dn;
	copy_from_user(&dn, (const void __user *)control->value, sizeof(ISP_CORE_MODE_DN_E));
	if(!param){
		v4l2_err(tuning->video->v4l2_dev,"Can't get the parameters of isp tuning!\n");
		return -ISP_ERROR;
	}
	if(dn != ctrls->daynight){
		ctrls->daynight = dn;
		core->isp_daynight_switch = 1;
	}
	return ret;
}


static inline int apical_isp_ae_strategy_g_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	apical_api_control_t api;
	unsigned int reason = 0;
	unsigned int status = 0;

        api.type = TALGORITHMS;
        api.dir = COMMAND_GET;
        api.id = AE_SPLIT_PRESET_ID;
        api.value = -1;
        status = apical_command(api.type, api.id, api.value, api.dir, &reason);

	switch(reason){
	case AE_SPLIT_BALANCED:
		control->value = IMPISP_AE_STRATEGY_SPLIT_BALANCED;
		break;
	case AE_SPLIT_INTEGRATION_PRIORITY:
		control->value = IMPISP_AE_STRATEGY_SPLIT_INTEGRATION_PRIORITY;
		break;
	default:
		control->value = IMPISP_AE_STRATEGY_BUTT;
		break;
	}
        if(status != ISP_SUCCESS) {
		ISP_PRINT(ISP_WARNING_LEVEL,"Custom Get AE strategy failure!reture value is %d,reason is %d\n", status, reason);
	}
	return ISP_SUCCESS;
}

static inline int apical_isp_ae_strategy_s_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	apical_api_control_t api;
	unsigned int reason = 0;
	unsigned int status = 0;
	int ret = ISP_SUCCESS;

        api.type = TALGORITHMS;
        api.id = AE_SPLIT_PRESET_ID;
        api.dir = COMMAND_SET;
	api.value = control->value ? AE_SPLIT_INTEGRATION_PRIORITY : AE_SPLIT_BALANCED;
        status = apical_command(api.type, api.id, api.value, api.dir, &reason);

        if(status != ISP_SUCCESS) {
		ISP_PRINT(ISP_WARNING_LEVEL,"Custom Set AE strategy failure!reture value is %d,reason is %d\n",status, reason);
	}
	return ret;
}

static inline int apical_isp_awb_cwf_g_shift(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	TXispPrivParamManage *param = core->param;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct image_tuning_ctrls *ctrls = &(tuning->ctrls);
	LookupTable** table = NULL;
	ISP_CORE_MODE_DN_E dn;
	uint16_t rgain = 0;
	uint16_t bgain = 0;

	dn = ctrls->daynight;
	if(param){
		table = param->isp_param[dn].calibrations;

		rgain = *(uint16_t *)(table[_CALIBRATION_LIGHT_SRC]->ptr);
		bgain = *((uint16_t *)(table[_CALIBRATION_LIGHT_SRC]->ptr) + 1);
		control->value = (rgain << 16) + bgain;
	}
	return 0;
}

static inline int apical_isp_awb_cwf_s_shift(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	TXispPrivParamManage *param = core->param;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct image_tuning_ctrls *ctrls = &(tuning->ctrls);
	LookupTable** table = NULL;
	ISP_CORE_MODE_DN_E dn;
	int ret = ISP_SUCCESS;

	dn = ctrls->daynight;
	if(param){
		table = param->isp_param[dn].calibrations;

		*(uint16_t *)(table[_CALIBRATION_LIGHT_SRC]->ptr) = control->value >> 16;
		*((uint16_t *)(table[_CALIBRATION_LIGHT_SRC]->ptr) + 1) = control->value & 0xffff;
		apical_api_calibration(CALIBRATION_LIGHT_SRC,COMMAND_SET, table[_CALIBRATION_LIGHT_SRC]->ptr,
				table[_CALIBRATION_LIGHT_SRC]->rows * table[_CALIBRATION_LIGHT_SRC]->cols
				* table[_CALIBRATION_LIGHT_SRC]->width, &ret);
	}
	return ret;
}

static inline int apical_isp_sat_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	TXispPrivParamManage *param = core->param;
	struct v4l2_mbus_framefmt *mbus = &core->vin.mbus;
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	unsigned int value = 128;
	unsigned int mid_node = 128;
	int ret = ISP_SUCCESS;

	if(mbus->code == V4L2_MBUS_FMT_YUYV8_1X16){
		return ret;
	}
	/* the original value */
	value = tuning->ctrls.saturation->val & 0xff;
	if(param == NULL)
		goto set_saturation_val;

	if(tuning->ctrls.daynight == ISP_CORE_RUNING_MODE_DAY_MODE){
			mid_node = param->customer[TX_ISP_PRIV_PARAM_DAY_MODE].saturation;
	}else{
			mid_node = param->customer[TX_ISP_PRIV_PARAM_NIGHT_MODE].saturation;
	}

	/* the amended value */
	value = value < 128 ? (value * mid_node / 128) : (mid_node + (0xff - mid_node) * (value - 128) / 128);

set_saturation_val:
	api.type = TSCENE_MODES;
	api.dir = COMMAND_SET;
	api.id = SATURATION_STRENGTH_ID;
	api.value = value;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	return ret;
}

static inline int apical_isp_bright_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	TXispPrivParamManage *param = core->param;
	struct v4l2_mbus_framefmt *mbus = &core->vin.mbus;
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	unsigned int value = 128;
	unsigned int mid_node = 128;
	int ret = ISP_SUCCESS;

	if(mbus->code == V4L2_MBUS_FMT_YUYV8_1X16){
		return ret;
	}
	/* the original value */
	value = tuning->ctrls.brightness->val & 0xff;

	if(param == NULL)
		goto set_bright_val;

	if(tuning->ctrls.daynight == ISP_CORE_RUNING_MODE_DAY_MODE){
			mid_node = param->customer[TX_ISP_PRIV_PARAM_DAY_MODE].brightness;
	}else{
			mid_node = param->customer[TX_ISP_PRIV_PARAM_NIGHT_MODE].brightness;
	}

	/* the amended value */
	value = value < 128 ? (value * mid_node / 128) : (mid_node + (0xff - mid_node) * (value - 128) / 128);

set_bright_val:
	api.type = TSCENE_MODES;
	api.dir = COMMAND_SET;
	api.id = BRIGHTNESS_STRENGTH_ID;
	api.value = value;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	return ret;
}

static inline int apical_isp_contrast_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	TXispPrivParamManage *param = core->param;
	struct v4l2_mbus_framefmt *mbus = &core->vin.mbus;
	unsigned char (*curves)[2] = NULL;
	unsigned int value = 128;
	int x1, x2;
	unsigned int y1,y2,v1,v2;
	unsigned int total_gain;
	apical_api_control_t api;
	int i = 0;
	unsigned char status = 0;
	int reason = 0;
	int ret = ISP_SUCCESS;

	if(mbus->code == V4L2_MBUS_FMT_YUYV8_1X16){
		return ret;
	}

	value = tuning->ctrls.contrast->val & 0xff;

	if(param == NULL)
		goto set_contrast_val;
	/* total_gain format 27.5; for example, the value is 170, is '101.01010'b, is 2^5 + (10 / 32), is 32.2125x */
	total_gain =  stab.global_sensor_analog_gain  + stab.global_sensor_digital_gain + stab.global_isp_digital_gain;

	total_gain = math_exp2(total_gain,5,5);

	if(tuning->ctrls.daynight == ISP_CORE_RUNING_MODE_DAY_MODE){
			curves = param->customer[TX_ISP_PRIV_PARAM_DAY_MODE].contrast;
	}else{
			curves = param->customer[TX_ISP_PRIV_PARAM_NIGHT_MODE].contrast;
	}

	/* When curve[0][0] == 0xff, the curves is invalid. */
	if(curves[0][0] != 0xff){
		for(i = 0; i < CONTRAST_CURVES_MAXNUM; i++){
			if((curves[i][0] == 0xff) || ((curves[i][0] << 5) >= total_gain))
				break;
		}
		if(curves[i][0] == 0xff){
			x1 = i - 1;
			x2 = i - 1;
		}else{
			x2 = i;
			x1 = x2 - 1 < 0 ? 0 : x2 - 1;
		}

		if(x1 == x2){
			y1 = curves[x1][1];
			value = value < 128 ? (value * y1 / 128) : (y1 + (0xff - y1) * (value - 128) / 128);
		}else{
			y1 = curves[x1][1];
			y2 = curves[x2][1];
			v1 = value < 128 ? (value * y1 / 128) : (y1 + (0xff - y1) * (value - 128) / 128);
			v2 = value < 128 ? (value * y2 / 128) : (y2 + (0xff - y2) * (value - 128) / 128);

			x1 = curves[x1][0];
			x2 = curves[x2][0];
			x1 = x1 << 5;
			x2 = x2 << 5;

			value = ((total_gain - x1) * v2 + (x2 - total_gain) * v1) / (x2 - x1);
		}
	}
set_contrast_val:
	api.type = TSCENE_MODES;
	api.dir = COMMAND_SET;
	api.id = CONTRAST_STRENGTH_ID;
	api.value = value;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	return ret;
}

static inline int apical_isp_resolution_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
//	struct video_device *video = core->tun;
//	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
//	struct v4l2_ctrl *res = tuning->ctrls.resolution;
//	apical_api_control_t api;
//	unsigned char status = 0;
	int ret = ISP_SUCCESS;
	return ret;
}

static inline int apical_isp_fps_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_private_ioctl ioctl;
	struct tx_isp_notify_argument arg;
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	unsigned int fps = 0;
	copy_from_user(&fps, (const void __user *)control->value, sizeof(unsigned int));
	if(fps != core->vin.fps){
		ioctl.dir = TX_ISP_PRIVATE_IOCTL_SET;
		ioctl.cmd = TX_ISP_PRIVATE_IOCTL_SENSOR_FPS;
		ioctl.value = fps;

		arg.value = (int)&ioctl;
		tx_isp_sd_notify(tuning->parent, TX_ISP_NOTIFY_PRIVATE_IOCTL, &arg);
		api.type = TIMAGE;
		api.dir = COMMAND_SET;
		api.id = SENSOR_FPS_MODE_ID;
		api.value = FPS25;
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	}
	return 0;
}

static inline int apical_isp_fps_g_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
//	struct video_device *video = core->tun;
//	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	unsigned int fps = core->vin.fps;
	copy_to_user((void __user *)control->value, (const void *)&fps, sizeof(unsigned int));

	return ISP_SUCCESS;
}

static inline int apical_isp_fc_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_false_color_attr *attr = &tuning->ctrls.fc_attr;

	return ISP_SUCCESS;
	copy_from_user(attr, (const void __user *)control->value, sizeof(*attr));
	apical_isp_demosaic_fc_slope_write(attr->strength);
	apical_isp_demosaic_fc_alias_slope_write(attr->alias_strength);
	apical_isp_demosaic_fc_alias_thresh_write(attr->alias_thresh);
	return ISP_SUCCESS;
}

static inline int apical_isp_sharp_s_control(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	TXispPrivParamManage *param = core->param;
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	unsigned int value = 128;
	unsigned int mid_node = 128;
	int ret = ISP_SUCCESS;

	/* the original value */
	value = tuning->ctrls.sharpness->val & 0xff;

	if(param == NULL)
		goto set_sharpness_val;

	if(tuning->ctrls.daynight == ISP_CORE_RUNING_MODE_DAY_MODE){
			mid_node = param->customer[TX_ISP_PRIV_PARAM_DAY_MODE].sharpness;
	}else{
			mid_node = param->customer[TX_ISP_PRIV_PARAM_NIGHT_MODE].sharpness;
	}

	/* the amended value */
	value = value < 128 ? (value * mid_node / 128) : (mid_node + (0xff - mid_node) * (value - 128) / 128);

set_sharpness_val:
	api.type = TSCENE_MODES;
	api.dir = COMMAND_SET;
	api.id = SHARPENING_STRENGTH_ID;
	api.value = value;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	return ret;
}

static inline int apical_isp_sharp_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_sharpness_attr *attr = &tuning->ctrls.sharp_attr;

	copy_from_user(attr, (const void __user *)control->value, sizeof(*attr));
#if 1
	apical_isp_fr_sharpen_strength_write(attr->target_sharp);
	apical_isp_ds1_sharpen_strength_write(attr->target_sharp);
	apical_isp_ds2_sharpen_strength_write(attr->target_sharp);
#else
	apical_isp_demosaic_sharp_alt_d_write(attr->demo_strength_d);
	apical_isp_demosaic_sharp_alt_ud_write(attr->demo_strength_ud);
	apical_isp_demosaic_lum_thresh_write(attr->demo_threshold);
	apical_isp_fr_sharpen_enable_write(attr->fr_enable);
	apical_isp_ds1_sharpen_enable_write(attr->ds1_enable);
	apical_isp_ds2_sharpen_enable_write(attr->ds2_enable);
	if(attr->fr_enable){
		apical_isp_fr_sharpen_strength_write(attr->fr_strength);
	}
	if(attr->ds1_enable){
		apical_isp_ds1_sharpen_strength_write(attr->ds1_strength);
	}
	if(attr->ds2_enable){
		apical_isp_ds2_sharpen_strength_write(attr->ds2_strength);
	}
#endif
	return ISP_SUCCESS;
}

static inline int apical_isp_demosaic_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct isp_core_demosaic_attr *attr = &tuning->ctrls.demo_attr;

	copy_from_user(attr, (const void __user *)control->value, sizeof(*attr));
	apical_isp_demosaic_vh_slope_write(attr->vh_slope);
	apical_isp_demosaic_aa_slope_write(attr->aa_slope);
	apical_isp_demosaic_va_slope_write(attr->va_slope);
	apical_isp_demosaic_uu_slope_write(attr->uu_slope);
	apical_isp_demosaic_sat_slope_write(attr->sat_slope);
	apical_isp_demosaic_vh_thresh_write(attr->vh_thresh);
	apical_isp_demosaic_aa_thresh_write(attr->aa_thresh);
	apical_isp_demosaic_va_thresh_write(attr->va_thresh);
	apical_isp_demosaic_uu_thresh_write(attr->uu_thresh);
	apical_isp_demosaic_sat_thresh_write(attr->sat_thresh);
	apical_isp_demosaic_vh_offset_write(attr->vh_offset);
	apical_isp_demosaic_aa_offset_write(attr->aa_offset);
	apical_isp_demosaic_va_offset_write(attr->va_offset);
	apical_isp_demosaic_uu_offset_write(attr->uu_offset);
	apical_isp_demosaic_sat_offset_write(attr->sat_offset);
	apical_isp_demosaic_dmsc_config_write(attr->dmsc_config);

	return ISP_SUCCESS;
}

static inline int af_status_apical_to_v4l2(unsigned int val)
{
	int ret = ISP_SUCCESS;
	switch(val){
		case AF_NOT_FOCUSED:
			ret = V4L2_AUTO_FOCUS_STATUS_IDLE;
			break;
		case AF_SUCCESS:
		case AF_LOCKED:
			ret = V4L2_AUTO_FOCUS_STATUS_REACHED;
			break;
		case AF_FAIL:
			ret = V4L2_AUTO_FOCUS_STATUS_FAILED;
			break;
		case AF_RUNNING:
			ret = V4L2_AUTO_FOCUS_STATUS_BUSY;
			break;
		default:
			ret = -ISP_ERROR;
			break;
	}
	return ret;
}
static inline int apical_isp_af_g_status(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct v4l2_ctrl *af_status = tuning->ctrls.af_status;
	apical_api_control_t api;
	unsigned char status = 0;
	unsigned int ret_value = 0;
	int ret = ISP_SUCCESS;

	api.type = TALGORITHMS;
	api.dir = COMMAND_GET;
	api.id = AF_STATUS_ID;
	api.value = 0;
	status = apical_command(api.type, api.id, api.value, api.dir, &ret_value);
	af_status->val = af_status_apical_to_v4l2(ret_value);
	return ret;
}

static int apical_isp_gamma_g_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	int ret = ISP_SUCCESS;
	struct isp_core_gamma_attr attr;
	int i = 0;

	for (i = 0; i < 129; i++) {
		attr.gamma[i] = APICAL_READ_32(0x10400+4*i);
	}
	copy_to_user((void __user*)control->value, (const void *)&attr, sizeof(attr));
	return ret;
}

static int apical_isp_stab_g_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct isp_core_stab_attr stab_attr;
	memset(&stab_attr, 0, sizeof(stab_attr));
	memcpy(&stab_attr.stab, &stab, sizeof(stab));
	copy_to_user((void __user*)control->value, &stab_attr, sizeof(stab_attr));
	return 0;
}

#define SYSTEM_TAB_ITEM(x) if (stab_attr.stab_ctrl.ctrl_##x) stab.x = stab_attr.stab.x
static int apical_isp_stab_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct isp_core_stab_attr stab_attr;
	copy_from_user(&stab_attr, (const void __user*)control->value, sizeof(stab_attr));
	SYSTEM_TAB_ITEM(global_freeze_firmware);
	SYSTEM_TAB_ITEM(global_manual_exposure);
	SYSTEM_TAB_ITEM(global_manual_exposure_ratio);
	SYSTEM_TAB_ITEM(global_manual_integration_time);
	SYSTEM_TAB_ITEM(global_manual_sensor_analog_gain);
	SYSTEM_TAB_ITEM(global_manual_sensor_digital_gain);
	SYSTEM_TAB_ITEM(global_manual_isp_digital_gain);
	SYSTEM_TAB_ITEM(global_manual_directional_sharpening);
	SYSTEM_TAB_ITEM(global_manual_un_directional_sharpening);
	SYSTEM_TAB_ITEM(global_manual_iridix);
	SYSTEM_TAB_ITEM(global_manual_sinter);
	SYSTEM_TAB_ITEM(global_manual_temper);
	SYSTEM_TAB_ITEM(global_manual_awb);
	SYSTEM_TAB_ITEM(global_antiflicker_enable);
	SYSTEM_TAB_ITEM(global_slow_frame_rate_enable);
	SYSTEM_TAB_ITEM(global_manual_saturation);
	SYSTEM_TAB_ITEM(global_manual_exposure_time);
	SYSTEM_TAB_ITEM(global_exposure_dark_target);
	SYSTEM_TAB_ITEM(global_exposure_bright_target);
	SYSTEM_TAB_ITEM(global_exposure_ratio);
	SYSTEM_TAB_ITEM(global_max_exposure_ratio);
	SYSTEM_TAB_ITEM(global_integration_time);
	SYSTEM_TAB_ITEM(global_max_integration_time);
	SYSTEM_TAB_ITEM(global_sensor_analog_gain);
	SYSTEM_TAB_ITEM(global_max_sensor_analog_gain);
	SYSTEM_TAB_ITEM(global_sensor_digital_gain);
	SYSTEM_TAB_ITEM(global_max_sensor_digital_gain);
	SYSTEM_TAB_ITEM(global_isp_digital_gain);
	SYSTEM_TAB_ITEM(global_max_isp_digital_gain);
	SYSTEM_TAB_ITEM(global_directional_sharpening_target);
	SYSTEM_TAB_ITEM(global_maximum_directional_sharpening);
	SYSTEM_TAB_ITEM(global_minimum_directional_sharpening);
	SYSTEM_TAB_ITEM(global_un_directional_sharpening_target);
	SYSTEM_TAB_ITEM(global_maximum_un_directional_sharpening);
	SYSTEM_TAB_ITEM(global_minimum_un_directional_sharpening);
	SYSTEM_TAB_ITEM(global_iridix_strength_target);
	SYSTEM_TAB_ITEM(global_maximum_iridix_strength);
	SYSTEM_TAB_ITEM(global_minimum_iridix_strength);
	SYSTEM_TAB_ITEM(global_sinter_threshold_target);
	SYSTEM_TAB_ITEM(global_maximum_sinter_strength);
	SYSTEM_TAB_ITEM(global_minimum_sinter_strength);
	SYSTEM_TAB_ITEM(global_temper_threshold_target);
	SYSTEM_TAB_ITEM(global_maximum_temper_strength);
	SYSTEM_TAB_ITEM(global_minimum_temper_strength);
	SYSTEM_TAB_ITEM(global_awb_red_gain);
	SYSTEM_TAB_ITEM(global_awb_blue_gain);
	SYSTEM_TAB_ITEM(global_saturation_target);
	SYSTEM_TAB_ITEM(global_anti_flicker_frequency);
	SYSTEM_TAB_ITEM(global_ae_compensation);
	SYSTEM_TAB_ITEM(global_calibrate_bad_pixels);
	SYSTEM_TAB_ITEM(global_dis_x);
	SYSTEM_TAB_ITEM(global_dis_y);
	return 0;
}

static int apical_isp_gamma_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	int ret = ISP_SUCCESS;
	struct isp_core_gamma_attr attr;

	if (0 == control->value) {
		apical_api_calibration(CALIBRATION_GAMMA_LINEAR, COMMAND_GET, attr.gamma, sizeof(attr.gamma), &ret);
		if (ret != ISP_SUCCESS)
			goto err_get_def_gamma;
	} else {
		copy_from_user(&attr, (const void __user*)control->value, sizeof(attr));
	}
	apical_api_calibration(CALIBRATION_GAMMA_LINEAR, COMMAND_SET, attr.gamma, sizeof(attr.gamma), &ret);
		if (ret != ISP_SUCCESS)
			goto err_set_def_gamma;
	return ret;

err_set_def_gamma:
err_get_def_gamma:
	return ret;
}


static int apical_isp_ae_comp_s_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	unsigned char status = 0;
	int reason = 0;

	apical_api_control_t api;
	api.type = TALGORITHMS;
	api.dir = COMMAND_SET;

	api.id = AE_COMPENSATION_ID;
	api.value = control->value;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static int apical_isp_ae_comp_g_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	unsigned char status = 0;
	int reason = 0;
	apical_api_control_t api;
	api.type = TALGORITHMS;
	api.dir = COMMAND_GET;

	api.id = AE_COMPENSATION_ID;
	api.value = -1;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		return -1;
	}
	control->value = reason;

	return 0;
}

static int apical_isp_expr_s_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct tx_isp_sensor_attribute *attr = core->vin.attr;
	union isp_core_expr_attr expr_attr;
	apical_api_control_t api;
	int mode = 0;
	unsigned int integration_time = 0;

	unsigned char status = 0;
	int reason = 0;

	copy_from_user(&expr_attr, (const void __user*)control->value, sizeof(expr_attr));

	if (expr_attr.s_attr.mode == ISP_CORE_EXPR_MODE_AUTO) {
		mode = 0;
	} else {
		mode = 1;
	}

	if ((expr_attr.s_attr.unit == ISP_CORE_EXPR_UNIT_US)&&(1 == mode)) {
		if (attr->one_line_expr_in_us != 0)
			integration_time = expr_attr.s_attr.time/attr->one_line_expr_in_us;
		else {
			printk("err: %s,%d one_line_expr_in_us = %d \n", __func__, __LINE__, attr->one_line_expr_in_us);
			goto err_one_line_expr_in_us;
		}
	}

	api.type = TSYSTEM;
	api.dir = COMMAND_SET;
	api.id = SYSTEM_MANUAL_INTEGRATION_TIME;
	api.value = mode;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_set_expr_mode;
	}

	if (mode) {
		api.type = TSYSTEM;
		api.dir = COMMAND_SET;
		api.id = SYSTEM_INTEGRATION_TIME;
		api.value = integration_time;
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
		if (status != ISP_SUCCESS) {
			printk("err: %s,%d apical_command err \n", __func__, __LINE__);
			goto err_set_integration_time;
		}
	}
	return 0;

err_set_integration_time:
err_set_expr_mode:
err_one_line_expr_in_us:
	return -1;
}

static int apical_isp_expr_g_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct tx_isp_sensor_attribute *attr = core->vin.attr;
	union isp_core_expr_attr expr_attr;
	apical_api_control_t api;
	int mode = 0;
	unsigned int integration_time = 0;

	unsigned char status = 0;
	int reason = 0;

	api.type = TSYSTEM;
	api.dir = COMMAND_GET;
	api.id = SYSTEM_MANUAL_INTEGRATION_TIME;
	api.value = mode;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_get_expr_mode;
	}
	mode = reason;

	api.type = TSYSTEM;
	api.dir = COMMAND_GET;
	api.id = SYSTEM_INTEGRATION_TIME;
	api.value = integration_time;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_get_integration_time;
	}
	integration_time = reason;

	expr_attr.g_attr.mode = (0 == mode)?ISP_CORE_EXPR_MODE_AUTO:ISP_CORE_EXPR_MODE_MANUAL;
	expr_attr.g_attr.integration_time = integration_time;
	expr_attr.g_attr.integration_time_min = attr->min_integration_time;
	expr_attr.g_attr.integration_time_max = attr->max_integration_time;
	expr_attr.g_attr.one_line_expr_in_us = attr->one_line_expr_in_us;

	copy_to_user((void __user*)control->value, &expr_attr, sizeof(expr_attr));

	return 0;

err_get_integration_time:
err_get_expr_mode:
	return 0;
}

static int apical_isp_ae_g_roi(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;

	api.type = TALGORITHMS;
	api.dir = COMMAND_GET;
	api.id = AE_ROI_ID;
	api.value = 0;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	control->value = reason;
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		return -1;
	}
	return 0;
}

static int apical_isp_ae_s_roi(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;

	api.type = TALGORITHMS;
	api.dir = COMMAND_SET;
	api.id = AE_ROI_ID;
	api.value = control->value;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		return -1;
	}
	return 0;
}

static int isp_wb_mode_to_apical(enum isp_core_wb_mode mode)
{
	int apical_mode = -1;
	switch(mode) {
	case ISP_CORE_WB_MODE_AUTO:
		apical_mode = AWB_AUTO;
		break;
	case ISP_CORE_WB_MODE_MANUAL:
		apical_mode = AWB_MANUAL;
		break;
	case ISP_CORE_WB_MODE_DAY_LIGHT:
		apical_mode = AWB_DAY_LIGHT;
		break;
	case ISP_CORE_WB_MODE_CLOUDY:
		apical_mode = AWB_CLOUDY;
		break;
	case ISP_CORE_WB_MODE_INCANDESCENT:
		apical_mode = AWB_INCANDESCENT;
		break;
	case ISP_CORE_WB_MODE_FLOURESCENT:
		apical_mode = AWB_FLOURESCENT;
		break;
	case ISP_CORE_WB_MODE_TWILIGHT:
		apical_mode = AWB_TWILIGHT;
		break;
	case ISP_CORE_WB_MODE_SHADE:
		apical_mode = AWB_SHADE;
		break;
	case ISP_CORE_WB_MODE_WARM_FLOURESCENT:
		apical_mode = AWB_WARM_FLOURESCENT;
		break;
	default:
		break;
	}
	return apical_mode;
}

static int apical_wb_mode_to_isp(int apical_mode)
{
	int isp_mode = -1;
	switch(apical_mode) {
	case AWB_AUTO:
		isp_mode = ISP_CORE_WB_MODE_AUTO;
		break;
	case AWB_MANUAL:
		isp_mode = ISP_CORE_WB_MODE_MANUAL;
		break;
	case AWB_DAY_LIGHT:
		isp_mode = ISP_CORE_WB_MODE_DAY_LIGHT;
		break;
	case AWB_CLOUDY:
		isp_mode = ISP_CORE_WB_MODE_CLOUDY;
		break;
	case AWB_INCANDESCENT:
		isp_mode = ISP_CORE_WB_MODE_INCANDESCENT;
		break;
	case AWB_FLOURESCENT:
		isp_mode = ISP_CORE_WB_MODE_FLOURESCENT;
		break;
	case AWB_TWILIGHT:
		isp_mode = ISP_CORE_WB_MODE_TWILIGHT;
		break;
	case AWB_SHADE:
		isp_mode = ISP_CORE_WB_MODE_SHADE;
		break;
	case AWB_WARM_FLOURESCENT:
		isp_mode = ISP_CORE_WB_MODE_WARM_FLOURESCENT;
		break;
	default:
		break;
	}
	return isp_mode;
}

static int apical_isp_wb_s_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct isp_core_wb_attr wb_attr;
	apical_api_control_t api;
	int apical_mode = 0;
	int manual = 0;

	unsigned char status = 0;
	int reason = 0;

	copy_from_user(&wb_attr, (const void __user*)control->value, sizeof(wb_attr));

	apical_mode = isp_wb_mode_to_apical(wb_attr.mode);
	if (-1 == apical_mode) {
		printk("err: %s,%d mode = %d \n", __func__, __LINE__, wb_attr.mode);
		goto err_mode;
	}
	if (wb_attr.mode == ISP_CORE_WB_MODE_MANUAL) {
		manual = 1;
	} else {
		manual = 0;
	}
	api.type = TALGORITHMS;
	api.dir = COMMAND_SET;
	api.id = AWB_MODE_ID;
	api.value = apical_mode;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_set_wb_mode;
	}
	if (manual) {
		api.type = TSYSTEM;
		api.dir = COMMAND_SET;
		api.id = SYSTEM_AWB_RED_GAIN;
		api.value = wb_attr.rgain;
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
		if (status != ISP_SUCCESS) {
			printk("err: %s,%d apical_command err \n", __func__, __LINE__);
			goto err_set_wb_rgain;
		}
		api.type = TSYSTEM;
		api.dir = COMMAND_SET;
		api.id = SYSTEM_AWB_BLUE_GAIN;
		api.value = wb_attr.bgain;
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
		if (status != ISP_SUCCESS) {
			printk("err: %s,%d apical_command err \n", __func__, __LINE__);
			goto err_set_wb_bgain;
		}
	}
	return 0;

err_set_wb_bgain:
err_set_wb_rgain:
err_set_wb_mode:
err_mode:
	return -1;
}

static int apical_isp_wb_statis_g_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct isp_core_wb_attr wb_attr;

	wb_attr.rgain = apical_isp_metering_awb_rg_read();
	wb_attr.bgain = apical_isp_metering_awb_bg_read();
	control->value = (wb_attr.rgain << 16) + wb_attr.bgain;
	return 0;
}

static int apical_isp_wb_g_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct isp_core_wb_attr wb_attr;
	apical_api_control_t api;

	unsigned char status = 0;
	int reason = 0;
	int isp_mode = 0;

	api.type = TALGORITHMS;
	api.dir = COMMAND_GET;
	api.id = AWB_MODE_ID;
	api.value = -1;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_get_wb_mode;
	}
	isp_mode = apical_wb_mode_to_isp(reason);
	if (-1 == isp_mode) {
		printk("err: %s,%d isp wb mode :%d  \n", __func__, __LINE__, isp_mode);
		goto err_get_wb_mode;
	}
	wb_attr.mode = isp_mode;

	{
		api.type = TSYSTEM;
		api.dir = COMMAND_GET;
		api.id = SYSTEM_AWB_RED_GAIN;
		api.value = -1;
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
		if (status != ISP_SUCCESS) {
			printk("err: %s,%d apical_command err \n", __func__, __LINE__);
			goto err_get_wb_rgain;
		}
		wb_attr.rgain = reason;

		api.type = TSYSTEM;
		api.dir = COMMAND_GET;
		api.id = SYSTEM_AWB_BLUE_GAIN;
		api.value = -1;
		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
		if (status != ISP_SUCCESS) {
			printk("err: %s,%d apical_command err \n", __func__, __LINE__);
			goto err_get_wb_bgain;
		}
		wb_attr.bgain = reason;
	}
	copy_to_user((void __user*)control->value, &wb_attr, sizeof(wb_attr));

	return 0;

err_get_wb_bgain:
err_get_wb_rgain:
err_get_wb_mode:
	return -1;
}

static int apical_isp_max_again_s_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	unsigned char status = 0;
	int reason = 0;

	apical_api_control_t api;
	api.type = TSYSTEM;
	api.dir = COMMAND_SET;

	api.id = SYSTEM_MAX_SENSOR_ANALOG_GAIN;
	api.value = control->value;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_set_max_again_bgain;
	}

	return 0;
err_set_max_again_bgain:
	return 0;
}

static int apical_isp_max_again_g_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	unsigned char status = 0;
	int reason = 0;
	apical_api_control_t api;
	api.type = TSYSTEM;
	api.dir = COMMAND_GET;

	api.id = SYSTEM_MAX_SENSOR_ANALOG_GAIN;
	api.value = -1;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_get_max_again_bgain;
	}
	control->value = reason;

	return 0;
err_get_max_again_bgain:
	return 0;
}

static int apical_isp_max_dgain_s_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	unsigned char status = 0;
	int reason = 0;

	apical_api_control_t api;
	api.type = TSYSTEM;
	api.dir = COMMAND_SET;

	api.id = SYSTEM_MAX_ISP_DIGITAL_GAIN;
	api.value = control->value;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_set_max_dgain;
	}

	return 0;
err_set_max_dgain:
	return 0;
}

static int apical_isp_max_dgain_g_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	unsigned char status = 0;
	int reason = 0;
	apical_api_control_t api;
	api.type = TSYSTEM;
	api.dir = COMMAND_GET;

	api.id = SYSTEM_MAX_ISP_DIGITAL_GAIN;
	api.value = -1;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_get_max_dgain;
	}
	control->value = reason;

	return 0;
err_get_max_dgain:
	return 0;
}

static int apical_isp_hi_light_depress_s_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	unsigned char status = 0;
	int ret = 0;
	struct video_device *video = core->tun;
	TXispPrivParamManage *param = core->param;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct image_tuning_ctrls *ctrls = &(tuning->ctrls);
	int strength = control->value;

	struct v4l2_ctrl *wdr = tuning->ctrls.wdr;

	LookupTable** table = NULL;
	ISP_CORE_MODE_DN_E dn;
	unsigned int rows = 0;
	unsigned int cols = 0;
	unsigned int width = 0;
	unsigned int size = 0;
	void *data = 0;

	if(wdr->val == ISPCORE_MODULE_ENABLE){
		printk("err: wdr enabled\n");
		return -1;
	}

	dn = ctrls->daynight;
	if (NULL == param) {
		printk("err: param == NULL\n");
		return -1;
	}
	table = param->isp_param[dn].calibrations;

	rows = table[_CALIBRATION_AE_BALANCED_LINEAR]->rows;
	cols = table[_CALIBRATION_AE_BALANCED_LINEAR]->cols;
	width = table[_CALIBRATION_AE_BALANCED_LINEAR]->width;
	size = rows*cols*width;

	data = kzalloc(size, GFP_KERNEL);
	if(!data){
		printk("err: Failed to allocate isp table mem\n");
		return -1;
	}
	memcpy(data, table[_CALIBRATION_AE_BALANCED_LINEAR]->ptr, size);
	if (1 == width) {
		uint8_t *v = data;
		v[2] = strength;
	} else if (2 == width) {
		uint16_t *v = data;
		v[2] = strength;
	} else if (4 == width) {
		uint32_t *v = data;
		v[2] = strength;
	} else {
		printk("err: %s(%d),format error !\n", __func__, __LINE__);
		kfree(data);
		return -1;
	}

	status = apical_api_calibration(CALIBRATION_AE_BALANCED_LINEAR, COMMAND_SET, data, size, &ret);
	if (0 != ret) {
		kfree(data);
		printk("err: %s,%d, status = %d, ret = %d\n", __func__, __LINE__, status, ret);
		return -1;
	}
	kfree(data);

	return 0;
}

static int apical_isp_hi_light_depress_g_ctrl(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	unsigned char status = 0;
	struct video_device *video = core->tun;
	TXispPrivParamManage *param = core->param;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct image_tuning_ctrls *ctrls = &(tuning->ctrls);
	int ret = 0;

	struct v4l2_ctrl *wdr = tuning->ctrls.wdr;

	LookupTable** table = NULL;
	ISP_CORE_MODE_DN_E dn;
	unsigned int rows = 0;
	unsigned int cols = 0;
	unsigned int width = 0;
	unsigned int size = 0;
	void *data = 0;

	if(wdr->val == ISPCORE_MODULE_ENABLE){
		printk("err: wdr enabled\n");
		return -1;
	}

	dn = ctrls->daynight;
	if (NULL == param) {
		printk("err: param == NULL\n");
		return -1;
	}
	table = param->isp_param[dn].calibrations;

	rows = table[_CALIBRATION_AE_BALANCED_LINEAR]->rows;
	cols = table[_CALIBRATION_AE_BALANCED_LINEAR]->cols;
	width = table[_CALIBRATION_AE_BALANCED_LINEAR]->width;
	size = rows*cols*width;

	data = kzalloc(size, GFP_KERNEL);
	if(!data){
		printk("err: Failed to allocate isp table mem\n");
		return -1;
	}

	status = apical_api_calibration(CALIBRATION_AE_BALANCED_LINEAR, COMMAND_GET, data, size, &ret);
	if (0 != ret)
		printk("err: %s,%d, status = %d, ret = %d\n", __func__, __LINE__, status, ret);

	if (1 == width) {
		uint8_t *v = data;
		control->value = v[2];
	} else if (2 == width) {
		uint16_t *v = data;
		control->value = v[2];
	} else if (4 == width) {
		uint32_t *v = data;
		control->value = v[2];
	} else {
		printk("err: %s(%d),format error !\n", __func__, __LINE__);
		kfree(data);
		return -1;
	}
	kfree(data);

	return 0;
}

struct isp_table_info {
	unsigned int id;
	unsigned int rows;
	unsigned int cols;
	unsigned int width;
	unsigned int tsource;
	void *ptr;
};

static int apical_isp_table_g_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	TXispPrivParamManage *param = core->param;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct image_tuning_ctrls *ctrls = &(tuning->ctrls);

	int ret = ISP_SUCCESS;
	unsigned int status = 0;
	LookupTable** table = NULL;
	ISP_CORE_MODE_DN_E dn;

	unsigned int rows = 0;
	unsigned int cols = 0;
	unsigned int width = 0;
	unsigned int size = 0;
	unsigned int id = 0;
	unsigned int tid = 0;
	struct isp_table_info tinfo;
	void *data = 0;

	dn = ctrls->daynight;
	if (NULL == param) {
		goto err_isp_param;
	}
	table = param->isp_param[dn].calibrations;
	copy_from_user(&tinfo, (const void __user*)control->value, sizeof(tinfo));
	id = tinfo.id;

	switch(id) {
		case CALIBRATION_TEMPER_STRENGTH:
			rows = table[_CALIBRATION_TEMPER_STRENGTH]->rows;
			cols = table[_CALIBRATION_TEMPER_STRENGTH]->cols;
			width = table[_CALIBRATION_TEMPER_STRENGTH]->width;
			tid = _CALIBRATION_TEMPER_STRENGTH;
			break;
		case CALIBRATION_SINTER_STRENGTH_LINEAR:
			rows = table[_CALIBRATION_SINTER_STRENGTH_LINEAR]->rows;
			cols = table[_CALIBRATION_SINTER_STRENGTH_LINEAR]->cols;
			width = table[_CALIBRATION_SINTER_STRENGTH_LINEAR]->width;
			tid = _CALIBRATION_SINTER_STRENGTH_LINEAR;
			break;
		case CALIBRATION_DP_SLOPE_LINEAR:
			rows = table[_CALIBRATION_DP_SLOPE_LINEAR]->rows;
			cols = table[_CALIBRATION_DP_SLOPE_LINEAR]->cols;
			width = table[_CALIBRATION_DP_SLOPE_LINEAR]->width;
			tid = _CALIBRATION_DP_SLOPE_LINEAR;
			break;
		default:
			printk("%s,%d, err id: %d\n", __func__, __LINE__, id);
			ret = -EPERM;
			break;
	}
	tinfo.rows = rows;
	tinfo.cols = cols;
	tinfo.width = width;
	size = rows*cols*width;

	copy_to_user((void __user*)control->value, &tinfo, sizeof(tinfo));
	if (NULL != tinfo.ptr) {
		if (0 == tinfo.tsource) {
			data = kzalloc(size, GFP_KERNEL);
			if(!data){
				printk("Failed to allocate isp table mem\n");
				return -1;
			}
			status = apical_api_calibration(id, COMMAND_GET, tinfo.ptr, size, &ret);
			if (0 != ret)
				printk("%s,%d, status = %d, ret = %d\n", __func__, __LINE__, status, ret);
			copy_to_user((void __user*)tinfo.ptr, data, size);
			kfree(data);
		} else {
			copy_to_user((void __user*)tinfo.ptr, table[tid]->ptr, size);
		}
	}
	return 0;
err_isp_param:
	return -1;
}

static int apical_isp_table_s_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct video_device *video = core->tun;
	TXispPrivParamManage *param = core->param;
	image_tuning_vdrv_t *tuning = video_get_drvdata(video);
	struct image_tuning_ctrls *ctrls = &(tuning->ctrls);

	int ret = ISP_SUCCESS;
	unsigned int status = 0;
	LookupTable** table = NULL;
	ISP_CORE_MODE_DN_E dn;

	unsigned int rows = 0;
	unsigned int cols = 0;
	unsigned int width = 0;
	unsigned int size = 0;
	unsigned int id = 0;
	struct isp_table_info tinfo;
	void *data = 0;

	if (NULL == param) {
		goto err_isp_param;
	}
	dn = ctrls->daynight;
	table = param->isp_param[dn].calibrations;
	copy_from_user(&tinfo, (const void __user*)control->value, sizeof(tinfo));

	id = tinfo.id;
	rows = tinfo.rows;
	cols = tinfo.cols;
	width = tinfo.width;
	size = rows*cols*width;

	if (NULL != tinfo.ptr) {
		data = kzalloc(size, GFP_KERNEL);
		if(!data){
			printk("Failed to allocate isp table mem\n");
			return -1;
		}
		copy_from_user(data, (const void __user*)tinfo.ptr, size);
		status = apical_api_calibration(id, COMMAND_SET, data, size, &ret);
		if (0 != ret)
			printk("%s,%d, status = %d, ret = %d\n", __func__, __LINE__, status, ret);
		kfree(data);
	} else {
		printk("%s,%d, param err\n", __func__, __LINE__);
	}
	return 0;
err_isp_param:
	return -1;
}


static int apical_isp_ev_g_attr(struct tx_isp_core_device *core, struct v4l2_control *control)
{
	struct isp_core_ev_attr ev_attr;
	apical_api_control_t api;

	unsigned char status = 0;
	int reason = 0;


	api.type = TSYSTEM;
	api.dir = COMMAND_GET;

	api.id = SYSTEM_MANUAL_EXPOSURE_TIME;
	api.value = -1;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_get_ev;
	}
	ev_attr.ev = reason;

	api.id = SYSTEM_SENSOR_ANALOG_GAIN;
	api.value = -1;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_get_again;
	}
	ev_attr.again = reason;

	api.id = SYSTEM_ISP_DIGITAL_GAIN;
	api.value = -1;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_get_dgain;
	}
	ev_attr.dgain = reason;

	api.type = TALGORITHMS;
	api.dir = COMMAND_GET;
	api.id = AE_EXPOSURE_ID;
	api.value = -1;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_get_expr;
	}
	ev_attr.expr_us = reason;

	api.type = CALIBRATION;
	api.id = EXPOSURE_LOG2_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_get_ev_log2;
	}
	ev_attr.ev_log2 = reason;

	api.id = GAIN_LOG2_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if (status != ISP_SUCCESS) {
		printk("err: %s,%d apical_command err \n", __func__, __LINE__);
		goto err_get_gain_log2;
	}
	ev_attr.gain_log2 = reason;


	copy_to_user((void __user*)control->value, &ev_attr, sizeof(ev_attr));

	return 0;

err_get_dgain:
err_get_again:
err_get_expr:
err_get_ev:
err_get_ev_log2:
err_get_gain_log2:
	return -1;
}


static int apical_isp_core_ops_g_ctrl(struct tx_isp_core_device *core, struct v4l2_control *ctrl)
{
	int ret = ISP_SUCCESS;

	/* printk("%s[%d] ctrl->id = 0x%08x\n", __func__, __LINE__, ctrl->id); */
	switch(ctrl->id){
		case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
			break;
		case IMAGE_TUNING_CID_CUSTOM_AF_MODE:
			apical_isp_af_g_status(core, ctrl);
			break;
		case IMAGE_TUNING_CID_AWB_ATTR:
			break;
		case IMAGE_TUNING_CID_WB_STAINFO:
			break;
		case IMAGE_TUNING_CID_AE_ATTR:
			ret = apical_isp_ae_g_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_AE_STAINFO:
			break;
		case IMAGE_TUNING_CID_AF_ATTR:
			break;
		case IMAGE_TUNING_CID_AF_STAINFO:
			break;
        case IMAGE_TUNING_CID_TEMPER_STRENGTH:
			ret = apical_isp_temper_dns_g_strength(core, ctrl);
			break;
		case IMAGE_TUNING_CID_TEMPER_ATTR:
			ret = apical_isp_temper_dns_g_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_WDR_ATTR:
			break;
		case IMAGE_TUNING_CID_DIS_STAINFO:
			break;
		case IMAGE_TUNING_CID_FC_ATTR:
			break;
		case IMAGE_TUNING_CID_SHARP_ATTR:
			break;
		case IMAGE_TUNING_CID_DEMO_ATTR:
			break;
		case IMAGE_TUNING_CID_DRC_ATTR:
			ret = apical_isp_drc_g_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_SHAD_ATTR:
			break;
		case IMAGE_TUNING_CID_CONTROL_FPS:
			ret = apical_isp_fps_g_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_GET_TOTAL_GAIN:
			ret = apical_isp_g_totalgain(core, ctrl);
			break;
		case IMAGE_TUNING_CID_DAY_OR_NIGHT:
			ret = apical_isp_day_or_night_g_ctrl(core, ctrl);
			break;
	        case IMAGE_TUNING_CID_AE_STRATEGY:
			ret = apical_isp_ae_strategy_g_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_GAMMA_ATTR:
			ret = apical_isp_gamma_g_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_SYSTEM_TAB:
			ret = apical_isp_stab_g_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_EXPR_ATTR:
			ret = apical_isp_expr_g_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_AE_ROI:
			ret = apical_isp_ae_g_roi(core, ctrl);;
			break;
		case IMAGE_TUNING_CID_WB_ATTR:
			ret = apical_isp_wb_g_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_WB_STATIS_ATTR:
			ret = apical_isp_wb_statis_g_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_MAX_AGAIN_ATTR:
			ret = apical_isp_max_again_g_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_MAX_DGAIN_ATTR:
			ret = apical_isp_max_dgain_g_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_HILIGHT_DEPRESS_STRENGTH:
			ret = apical_isp_hi_light_depress_g_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_AE_COMP:
			ret = apical_isp_ae_comp_g_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_ISP_TABLE_ATTR:
			ret = apical_isp_table_g_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_ISP_EV_ATTR:
			ret = apical_isp_ev_g_attr(core, ctrl);
			break;
	    case IMAGE_TUNING_CID_AWB_CWF_SHIFT:
		    ret = apical_isp_awb_cwf_g_shift(core, ctrl);
		    break;
		default:
			ret = -EPERM;
			break;
	}
	return ret;
}

static int apical_isp_core_ops_s_ctrl(struct tx_isp_core_device *core, struct v4l2_control *ctrl)
{
	int ret = ISP_SUCCESS;
	switch (ctrl->id) {
		case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		case IMAGE_TUNING_CID_AWB_ATTR:
		case IMAGE_TUNING_CID_MWB_ATTR:
			/*ret = apical_isp_wb_s_control(core, ctrl);*/
			break;
		case V4L2_CID_EXPOSURE_AUTO:
			/*ret = apical_isp_ae_s_control(core, ctrl);*/
			break;
		case IMAGE_TUNING_CID_CUSTOM_AF_MODE:
		case IMAGE_TUNING_CID_AF_ATTR:
			/*ret = apical_isp_af_s_control(core, ctrl);*/
			break;
		case V4L2_CID_3A_LOCK:
			/*ret = apical_isp_3alock_s_control(core, ctrl);*/
			break;
		case V4L2_CID_HFLIP:
			ret = apical_isp_hflip_s_control(core, ctrl);
			break;
		case V4L2_CID_VFLIP:
			ret = apical_isp_vflip_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_SINTER_DNS:
			ret = apical_isp_sinter_dns_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_SINTER_ATTR:
			ret = apical_isp_sinter_dns_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_TEMPER_DNS:
			ret = apical_isp_temper_dns_s_control(core, ctrl);
			break;
        case IMAGE_TUNING_CID_TEMPER_STRENGTH:
			ret = apical_isp_temper_dns_s_strength(core, ctrl);
		    break;
		case IMAGE_TUNING_CID_TEMPER_ATTR:
			ret = apical_isp_temper_dns_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_WDR_FLT:
			ret = apical_isp_wdr_lut_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_NOISE_PROFILE_ATTR:
			ret = apical_isp_noise_profile_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_WDR:
			ret = apical_isp_wdr_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_WDR_ATTR:
			ret = apical_isp_wdr_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_ISP_PROCESS:
			ret = apical_isp_bypass_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_ISP_FREEZE:
			ret = apical_isp_freeze_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_TEST_PATTERN:
			ret = apical_isp_test_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_BL:
			ret = apical_isp_blacklevel_s_control(core, ctrl);
			break;
		case V4L2_CID_IMAGE_STABILIZATION:
			ret = apical_isp_dis_s_control(core, ctrl);
			break;
		case V4L2_CID_POWER_LINE_FREQUENCY:
			ret = apical_isp_flicker_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_SHAD:
			ret = apical_isp_lens_shad_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_SHAD_ATTR:
			ret = apical_isp_lens_shad_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_GE_ATTR:
			ret = apical_isp_ge_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_DYNAMIC_DP:
		case IMAGE_TUNING_CID_CUSTOM_GE:
			ret = apical_isp_dynamic_dp_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_DYNAMIC_DP_ATTR:
			ret = apical_isp_dynamic_dp_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_STATIC_DP:
			ret = apical_isp_static_dp_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_STATIC_DP_ATTR:
			ret = apical_isp_static_dp_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_ANTI_FOG:
			ret = apical_isp_antifog_s_control(core, ctrl);
			break;
		case V4L2_CID_SCENE_MODE:
			ret = apical_isp_scene_s_control(core, ctrl);
			break;
		case V4L2_CID_COLORFX:
			ret = apical_isp_colorfx_s_control(core, ctrl);
			break;
		case V4L2_CID_SATURATION:
			ret = apical_isp_sat_s_control(core, ctrl);
			break;
		case V4L2_CID_BRIGHTNESS:
			ret = apical_isp_bright_s_control(core, ctrl);
			break;
		case V4L2_CID_CONTRAST:
			ret = apical_isp_contrast_s_control(core, ctrl);
			break;
		case V4L2_CID_SHARPNESS:
			ret = apical_isp_sharp_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_SHARP_ATTR:
			ret = apical_isp_sharp_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_RESOLUTION:
			ret = apical_isp_resolution_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_DRC:
			ret = apical_isp_drc_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_DRC_ATTR:
			ret = apical_isp_drc_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_DEMO_ATTR:
			ret = apical_isp_demosaic_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_FC_ATTR:
			ret = apical_isp_fc_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_CONTROL_FPS:
			ret = apical_isp_fps_s_control(core, ctrl);
			break;
		case IMAGE_TUNING_CID_DAY_OR_NIGHT:
			ret = apical_isp_day_or_night_s_ctrl(core, ctrl);
			break;
	        case IMAGE_TUNING_CID_AE_STRATEGY:
			ret = apical_isp_ae_strategy_s_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_GAMMA_ATTR:
			ret = apical_isp_gamma_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_SYSTEM_TAB:
			ret = apical_isp_stab_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_EXPR_ATTR:
			ret = apical_isp_expr_s_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_AE_ROI:
			ret = apical_isp_ae_s_roi(core, ctrl);
			break;
		case IMAGE_TUNING_CID_WB_ATTR:
			ret = apical_isp_wb_s_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_MAX_AGAIN_ATTR:
			ret = apical_isp_max_again_s_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_MAX_DGAIN_ATTR:
			ret = apical_isp_max_dgain_s_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_HILIGHT_DEPRESS_STRENGTH:
			ret = apical_isp_hi_light_depress_s_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_AE_COMP:
			ret = apical_isp_ae_comp_s_ctrl(core, ctrl);
			break;
		case IMAGE_TUNING_CID_ISP_TABLE_ATTR:
			ret = apical_isp_table_s_attr(core, ctrl);
			break;
		case IMAGE_TUNING_CID_AWB_CWF_SHIFT:
			ret = apical_isp_awb_cwf_s_shift(core, ctrl);
			break;
		default:
			ret = -EPERM;
			break;
	}
	return ret;
}

int isp_core_ops_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct tx_isp_core_device *core = sd_to_tx_isp_core_device(sd);
	struct v4l2_device *v4l2_dev = core->sd.v4l2_dev;
	int ret = ISP_SUCCESS;

	if(atomic_read(&core->state) < TX_ISP_STATE_START){
		v4l2_err(v4l2_dev, "%s[%d] please enable video-in device firstly.\n",
				__func__,__LINE__);
		return -EPERM;
	}
	/* printk("%s[%d] ctrl->id = 0x%08x\n", __func__, __LINE__, ctrl->id); */
	ret = apical_isp_core_ops_g_ctrl(core, ctrl);
	return ret;
}

int isp_core_ops_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct tx_isp_core_device *core = sd_to_tx_isp_core_device(sd);
	struct v4l2_device *v4l2_dev = core->sd.v4l2_dev;
	int ret = ISP_SUCCESS;

	if(atomic_read(&core->state) < TX_ISP_STATE_START){
		v4l2_err(v4l2_dev, "%s[%d] please enable video-in device firstly.\n",
				__func__,__LINE__);
		return -EPERM;
	}
	ret = apical_isp_core_ops_s_ctrl(core, ctrl);
	return ret;
}


/*
* the video_device_ops
* its function is that setting these attributions of isp modules.
*/

static long image_tuning_vidioc_default(struct file *file, void *fh, bool valid_prio, unsigned int cmd, void *arg)
{
	image_tuning_vdrv_t *tuning = video_drvdata(file);
	struct tx_isp_core_device *core = sd_to_tx_isp_core_device(tuning->parent);
	struct isp_image_tuning_default_ctrl *ctrl;
	long ret = ISP_SUCCESS;

	/* ISP must be runing */
	if(atomic_read(&core->state) < TX_ISP_STATE_START){
		return -EPERM;
	}

	/* the file must be opened firstly. */
	if(atomic_read(&tuning->state) < TX_ISP_STATE_START){
		return -EPERM;
	}

	ctrl = (struct isp_image_tuning_default_ctrl *)arg;
	switch(cmd){
		case VIDIOC_DEFAULT_CMD_ISP_TUNING:
			if(ctrl->dir == TX_ISP_PRIVATE_IOCTL_SET)
				ret = v4l2_subdev_call(tuning->parent, core, s_ctrl, &ctrl->control);
			else
				ret = v4l2_subdev_call(tuning->parent, core, g_ctrl, &ctrl->control);
			break;
		default :
			ret = -EPERM;
			break;
	}
	return ret;
}

/* --- controls ---------------------------------------------- */

static inline int image_tuning_set_wb_ctrl(image_tuning_vdrv_t *tuning, struct v4l2_ctrl *ctrl)
{
	struct image_tuning_ctrls *ctrls = &tuning->ctrls;
	struct v4l2_ctrl *aefwb_lock = ctrls->aefwb_lock;
	struct v4l2_control control;

	if(aefwb_lock->cur.val & V4L2_LOCK_WHITE_BALANCE){
		return -EPERM;
	}

	control.id = ctrl->id;
	return v4l2_subdev_call(tuning->parent, core, s_ctrl, &control);
}

static inline int image_tuning_set_ae_ctrl(image_tuning_vdrv_t *tuning, struct v4l2_ctrl *ctrl)
{
	struct image_tuning_ctrls *ctrls = &tuning->ctrls;
	struct v4l2_ctrl *aefwb_lock = ctrls->aefwb_lock;
	struct v4l2_control control;

	if(aefwb_lock->cur.val & V4L2_LOCK_EXPOSURE){
		return -EPERM;
	}

	control.id = ctrl->id;
	return v4l2_subdev_call(tuning->parent, core, s_ctrl, &control);
}

static inline int image_tuning_set_af_ctrl(image_tuning_vdrv_t *tuning, struct v4l2_ctrl *ctrl)
{
	struct image_tuning_ctrls *ctrls = &tuning->ctrls;
	struct v4l2_ctrl *aefwb_lock = ctrls->aefwb_lock;
	struct v4l2_control control;

	if(aefwb_lock->cur.val & V4L2_LOCK_FOCUS){
		return -EPERM;
	}

	control.id = ctrl->id;
	return v4l2_subdev_call(tuning->parent, core, s_ctrl, &control);
}

static int image_tuning_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	image_tuning_vdrv_t *tuning = ctrl_to_image_tuning(ctrl);
	struct tx_isp_core_device *core = sd_to_tx_isp_core_device(tuning->parent);
	//	struct image_tuning_ctrls *ctrls = &tuning->ctrls;
	struct v4l2_control control;

	/* ISP must be runing */
	if(atomic_read(&core->state) < TX_ISP_STATE_START){
		return -EPERM;
	}
	/* the file must be opened firstly. */
	if(atomic_read(&tuning->state) < TX_ISP_STATE_START){
		return -EPERM;
	}

	//	printk("*** %s[%d] tuning = %p  id = 0x%08x ***\n", __func__,__LINE__, tuning, ctrl->id);
	control.id = ctrl->id;
	switch (ctrl->id) {
		case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		case IMAGE_TUNING_CID_CUSTOM_AF_MODE:
			v4l2_subdev_call(tuning->parent, core, g_ctrl, &control);
			break;
		default:
			break;
	}

	return 0;
}

static int image_tuning_s_ctrl(struct v4l2_ctrl *ctrl)
{
	image_tuning_vdrv_t *tuning = ctrl_to_image_tuning(ctrl);
	struct tx_isp_core_device *core = sd_to_tx_isp_core_device(tuning->parent);
	struct v4l2_control control;
	int ret = ISP_SUCCESS;

	/* ISP must be runing */
	if(atomic_read(&core->state) < TX_ISP_STATE_START){
		return -EPERM;
	}
	/* the file must be opened firstly. */
	if(atomic_read(&tuning->state) < TX_ISP_STATE_START){
		return -EPERM;
	}
	switch (ctrl->id) {
		case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
			ret = image_tuning_set_wb_ctrl(tuning, ctrl);
			break;
		case V4L2_CID_EXPOSURE_AUTO:
			ret = image_tuning_set_ae_ctrl(tuning, ctrl);
			break;
		case IMAGE_TUNING_CID_CUSTOM_AF_MODE:
			ret = image_tuning_set_af_ctrl(tuning, ctrl);
			break;
		default:
			control.id = ctrl->id;
			control.value = (int)ctrl;
			ret = v4l2_subdev_call(tuning->parent, core, s_ctrl, &control);
			break;
	}
	return ret;
}

/* ------------------------------------------------------------------
   File operations for the device
   ------------------------------------------------------------------*/

static const struct v4l2_ctrl_ops image_tuning_ctrl_ops = {
	.g_volatile_ctrl = image_tuning_g_volatile_ctrl,
	.s_ctrl = image_tuning_s_ctrl,
};

static const struct v4l2_ctrl_config ae_gain_ctrl_int32 = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_AE_GAIN,
	.name = "The gain of AE",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0x0,
	.max = 0x0000ffff,
	.step = 1,
	.def = 1 << 8,
};

static const struct v4l2_ctrl_config ae_compensation_ctrl_int32 = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_AE_COMP,
	.name = "The compensation of AE",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0x0,
	.max = 0x000000ff,
	.step = 1,
	.def = 128,
};

static const char * const af_mode_menu_strings[] = {
	"AF auto single",
	"AF auto continuous",
	"AF auto single macro",
	"AF auto continuous macro",
	"AF hyper focal",
	"AF infinity",
	"AF abort",
	NULL,
};

static const struct v4l2_ctrl_config af_mode_ctrl_menu = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_AF_MODE,
	.name = "AF mode",
	.type = V4L2_CTRL_TYPE_MENU,
	.min = 0,
	.max = 6,
	.def = 0,
	.menu_skip_mask = ~0x7f,
	.qmenu = af_mode_menu_strings,
};

static const char * const test_pattern_menu_strings[] = {
	"disable",
	"flat field",
	"horizontal gradient",
	"vertical gradient",
	"vertical bar",
	NULL,
};

static const struct v4l2_ctrl_config test_pattern_ctrl_menu = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_TEST_PATTERN,
	.name = "Test pattern",
	.type = V4L2_CTRL_TYPE_MENU,
	.min = 0,
	.max = 4,
	.def = 0,
	.menu_skip_mask = ~0x1f,
	.qmenu = test_pattern_menu_strings,
};

static const char * const fog_menu_strings[] = {
	"antifog disable",
	"antifog strong",
	"antifog medium",
	"antifog weak",
	NULL,
};

static const struct v4l2_ctrl_config fog_ctrl_menu = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_ANTI_FOG,
	.name = "Anti fog",
	.type = V4L2_CTRL_TYPE_MENU,
	.min = 0,
	.max = 3,
	.def = 0,
	.menu_skip_mask = ~0xf,
	.qmenu = fog_menu_strings,
};

static const struct v4l2_ctrl_config ispprocess_ctrl_bool = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_ISP_PROCESS,
	.name = "ISP Process",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 1,
};

static const struct v4l2_ctrl_config ispfreeze_ctrl_bool = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_ISP_FREEZE,
	.name = "Freeze FW",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config blacklevel_ctrl_bool = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_BL,
	.name = "Black level",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 1,
};

static const struct v4l2_ctrl_config shading_ctrl_bool = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_SHAD,
	.name = "Lens shading",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 1,
};

static const struct v4l2_ctrl_config ddp_ctrl_bool = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_DYNAMIC_DP,
	.name = "Dynamic Defect pixels",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 1,
};

static const struct v4l2_ctrl_config sdp_ctrl_bool = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_STATIC_DP,
	.name = "Static Defect pixels",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 1,
};


static const struct v4l2_ctrl_config sinter_ctrl_bool = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_SINTER_DNS,
	.name = "Sinter denoise",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 1,
};

static const struct v4l2_ctrl_config ge_ctrl_bool = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_GE,
	.name = "Green Equalist",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 1,
};

static const struct v4l2_ctrl_config wdr_lut_ctrl_bool = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_WDR_FLT,
	.name = "WDR Companded frontend LUT",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config wdr_ctrl_bool = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_WDR,
	.name = "Wide Dynamic Range",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
};

static const char * const temper_menu_strings[] = {
	"Disable Temper",
	"Auto Temper",
	"Manual set the strength of Temper",
	NULL,
};

static const struct v4l2_ctrl_config temper_ctrl_menu = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_TEMPER_DNS,
	.name = "Temper denoise",
	.type = V4L2_CTRL_TYPE_MENU,
	.min = 0,
	.max = 2,
	.def = 1,
	.menu_skip_mask = ~0x7,
	.qmenu = temper_menu_strings,
};

static const char * const drc_menu_strings[] = {
	"Manual set the strength of DRC",
	"Unlimit",
	"High",
	"Medium",
	"Low",
	"Disable DRC",
	NULL,
};

static const struct v4l2_ctrl_config drc_ctrl_menu = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_DRC,
	.name = "RAW Dynamic range compression",
	.type = V4L2_CTRL_TYPE_MENU,
	.min = 0,
	.max = 5,
	.def = 3,
	.menu_skip_mask = ~0x3f,
	.qmenu = drc_menu_strings,
};

static const char * const resolution_menu_strings[] = {
	"Full resolution, fps max",
	"Preview resolution, fps max",
	NULL,
};

static const struct v4l2_ctrl_config resolution_ctrl_menu = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_RESOLUTION,
	.name = "sensor resolution",
	.type = V4L2_CTRL_TYPE_MENU,
	.min = 0,
	.max = 2,
	.def = 0,
	.menu_skip_mask = ~0x3,
	.qmenu = resolution_menu_strings,
};

static const s64 fps_ctrl_int_menu_values[] = {
	25, 30,
};

static const struct v4l2_ctrl_config fps_ctrl_int32 = {
	.ops = &image_tuning_ctrl_ops,
	.id = IMAGE_TUNING_CID_CUSTOM_FPS,
	.name = "Tun FPS",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 5,
	.max = 30,
	.step = 1,
	.def = 25,
};


static const struct v4l2_ioctl_ops image_tuning_v4l2_ioctl_ops = {
	/* Priority handling */
	//	.vidioc_s_priority		= image_tuning_vidioc_s_priority,
	//	.vidioc_g_priority		= image_tuning_vidioc_g_priority,
	/* For other private ioctls */
	.vidioc_default			= image_tuning_vidioc_default,
};

static int image_tuning_v4l2_open(struct file *file)
{
	image_tuning_vdrv_t *tuning = video_drvdata(file);
	struct tx_isp_core_device *core = sd_to_tx_isp_core_device(tuning->parent);
	struct tx_isp_video_in *vin = &core->vin;
	int ret = ISP_SUCCESS;
	unsigned int tmp = 0;
	printk("###### %s %d #######\n",__func__,__LINE__);
	/* ISP must be runing */
	if(atomic_read(&core->state) < TX_ISP_STATE_START){
		return -EPERM;
	}

	if(atomic_read(&tuning->state) != TX_ISP_STATE_STOP){
		return -EBUSY;
	}
	tuning->wdr_buffer_size = vin->vi_max_width * vin->vi_max_height * 4;
	tuning->temper_buffer_size = vin->vi_max_width * vin->vi_max_height * 4;

	if(tuning->temper_buffer_size <= ispmem_size && ispmem_base != -1){
		tuning->temper_paddr = ispmem_base;
		/* config temper dma */
		apical_isp_temper_frame_buffer_bank0_base_write(tuning->temper_paddr);
		apical_isp_temper_frame_buffer_bank1_base_write(tuning->temper_paddr);
		/* apical_isp_temper_frame_buffer_bank1_base_write(tuning->temper_paddr + (tuning->temper_buffer_size)); */
		apical_isp_temper_frame_buffer_frame_write_cancel_write(0);
		apical_isp_temper_frame_buffer_frame_read_cancel_write(0);
		apical_isp_temper_frame_buffer_frame_write_on_write(1);
		apical_isp_temper_frame_buffer_frame_read_on_write(1);
	}else{
		tuning->temper_paddr = 0;
		tuning->temper_buffer_size = 0;
	}

	if(tuning->wdr_buffer_size <= (ispmem_size - tuning->temper_buffer_size) && ispmem_base != -1){
		tuning->wdr_paddr = ispmem_base + tuning->wdr_buffer_size;
		/*config WDR dma */
		apical_isp_frame_stitch_frame_buffer_bank0_base_write(tuning->wdr_paddr);
		apical_isp_frame_stitch_frame_buffer_bank1_base_write(tuning->wdr_paddr);
	//	apical_isp_frame_stitch_frame_buffer_bank1_base_write(tuning->wdr_paddr + (tuning->wdr_buffer_size >> 1));
	}else{
		tuning->wdr_paddr = 0;
		tuning->wdr_buffer_size = 0;
	}

	file->private_data = &tuning->fh;
	//	fh->prio = V4L2_PRIORITY_DEFAULT;
	//	v4l2_prio_open(&camdev->prio, &tuning->fh.prio);

	atomic_set(&tuning->state, TX_ISP_STATE_START);
	/* update isp config from bin file */
	v4l2_ctrl_handler_setup(&tuning->ctrls.handler);
	{
		TXispPrivParamManage *param = core->param;
		TXispPrivCustomerParamer *customer = NULL;
		if (NULL != param) {
			customer = &param->customer[TX_ISP_PRIV_PARAM_DAY_MODE];
			if (NULL != customer) {
				tmp = APICAL_READ_32(0x40);
				tmp = (tmp | 0x0c02da6c) & (~(customer->top));
				if(TX_ISP_EXIST_FR_CHANNEL == 0)
					tmp |= 0x00fc0000;
				APICAL_WRITE_32(0x40, tmp);
				if (customer->top & (1 << 19)){
#if TX_ISP_EXIST_FR_CHANNEL
					apical_isp_top_bypass_fr_gamma_rgb_write(0);
					apical_isp_fr_gamma_rgb_enable_write(1);
#endif
#if TX_ISP_EXIST_DS2_CHANNEL
					apical_isp_top_bypass_ds2_gamma_rgb_write(0);
					apical_isp_ds2_gamma_rgb_enable_write(1);
#endif
				} else {
#if TX_ISP_EXIST_FR_CHANNEL
					apical_isp_top_bypass_fr_gamma_rgb_write(1);
					apical_isp_fr_gamma_rgb_enable_write(0);
#endif
#if TX_ISP_EXIST_DS2_CHANNEL
					apical_isp_top_bypass_ds2_gamma_rgb_write(1);
					apical_isp_ds2_gamma_rgb_enable_write(0);
#endif
				}

				if ((customer->top) & (1 << 20)){
#if TX_ISP_EXIST_FR_CHANNEL
					apical_isp_top_bypass_fr_sharpen_write(0);
					apical_isp_fr_sharpen_enable_write(1);
#endif
#if TX_ISP_EXIST_DS2_CHANNEL
					apical_isp_top_bypass_ds2_sharpen_write(0);
					apical_isp_ds2_sharpen_enable_write(1);
#endif
				} else {
#if TX_ISP_EXIST_FR_CHANNEL
					apical_isp_fr_sharpen_enable_write(1);
					apical_isp_top_bypass_fr_sharpen_write(0);
#endif
#ifdef TX_ISP_EXIST_DS2_CHANNEL
					apical_isp_top_bypass_ds2_sharpen_write(1);
					apical_isp_ds2_sharpen_enable_write(0);
#endif
				}
				if ((customer->top) & (1 << 27))
					apical_isp_ds1_sharpen_enable_write(1);
				else
					apical_isp_ds1_sharpen_enable_write(0);
			}
		}
	}
	return ret;
}

static int image_tuning_v4l2_close(struct file *file)
{
	image_tuning_vdrv_t *tuning = video_drvdata(file);

	if(atomic_read(&tuning->state) != TX_ISP_STATE_START){
		return 0;
	}

	atomic_set(&tuning->state, TX_ISP_STATE_STOP);
	//	v4l2_prio_close(&camdev->prio, tuning->fh.prio);
	return 0;
}

static struct v4l2_file_operations image_tuning_v4l2_fops = {
	.owner 		= THIS_MODULE,
	.open 		= image_tuning_v4l2_open,
	.release 	= image_tuning_v4l2_close,
	.unlocked_ioctl	= video_ioctl2,
};

static struct video_device image_tuning_vdev = {
	.name = "isp image_tuning",
	.minor = -1,
	.release = video_device_release,
	.fops = &image_tuning_v4l2_fops,
	.ioctl_ops = &image_tuning_v4l2_ioctl_ops,
};

struct video_device *tx_isp_image_tuning_device_register(struct v4l2_subdev *parent)
{
	struct v4l2_device *v4l2_dev;
	image_tuning_vdrv_t *tuning = NULL;
	struct video_device *vfd = NULL;
	struct image_tuning_ctrls *ctrls;
	struct v4l2_ctrl_handler *handler;
	const struct v4l2_ctrl_ops *ops = &image_tuning_ctrl_ops;
	int ret = ISP_SUCCESS;

	if(!parent || !parent->v4l2_dev){
		return NULL;
	}

	v4l2_dev = parent->v4l2_dev;
	tuning = (image_tuning_vdrv_t *)kzalloc(sizeof(*tuning), GFP_KERNEL);
	if(!tuning){
		v4l2_err(v4l2_dev,"Failed to allocate isp image tuning device\n");
		return NULL;
	}
	memset(tuning, 0, sizeof(*tuning));

	vfd = video_device_alloc();
	if (!vfd) {
		v4l2_err(v4l2_dev,"Failed to allocate video device\n");
		goto failed_video_alloc;
	}
	tuning->parent = parent;
	spin_lock_init(&tuning->slock);
	mutex_init(&tuning->mlock);

	ctrls = &tuning->ctrls;
	handler = &ctrls->handler;
	v4l2_ctrl_handler_init(handler, 38);

	ctrls->wb_mode = v4l2_ctrl_new_std_menu(handler, ops,
			V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE,
			9, ~0x37f, V4L2_WHITE_BALANCE_AUTO);

	ctrls->wb_temperature = v4l2_ctrl_new_std(handler, ops, V4L2_CID_WHITE_BALANCE_TEMPERATURE,
			0, 25500, 100, 0);
	ctrls->wb_temperature->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrls->exp_mode = v4l2_ctrl_new_std_menu(handler, ops, V4L2_CID_EXPOSURE_AUTO, 3,
			~0x3, V4L2_EXPOSURE_AUTO);

	ctrls->manual_exp = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE_ABSOLUTE,
			0, 0x0fffffff, 1, 1);

	ctrls->exp_gain = v4l2_ctrl_new_custom(handler, &ae_gain_ctrl_int32, NULL);
	ctrls->exp_compensation = v4l2_ctrl_new_custom(handler, &ae_compensation_ctrl_int32, NULL);
	ctrls->af_mode = v4l2_ctrl_new_custom(handler, &af_mode_ctrl_menu, NULL);

	ctrls->af_status = v4l2_ctrl_new_std(handler, ops, V4L2_CID_AUTO_FOCUS_STATUS,
			0, 0x7, 0, 0);
	ctrls->af_status->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrls->hflip = v4l2_ctrl_new_std(handler, ops, V4L2_CID_HFLIP,
			0, 1, 1, 0);
	ctrls->vflip = v4l2_ctrl_new_std(handler, ops, V4L2_CID_VFLIP,
			0, 1, 1, 0);

	ctrls->dis = v4l2_ctrl_new_std(handler, ops, V4L2_CID_IMAGE_STABILIZATION,
			0, 1, 1, 0);

	ctrls->flicker = v4l2_ctrl_new_std_menu(handler, ops,
			V4L2_CID_POWER_LINE_FREQUENCY, 3,
			~0x7, V4L2_CID_POWER_LINE_FREQUENCY_DISABLED);

	/* Add support for NIGHT_PORTRAIT option */
	ctrls->scene = v4l2_ctrl_new_std_menu(handler, ops,
			V4L2_CID_SCENE_MODE, V4L2_SCENE_MODE_TEXT + 1,
			~0x7bfd, V4L2_SCENE_MODE_NONE);

	ctrls->colorfx = v4l2_ctrl_new_std_menu(handler, ops, V4L2_CID_COLORFX,
						V4L2_COLORFX_SET_CBCR, ~0x20f, V4L2_COLORFX_NONE);

	ctrls->saturation = v4l2_ctrl_new_std(handler, ops, V4L2_CID_SATURATION,
			0, 0x7fffffff, 1, 128);
	ctrls->brightness = v4l2_ctrl_new_std(handler, ops, V4L2_CID_BRIGHTNESS,
			0, 0x7fffffff, 1, 128);
	ctrls->contrast = v4l2_ctrl_new_std(handler, ops, V4L2_CID_CONTRAST,
			0, 0x7fffffff, 1, 128);

	ctrls->sharpness = v4l2_ctrl_new_std(handler, ops, V4L2_CID_SHARPNESS,
			0, 0x7fffffff, 1, 128);

	ctrls->aefwb_lock = v4l2_ctrl_new_std(handler, ops,
			V4L2_CID_3A_LOCK, 0, 0x7, 0, 0);

	/* init the ctrls of isp modules */
	ctrls->sinter = v4l2_ctrl_new_custom(handler, &sinter_ctrl_bool, NULL);
	ctrls->fog = v4l2_ctrl_new_custom(handler, &fog_ctrl_menu, NULL);
	ctrls->isp_process = v4l2_ctrl_new_custom(handler, &ispprocess_ctrl_bool, NULL);
	ctrls->freeze_fw = v4l2_ctrl_new_custom(handler, &ispfreeze_ctrl_bool, NULL);
	ctrls->black_level = v4l2_ctrl_new_custom(handler, &blacklevel_ctrl_bool, NULL);
	ctrls->lens_shad = v4l2_ctrl_new_custom(handler, &shading_ctrl_bool, NULL);
	ctrls->static_dp = v4l2_ctrl_new_custom(handler, &sdp_ctrl_bool, NULL);
	ctrls->dynamic_dp = v4l2_ctrl_new_custom(handler, &ddp_ctrl_bool, NULL);
	ctrls->green_eq = v4l2_ctrl_new_custom(handler, &ge_ctrl_bool, NULL);
	ctrls->raw_drc = v4l2_ctrl_new_custom(handler, &drc_ctrl_menu, NULL);
	ctrls->test_pattern = v4l2_ctrl_new_custom(handler, &test_pattern_ctrl_menu, NULL);

	ctrls->resolution = v4l2_ctrl_new_custom(handler, &resolution_ctrl_menu, NULL);
	ctrls->temper = v4l2_ctrl_new_custom(handler, &temper_ctrl_menu, NULL);
	ctrls->wdr_lut = v4l2_ctrl_new_custom(handler, &wdr_lut_ctrl_bool, NULL);
	ctrls->wdr = v4l2_ctrl_new_custom(handler, &wdr_ctrl_bool, NULL);


//	ctrls->fps = v4l2_ctrl_new_custom(handler, &fps_ctrl_int32, NULL);
	if (handler->error) {
		goto handler_error;
	}

	v4l2_ctrl_auto_cluster(2, &ctrls->wb_mode,
			V4L2_WHITE_BALANCE_MANUAL, false);

	v4l2_ctrl_auto_cluster(4, &ctrls->exp_mode,
			V4L2_EXPOSURE_MANUAL, true);

	v4l2_ctrl_cluster(2, &ctrls->af_mode);

	v4l2_ctrl_cluster(2, &ctrls->dynamic_dp);

	memcpy(vfd, &image_tuning_vdev, sizeof(image_tuning_vdev));
	vfd->lock = &tuning->mlock;
	vfd->v4l2_dev = v4l2_dev;
	/* vfd->debug = V4L2_DEBUG_IOCTL | V4L2_DEBUG_IOCTL_ARG; */
	vfd->ctrl_handler = handler;

	set_bit(V4L2_FL_USE_FH_PRIO, &(vfd->flags)); // add lately

	tuning->video = vfd;

	ret = video_register_device(vfd, VFL_TYPE_GRABBER, -1);
	if (ret < 0) {
		v4l2_err(v4l2_dev,"Failed to register video device\n");
		goto failed_register;
	}

	video_set_drvdata(vfd, tuning);

	atomic_set(&tuning->state, TX_ISP_STATE_STOP);
	return vfd;

failed_register:
	v4l2_ctrl_handler_free(handler);
handler_error:
	video_device_release(vfd);
failed_video_alloc:
	kfree(tuning);
	return NULL;
}
void tx_isp_image_tuning_device_release(struct video_device *vfd)
{
	v4l2_ctrl_handler_free(vfd->ctrl_handler);
	video_unregister_device(vfd);
}
