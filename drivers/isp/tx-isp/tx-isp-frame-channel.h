#ifndef __TX_ISP_FRAME_CHANNEL_H__
#define __TX_ISP_FRAME_CHANNEL_H__

#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-core.h>

#include <tx-isp-common.h>

struct frame_channel_format {
	unsigned char name[32];
	unsigned int fourcc;
	unsigned int depth;
	unsigned int priv;
};

struct tx_isp_frame_channel_video_device;

typedef struct tx_isp_frame_channel_video_device frame_chan_vdev_t;


struct frame_channel_attribute {
/* Private: internal use only */
	unsigned short max_width;
	unsigned short max_height;
	unsigned short min_width;
	unsigned short min_height;
	unsigned short step_width;
	unsigned short step_height;
	struct v4l2_rect        bounds;
	struct frame_image_scalercap scalercap;

/* Public: user could set it through special ioctl */
	struct v4l2_rect        crop;
	struct frame_image_scaler scaler;
	struct v4l2_format	output;
	unsigned int  crop_enable :1;
	unsigned int  scaler_enable :1;
	unsigned int lineoffset;
	int frame_rate;
};

struct frame_channel_buffer {
	struct list_head entry;
	unsigned int addr;
	void *priv;
};

struct frame_channel_video_buffer{
	struct vb2_buffer vb;
	struct frame_channel_buffer buf;
	struct list_head entry;
};

struct frame_channel_fh {
	enum v4l2_priority prio;
};
struct tx_isp_frame_channel_video_device {
	/* the paramters have been inited before it is registered */
	struct v4l2_subdev *parent;
	struct frame_vb2_manager *vbm;
	struct frame_channel_attribute attr;
	int index;
	int bypass;

	/* the parameters will be inited when it is registered */
	struct video_device *video;
	struct vb2_queue vbq;
	struct list_head active_list;
	spinlock_t slock;
	struct mutex mlock;
	atomic_t state;
	struct completion comp;
	int (*interrupt_service_routine)(frame_chan_vdev_t  *chan,
						u32 status, bool *handled);
	unsigned int out_frames;
	unsigned int lose_frames;
	struct v4l2_fmtdesc *fmtdesc;
	struct v4l2_format *fmt;
	int reqbufs;
};

int tx_isp_frame_channel_device_register(frame_chan_vdev_t *chan);
void tx_isp_frame_channel_device_unregister(frame_chan_vdev_t *chan);

#endif/*__TX_ISP_FRAME_CHANNEL_H__*/
