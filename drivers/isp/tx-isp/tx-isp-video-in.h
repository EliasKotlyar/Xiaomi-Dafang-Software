#ifndef __TX_ISP_SENSOR_H__
#define __TX_ISP_SENSOR_H__
#include <tx-isp-common.h>

struct tx_isp_video_in_device {
	struct v4l2_subdev sd;
	struct list_head sensors;
	struct tx_isp_sensor *active; /* the sensor instance */
	struct media_pad pads[TX_ISP_PADS_NUM];

	struct tx_isp_media_pipeline *p;
	spinlock_t slock;
	atomic_t state;
	unsigned int refcnt;
	struct tx_isp_subdev_platform_data *pdata;
};

#define sd_to_tx_video_in_device(sd) (container_of(sd, struct tx_isp_video_in_device, sd))
int register_tx_isp_video_in_device(void *pdata, struct v4l2_device *v4l2_dev);
void release_tx_isp_video_in_device(struct v4l2_subdev *sd);
#endif /* __TX_ISP_SENSOR_H__ */
