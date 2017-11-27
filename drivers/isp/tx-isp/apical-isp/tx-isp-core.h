#ifndef __TX_ISP_CORE_H__
#define __TX_ISP_CORE_H__

#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <jz_proc.h>

#include <apical-isp/apical.h>
#include <tx-isp-common.h>
#include "../tx-isp-frame-channel.h"
#include "../tx-isp-interrupt.h"
#include "../tx-isp-videobuf.h"
#include "tx-isp-load-parameters.h"


/* apical ISP channel define */
enum isp_video_channel_define{
	ISP_FR_VIDEO_CHANNEL,
	ISP_DS1_VIDEO_CHANNEL,
#if TX_ISP_EXIST_DS2_CHANNEL
	ISP_DS2_VIDEO_CHANNEL,
#endif
	ISP_MAX_OUTPUT_VIDEOS,
};

struct isp_reg_t {
	unsigned int reg;
	unsigned int value;
};
#define ISP_DMA_WRITE_MAXBASE_NUM 5
#define ISP_DMA_WRITE_BANK_FLAG_UNCONFIG 0
#define ISP_DMA_WRITE_BANK_FLAG_CONFIG 1
enum apical_isp_format_check_index {
	APICAL_ISP_INPUT_RAW_FMT_INDEX_START = 0,
	APICAL_ISP_INPUT_RGB888_FMT_INDEX_START = 0,
	APICAL_ISP_INPUT_YUV_FMT_INDEX_START = 0,
	APICAL_ISP_NV12_FMT_INDEX = 0,
	APICAL_ISP_NV21_FMT_INDEX,
	APICAL_ISP_YUYV_FMT_INDEX,
	APICAL_ISP_UYVY_FMT_INDEX,
	APICAL_ISP_INPUT_YUV_FMT_INDEX_END = APICAL_ISP_UYVY_FMT_INDEX,
	APICAL_ISP_YUV444_FMT_INDEX,
	APICAL_ISP_INPUT_RGB565_FMT_INDEX_START = APICAL_ISP_YUV444_FMT_INDEX,
	APICAL_ISP_RGB565_FMT_INDEX,
	APICAL_ISP_INPUT_RGB565_FMT_INDEX_END = APICAL_ISP_RGB565_FMT_INDEX,
	APICAL_ISP_RGB24_FMT_INDEX,
	APICAL_ISP_RGB888_FMT_INDEX,
	APICAL_ISP_INPUT_RGB888_FMT_INDEX_END = APICAL_ISP_RGB888_FMT_INDEX,
	APICAL_ISP_RGB310_FMT_INDEX,
	APICAL_ISP_RAW_FMT_INDEX,
	APICAL_ISP_INPUT_RAW_FMT_INDEX_END = APICAL_ISP_RAW_FMT_INDEX,
	APICAL_ISP_FMT_MAX_INDEX,
};

struct tx_isp_frame_channel {
	frame_chan_vdev_t video;
	struct list_head fifo;
	unsigned char bank_flag[ISP_DMA_WRITE_MAXBASE_NUM];
	unsigned char vflip_flag[ISP_DMA_WRITE_MAXBASE_NUM];
	unsigned int banks_addr[ISP_DMA_WRITE_MAXBASE_NUM];
	unsigned char start;
	unsigned char end;
	unsigned char dma_state;
	unsigned char reset_dma_flag;
	unsigned char vflip_state;
	unsigned char state;
	unsigned char usingbanks;
	spinlock_t slock;
};

#define APICAL_ISP_TOP_CONTROL_LOW_REG_DEFAULT (0x3f7fffff)
//#define APICAL_ISP_TOP_CONTROL_LOW_REG_DEFAULT (0x33658593)
//#define APICAL_ISP_TOP_CONTROL_LOW_REG_TEST (0x3f7ffffe)
#define APICAL_ISP_TOP_CONTROL_HIGH_REG_DEFAULT (0x00000c7e)

struct apical_isp_contrl {
	unsigned int infmt;
	unsigned short inwidth;
	unsigned short inheight;
	unsigned int pattern;
	enum apical_isp_format_check_index fmt_start;
	enum apical_isp_format_check_index fmt_end;
};

struct tx_isp_dma_conf {
	void *vaddr;
	dma_addr_t paddr;
	unsigned long size;
};

enum tx_isp_i2c_index {
	TX_ISP_I2C_SET_AGAIN,
	TX_ISP_I2C_SET_DGAIN,
	TX_ISP_I2C_SET_INTEGRATION,
	TX_ISP_I2C_SET_BUTTON,
};

struct tx_isp_i2c_msg {
	unsigned int flag;
	unsigned int value;
};

struct tx_isp_core_device {
	/* the common parameters */
	struct device *dev;
	struct v4l2_subdev sd;
	struct media_pad pads[TX_ISP_PADS_NUM];
	struct resource *res;
	void __iomem *base;
	struct clk **clks;
	unsigned int clk_num;
	spinlock_t slock;
	atomic_t state;
	unsigned int refcnt;
	const unsigned int revision;
	struct tx_isp_video_in vin;
	struct v4l2_prio_state prio;
	void * pdata;

	/* the special parameters of a instance */
	apical_isp_top_ctl_t top;
	struct apical_isp_contrl contrl;

	struct video_device *tun;
	struct mutex mlock;

	struct tx_isp_frame_channel *chans;
	atomic_t chan_state;
	int bypass;
	void *vbm;

	/* frame state */
	volatile unsigned int frame_state; // 0 : idle, 1 : processing
	unsigned int vflip_state; //0:disable, 1: enable
	unsigned int hflip_state; //0:disable, 1: enable
	unsigned int isp_daynight_switch;
	/* i2c sync messages */
	struct tx_isp_i2c_msg i2c_msgs[TX_ISP_I2C_SET_BUTTON];
	/* the private parameters */
	struct task_struct *process_thread;
	TXispPrivParamManage *param;
	/* the node of printing isp info */
	struct proc_dir_entry *proc;
};

#define sd_to_tx_isp_core_device(x) (container_of((x), struct tx_isp_core_device, sd))
int register_tx_isp_core_device(struct platform_device *pdev, struct v4l2_device *v4l2_dev);
void release_tx_isp_core_device(struct v4l2_subdev *sd);
#endif /* __TX_ISP_CORE_H__  */
