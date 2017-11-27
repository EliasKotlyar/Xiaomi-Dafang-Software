#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>

#include "tx-isp-video-in.h"
/*
 * Sensor subdevice helper functions
 */
static long subdev_core_ops_register_sensor(struct tx_isp_video_in_device *vi, void *arg)
{
	struct v4l2_tx_isp_sensor_register_info *info = arg;
	struct v4l2_device *v4l2_dev = NULL;
	struct v4l2_subdev *sd = NULL;
	struct tx_isp_sensor *sensor = NULL;
	int ret = ISP_SUCCESS;

	if (!vi || !info)
		return -ISP_ERROR;
	v4l2_dev = vi->sd.v4l2_dev;
	if(atomic_read(&vi->state) == TX_ISP_STATE_STOP){
		v4l2_warn(v4l2_dev, "the devnode does't have been opened.\n");
		return -ISP_ERROR;
	}
#if 0
	if(atomic_read(&vi->state) == TX_ISP_STATE_RUN){
		v4l2_warn(v4l2_dev, "the devnode is running,please stop it firstly.\n");
		return -ISP_ERROR;
	}
#endif
#if 0
	/* 放在sensor驱动中 */
	sensor = (struct tx_isp_sensor_instance *)kzalloc(sizeof(*sensor), GFP_KERNEL);
	if(!sensor){
		v4l2_err(v4l2_dev, "Failed to allocate sensor subdev.\n",);
		return -ENOMEM;
	}
#endif
	if(info->cbus_type == TX_SENSOR_CONTROL_INTERFACE_I2C){
		struct i2c_adapter *adapter;
		struct i2c_board_info board_info;
		adapter = i2c_get_adapter(info->i2c.i2c_adapter_id);
		if (!adapter) {
			v4l2_warn(v4l2_dev,
					"Failed to get I2C adapter %d, deferring probe\n",
					info->i2c.i2c_adapter_id);
			return -ISP_ERROR;
		}
		memset(&board_info, 0 , sizeof(board_info));
		memcpy(&board_info.type, &info->i2c.type, I2C_NAME_SIZE);
		board_info.addr = info->i2c.addr;
		sd = v4l2_i2c_new_subdev_board(v4l2_dev, adapter,
				&board_info, NULL);
		if (IS_ERR_OR_NULL(sd)) {
			i2c_put_adapter(adapter);
			v4l2_warn(v4l2_dev,
					"Failed to acquire subdev %s, deferring probe\n",
					info->i2c.type);
			return -ISP_ERROR;
		}
	}else if (info->cbus_type == TX_SENSOR_CONTROL_INTERFACE_SPI){
	}else{
		v4l2_warn(v4l2_dev, "%s[%d] the type of sensor SBUS hasn't been defined.\n",__func__,__LINE__);
		return -ISP_ERROR;
	}
	/* add private data of sensor */
	sensor = (struct tx_isp_sensor *)v4l2_get_subdev_hostdata(sd);
	sensor->info = *info;

	ret = v4l2_subdev_call(sd, core, g_chip_ident, NULL);
	if(ret != ISP_SUCCESS){
		if(info->cbus_type == TX_SENSOR_CONTROL_INTERFACE_I2C){
			struct i2c_client *client = v4l2_get_subdevdata(sd);
			struct i2c_adapter *adapter = client->adapter;
			if (adapter)
				i2c_put_adapter(adapter);
		//	i2c_unregister_device(client);
		}else{
		}
		v4l2_device_unregister_subdev(sd);
		return -ISP_ERROR;
	}

	spin_lock(&vi->slock);
	list_add_tail(&sensor->list, &vi->sensors);
	spin_unlock(&vi->slock);

	v4l2_info(v4l2_dev, "Registered sensor subdevice %s\n", sd->name);
	return ISP_SUCCESS;
}
static long subdev_core_ops_release_sensor(struct tx_isp_video_in_device *vi, void *arg)
{
	struct v4l2_tx_isp_sensor_register_info *info = arg;
	struct v4l2_device *v4l2_dev = NULL;
	struct v4l2_subdev *sd = NULL;
	struct tx_isp_sensor *sensor = NULL;

	if (!vi || !info)
		return -ISP_ERROR;
	v4l2_dev = vi->sd.v4l2_dev;
	if(atomic_read(&vi->state) == TX_ISP_STATE_STOP){
		v4l2_warn(v4l2_dev, "the devnode does't have been opened.\n");
		return -ISP_ERROR;
	}

	spin_lock(&vi->slock);
	list_for_each_entry(sensor, &vi->sensors, list) {
		if(!strcmp(sensor->info.name, info->name)){
			sd = &sensor->sd;
			break;
		}
	}
	/* when can't find the matching sensor, do nothing!*/
	if(!sd){
		spin_unlock(&vi->slock);
		return ISP_SUCCESS;
	}
	/* when the sensor is active, please stop it firstly.*/
	if(sensor == vi->active){
		spin_unlock(&vi->slock);
		v4l2_warn(v4l2_dev, "the sensor is active, please stop it firstly.\n");
		return -ISP_ERROR;
	}

	list_del(&sensor->list);
	spin_unlock(&vi->slock);

	if(sensor->info.cbus_type == TX_SENSOR_CONTROL_INTERFACE_I2C){
		struct i2c_client *client = v4l2_get_subdevdata(sd);
		struct i2c_adapter *adapter = client->adapter;
		if (adapter)
			i2c_put_adapter(adapter);
		i2c_unregister_device(client);

	}else if (sensor->info.cbus_type == TX_SENSOR_CONTROL_INTERFACE_SPI){
	}else{
		v4l2_warn(v4l2_dev, "%s[%d] the type of sensor SBUS hasn't been defined.\n",__func__,__LINE__);
		return -ISP_ERROR;
	}
//	v4l2_device_unregister_subdev(sd);
	return ISP_SUCCESS;
}

static long subdev_core_ops_release_all_sensor(struct tx_isp_video_in_device *vi)
{
	struct v4l2_device *v4l2_dev = NULL;
	struct v4l2_subdev *sd = NULL;
	struct tx_isp_sensor *sensor = NULL;

	if (!vi)
		return -ISP_ERROR;
	v4l2_dev = vi->sd.v4l2_dev;
	if(atomic_read(&vi->state) == TX_ISP_STATE_STOP){
		v4l2_warn(v4l2_dev, "the devnode does't have been opened.\n");
		return -ISP_ERROR;
	}

	while(!list_empty(&vi->sensors)){
		sensor = list_first_entry(&vi->sensors, struct tx_isp_sensor, list);
		list_del(&sensor->list);
		sd = &sensor->sd;
		if(sensor->info.cbus_type == TX_SENSOR_CONTROL_INTERFACE_I2C){
			struct i2c_client *client = v4l2_get_subdevdata(sd);
			struct i2c_adapter *adapter = client->adapter;
			if (adapter)
				i2c_put_adapter(adapter);
			i2c_unregister_device(client);

		}else if (sensor->info.cbus_type == TX_SENSOR_CONTROL_INTERFACE_SPI){
		}else{
			v4l2_warn(v4l2_dev, "%s[%d] the type of sensor SBUS hasn't been defined.\n",__func__,__LINE__);
			return -ISP_ERROR;
		}
	}
	return ISP_SUCCESS;
}



/*
* 每次枚举都需要对vi->sensors链表中的sensor的index进行编号。
*/
static long subdev_core_ops_enum_input(struct tx_isp_video_in_device *vi, void *arg)
{
	struct v4l2_input *input = (struct v4l2_input *)arg;
	struct v4l2_device *v4l2_dev = NULL;
	struct tx_isp_sensor *sensor = NULL;
	int i = 0;

	if (!vi || !input){
		return -ISP_ERROR;
	}
	v4l2_dev = vi->sd.v4l2_dev;

	spin_lock(&vi->slock);
	list_for_each_entry(sensor, &vi->sensors, list) {
		sensor->index = i++;
		if(sensor->index == input->index){
			input->type = sensor->type;
			strncpy(input->name, sensor->info.name, sizeof(sensor->info.name));
			break;
		}
	}
	spin_unlock(&vi->slock);
	if(sensor->index != input->index)
		return -ISP_ERROR;

	return 0;
}
/*
* 返回vi->active sensor 中的index编号
*/
static long subdev_core_ops_get_input(struct tx_isp_video_in_device *vi, int *index)
{
	if (!vi || !index)
		return -ISP_ERROR;
	*index = vi->active->index;
	return ISP_SUCCESS;
}
/*
*
*/
static long subdev_core_ops_set_input(struct tx_isp_video_in_device *vi, int *index)
{
	struct v4l2_device *v4l2_dev = NULL;
	struct tx_isp_sensor *sensor = NULL;
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = NULL;
	int ret = ISP_SUCCESS;
	if (!vi || !index)
		return -ISP_ERROR;

	/*firstly, Determine whether the point to the same sensor */
	if(vi->active && *index == vi->active->index)
		return ISP_SUCCESS;

	v4l2_dev = vi->sd.v4l2_dev;
	sensor = vi->active;

	/* secondly, streamoff, deinit and unprepare previous sensor. */
	if(sensor){
		if(atomic_read(&vi->state) == TX_ISP_STATE_RUN){
			v4l2_warn(v4l2_dev, "Please, streamoff sensor firstly!\n");
			return -ISP_ERROR;
		}
		ret = tx_isp_pipeline_call(vi->p, init, 0);
		if(ret != ISP_SUCCESS){
			v4l2_warn(v4l2_dev, "Failed to deinit the pipeline of %s.\n", sensor->attr.name);
			goto err_exit;
		}
		ret = tx_isp_pipeline_call(vi->p, unprepare);
		if(ret != ISP_SUCCESS){
			v4l2_warn(v4l2_dev, "Failed to unprepare the pipeline of %s.\n", sensor->attr.name);
			goto err_exit;
		}
		vi->p->subdevs[vi->sd.grp_id] = NULL;
		vi->active = NULL;

		arg.value = 0;
		sd = &sensor->sd;
		sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
		if(arg.ret != ISP_SUCCESS)
			goto err_exit;
	}

	/* third, s_input NULL .*/
	if(-1 == *index){
		return ISP_SUCCESS;
	}
	/* fourth, find a corresponding sensor.*/
	spin_lock(&vi->slock);
	list_for_each_entry(sensor, &vi->sensors, list) {
		if(sensor->index == *index){
			break;
		}
	}
	spin_unlock(&vi->slock);

	/* if the index isn't find in the list of sensors that has been registered.*/
	if(sensor->index != *index){
		v4l2_err(v4l2_dev, "Failed to the set input sensor(%d) that .\n", *index);
		return -ISP_ERROR;
	}

	/*lastly, prepare, init and streamon active sensor */
	vi->active = sensor;
	vi->p->subdevs[vi->sd.grp_id] = &sensor->sd;

	sd = &sensor->sd;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	if(arg.ret != ISP_SUCCESS)
		goto err_exit;

	ret = tx_isp_pipeline_call(vi->p, prepare); //&sensor->video);
	if(ret != ISP_SUCCESS){
		v4l2_warn(v4l2_dev, "Failed to unprepare the pipeline of %s.\n", sensor->attr.name);
		goto err_prepare;
	}
	ret = tx_isp_pipeline_call(vi->p, init, 1);
	if(ret != ISP_SUCCESS){
		v4l2_warn(v4l2_dev, "Failed to deinit the pipeline of %s.\n", sensor->attr.name);
		goto err_init;
	}

	return ISP_SUCCESS;
err_prepare:
	tx_isp_pipeline_call(vi->p, unprepare);
err_init:
err_exit:
	return ret;
}

static long subdev_core_ops_streamon(struct tx_isp_video_in_device *vi, int enable)
{
	struct v4l2_device *v4l2_dev = NULL;
	struct tx_isp_sensor *sensor = NULL;
	int ret = ISP_SUCCESS;
	if (!vi)
		return -ISP_ERROR;

	if(atomic_read(&vi->state) == TX_ISP_STATE_RUN){
		return ISP_SUCCESS;
	}

	v4l2_dev = vi->sd.v4l2_dev;
	sensor = vi->active;

	if(sensor){
		ret = tx_isp_pipeline_call(vi->p, set_stream, enable);
		if(ret == ISP_SUCCESS){
			atomic_set(&vi->state, TX_ISP_STATE_RUN);
		}else{
			v4l2_err(v4l2_dev, "Failed to streamon the pipeline of %s.\n",
							sensor->attr.name);
		}
	}else{
		v4l2_warn(v4l2_dev, "There isn't sensor that has been selected!\n");
	}
	return ret;
}

static long subdev_core_ops_streamoff(struct tx_isp_video_in_device *vi, int enable)
{
	struct v4l2_device *v4l2_dev = NULL;
	struct tx_isp_sensor *sensor = NULL;
	int ret = ISP_SUCCESS;
	if (!vi)
		return -ISP_ERROR;

	if(atomic_read(&vi->state) != TX_ISP_STATE_RUN){
		return ISP_SUCCESS;
	}

	v4l2_dev = vi->sd.v4l2_dev;
	sensor = vi->active;

	if(sensor){
		ret = tx_isp_pipeline_call(vi->p, set_stream, enable);
		if(ret == ISP_SUCCESS){
			atomic_set(&vi->state, TX_ISP_STATE_START);
		}else{
			v4l2_err(v4l2_dev, "Failed to streamoff the pipeline of %s.\n",
							sensor->attr.name);
		}
	}else{
		v4l2_warn(v4l2_dev, "There isn't sensor that has been selected!\n");
	}
	return ret;
}

static long video_in_core_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_video_in_device *vi = sd_to_tx_video_in_device(sd);
	long ret = 0;
	switch(cmd){
		case VIDIOC_ENUMINPUT:
			ret = subdev_core_ops_enum_input(vi, arg);
			break;
		case VIDIOC_G_INPUT:
			ret = subdev_core_ops_get_input(vi, arg);
			break;
		case VIDIOC_S_INPUT:
			ret = subdev_core_ops_set_input(vi, arg);
			break;
		case VIDIOC_REGISTER_SENSOR:
			ret = subdev_core_ops_register_sensor(vi, arg);
			break;
		case VIDIOC_RELEASE_SENSOR:
			ret = subdev_core_ops_release_sensor(vi, arg);
			break;
		case VIDIOC_STREAMON:
			ret = subdev_core_ops_streamon(vi, 1);
			break;
		case VIDIOC_STREAMOFF:
			ret = subdev_core_ops_streamoff(vi, 0);
			break;
		default:
			break;
	}
	return ret;
}

int tx_isp_video_in_subdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct tx_isp_video_in_device *vi = sd_to_tx_video_in_device(sd);
	struct tx_isp_notify_argument arg;
	if(atomic_read(&vi->state) == TX_ISP_STATE_STOP){
		atomic_set(&vi->state, TX_ISP_STATE_START);
		sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_GET_PIPELINE, &arg);
		if(arg.ret != ISP_SUCCESS)
			return -ISP_ERROR;
		vi->p = (struct tx_isp_media_pipeline *)arg.value;
	}
	vi->refcnt++;
	return 0;
}
int tx_isp_video_in_subdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct tx_isp_video_in_device *vi = sd_to_tx_video_in_device(sd);
	struct v4l2_device *v4l2_dev = NULL;
	int ret = ISP_SUCCESS;
	int index;
	vi->refcnt--;

	v4l2_dev = vi->sd.v4l2_dev;
	if(!vi->refcnt){
		if (atomic_read(&vi->state) == TX_ISP_STATE_RUN){
			ret = subdev_core_ops_streamoff(vi, 0);
			if(ret < 0){
				v4l2_err(v4l2_dev, "%s[%d] the device is running; Failed to stop it!\n",
					__func__,__LINE__);
			}else{
				index = -1;
				subdev_core_ops_set_input(vi, &index);
				subdev_core_ops_release_all_sensor(vi);
			}
		}
		if(atomic_read(&vi->state) == TX_ISP_STATE_START)
			atomic_set(&vi->state, TX_ISP_STATE_STOP);
	}
	return ret;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int tx_isp_video_in_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct tx_isp_video_in_device *vi = sd_to_tx_video_in_device(sd);
	struct tx_isp_sensor *sensor = vi->active;
	if(sensor){
		v4l2_subdev_call(&sensor->sd, core, g_register, reg);
	}
	return 0;
}
static int tx_isp_video_in_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct tx_isp_video_in_device *vi = sd_to_tx_video_in_device(sd);
	struct tx_isp_sensor *sensor = vi->active;
	if(sensor){
		v4l2_subdev_call(&sensor->sd, core, s_register, reg);
	}
	return 0;
}
#endif

static const struct v4l2_subdev_internal_ops video_in_subdev_internal_ops ={
	.open = tx_isp_video_in_subdev_open,
	.close = tx_isp_video_in_subdev_close,
};
static struct v4l2_subdev_core_ops video_in_subdev_core_ops ={
	.ioctl = &video_in_core_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = tx_isp_video_in_g_register,
	.s_register = tx_isp_video_in_s_register,
#endif
};
static struct v4l2_subdev_ops video_in_subdev_ops ={
	.core = &video_in_subdev_core_ops,
};
/* media operations */
static int video_in_link_setup(struct media_entity *entity,
			  const struct media_pad *local,
			  const struct media_pad *remote, u32 flags)
{
	return ISP_SUCCESS;
}
static const struct media_entity_operations video_in_media_ops = {
	.link_setup = video_in_link_setup,
};
int register_tx_isp_video_in_device(void *pdata, struct v4l2_device *v4l2_dev)
{
	struct tx_isp_video_in_device *vi = NULL;
	struct v4l2_subdev *sd = NULL;
	struct media_pad *pads = NULL;
	struct media_entity *me = NULL;
	int ret;

	vi = (struct tx_isp_video_in_device *)kzalloc(sizeof(*vi), GFP_KERNEL);
	if(!vi){
		v4l2_err(v4l2_dev, "Failed to allocate sensor subdev\n");
		ret = -ENOMEM;
		goto exit;
	}

	spin_lock_init(&vi->slock);
	INIT_LIST_HEAD(&vi->sensors);
	vi->refcnt = 0;
	vi->pdata = pdata;
	sd = &vi->sd;
	pads = vi->pads;
	me = &sd->entity;

	v4l2_subdev_init(sd, &video_in_subdev_ops);
	sd->internal_ops = &video_in_subdev_internal_ops;
	strlcpy(sd->name, "tx-isp-video-in", sizeof(sd->name));

	sd->grp_id = vi->pdata->grp_id;	/* group ID for isp subdevs */
	v4l2_set_subdevdata(sd, vi);

	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	pads[TX_ISP_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	pads[TX_ISP_PAD_LINK].flags = MEDIA_PAD_FL_SINK;

	me->ops = &video_in_media_ops;
	ret = media_entity_init(me, TX_ISP_PADS_NUM, pads, 0);
	if (ret < 0){
		v4l2_err(v4l2_dev, "Failed to allocate sensor device\n");
		ret = -ISP_ERROR;
		goto entity_init_failed;
	}
	ret = v4l2_device_register_subdev(v4l2_dev, sd);
	if (ret < 0){
		v4l2_err(v4l2_dev, "Failed to register sensor-subdev!\n");
		ret = -ISP_ERROR;
		goto register_failed;
	}
	/* set state of the subdev */
	atomic_set(&vi->state, TX_ISP_STATE_STOP);
	return ISP_SUCCESS;
register_failed:
	media_entity_cleanup(me);
entity_init_failed:
	kfree(vi);
exit:
	return ret;
}

void release_tx_isp_video_in_device(struct v4l2_subdev *sd)
{
	struct tx_isp_video_in_device *vi = v4l2_get_subdevdata(sd);

	v4l2_device_unregister_subdev(&vi->sd);
}
