#ifndef __TX_ISP_DEVICE_H__
#define __TX_ISP_DEVICE_H__

#include <linux/errno.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <tx-isp-common.h>

typedef struct __tx_isp_device {
	struct v4l2_device v4l2_dev;
	struct media_device media_dev;
	struct device *dev;
	unsigned int revision;
	struct v4l2_prio_state prio;

//	struct v4l2_subdev *subdevs[TX_ISP_MAX_IDX];
	struct tx_isp_irq_device *irq_dev;
//	struct mutex mlock;
//	atomic_t state;
	spinlock_t slock;
	struct tx_isp_media_pipeline pipeline;
} tx_isp_device_t;

#endif/*__TX_ISP_DEVICE_H__*/
