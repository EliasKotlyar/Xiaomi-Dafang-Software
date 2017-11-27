#ifndef __TX_ISP_COMMON_H__
#define __TX_ISP_COMMON_H__
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/videodev2.h>
#include <linux/v4l2-mediabus.h>
#include <media/media-entity.h>
#include <media/media-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-common.h>
#include <media/v4l2-device.h>
#include <linux/clk.h>
#include <mach/tx_isp.h>


#if defined(CONFIG_SOC_T10)
/* T10 */
#define TX_ISP_EXIST_FR_CHANNEL 1

#elif defined(CONFIG_SOC_T20)
/* T20 */
#define TX_ISP_EXIST_CSI_DEVICE 1
#define TX_ISP_EXIST_FR_CHANNEL 0
#define TX_ISP_EXIST_DS2_CHANNEL 1

#define TX_ISP_INPUT_PORT_MAX_WIDTH		2048
#define TX_ISP_INPUT_PORT_MAX_HEIGHT	1536
#if TX_ISP_EXIST_FR_CHANNEL
#define TX_ISP_FR_CAHNNEL_MAX_WIDTH		2048
#define TX_ISP_FR_CAHNNEL_MAX_HEIGHT	1536
#endif
#define TX_ISP_DS1_CAHNNEL_MAX_WIDTH	2048
#define TX_ISP_DS1_CAHNNEL_MAX_HEIGHT	1536
#if TX_ISP_EXIST_DS2_CHANNEL
#define TX_ISP_DS2_CAHNNEL_MAX_WIDTH	800
#define TX_ISP_DS2_CAHNNEL_MAX_HEIGHT	600
#endif
#else

#endif

/***************************************************
*  Provide extensions to v4l2 for ISP driver.
****************************************************/
#define V4L2_PIX_FMT_RGB310   v4l2_fourcc('R', 'G', 'B', 'A') /* 32  RGB-10-10-10  */
#define V4L2_MBUS_FMT_RGB888_3X8_LE (V4L2_MBUS_FMT_Y8_1X8 - 0x10)

/*
*------ definition sensor associated structure -----
*/

/* define control bus */
enum tx_sensor_control_bus_type{
	TX_SENSOR_CONTROL_INTERFACE_I2C = 1,
	TX_SENSOR_CONTROL_INTERFACE_SPI,
};
struct tx_isp_i2c_board_info {
	char type[I2C_NAME_SIZE];
	int addr;
//	struct i2c_board_info board_info;
	int i2c_adapter_id;
};

struct tx_isp_spi_board_info {
	char modalias[SPI_NAME_SIZE];
	int bus_num;
//	struct spi_board_info board_info;
};

/* define data bus */
enum tx_sensor_data_bus_type{
	TX_SENSOR_DATA_INTERFACE_MIPI = 1,
	TX_SENSOR_DATA_INTERFACE_DVP,
	TX_SENSOR_DATA_INTERFACE_BT601,
	TX_SENSOR_DATA_INTERFACE_BT656,
	TX_SENSOR_DATA_INTERFACE_BT1120,
};

typedef enum {
	DVP_PA_LOW_10BIT,
	DVP_PA_HIGH_10BIT,
	DVP_PA_12BIT,
	DVP_PA_LOW_8BIT,
	DVP_PA_HIGH_8BIT,
} sensor_dvp_gpio_mode;

typedef enum {
	SENSOR_DVP_HREF_MODE,
	SENSOR_DVP_HSYNC_MODE,
	SENSOR_DVP_SONY_MODE,
} sensor_dvp_timing_mode;

typedef enum {
	ISP_CLK_960P_MODE = 60000000,
	ISP_CLK_1080P_MODE = 80000000,
	ISP_CLK_3M_MODE = 100000000,
} isp_clk_mode;

typedef struct {
	unsigned short vblanking;
	unsigned short hblanking;
} sensor_dvp_blanking;

struct tx_isp_mipi_bus{
	unsigned int clk;
	unsigned char lans;
};

struct tx_isp_dvp_bus{
	sensor_dvp_gpio_mode gpio;
	sensor_dvp_timing_mode mode;
	sensor_dvp_blanking blanking;
};

struct tx_isp_bt1120_bus{
};
struct tx_isp_bt656_bus{
};
struct tx_isp_bt601_bus{
};

/* define sensor attribute */

#define TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS 	1
#define TX_ISP_SENSOR_FULL_RES_MAX_FPS 		2

struct v4l2_tx_isp_sensor_register_info{
	char name[32];
	enum tx_sensor_control_bus_type cbus_type;
	union {
		struct tx_isp_i2c_board_info i2c;
		struct tx_isp_spi_board_info spi;
	};
	unsigned short rst_gpio;
	unsigned short pwdn_gpio;
	unsigned short power_gpio;
};

typedef struct tx_isp_sensor_ctrl{
	/* isp_gain mean that the value is output of ISP-FW,it is not a gain multiplier unit.
	*  gain_mutiplier = (2^(isp_gain/(2^LOG_GAIN_SHIFT))).
	*  the fuction will convert gain_mutiplier to sensor_Xgain.
	*  the return value is isp_gain of sensor_Xgain's inverse conversion.
	*/
	unsigned int (*alloc_again)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again);
	unsigned int (*alloc_dgain)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain);
} TX_ISP_SENSOR_CTRL;

#define TX_ISP_GAIN_FIXED_POINT 16
struct tx_isp_sensor_attribute{
	const char *name;
	unsigned int chip_id;
	enum tx_sensor_control_bus_type cbus_type;
	unsigned int cbus_mask;
	unsigned int cbus_device;
	enum tx_sensor_data_bus_type dbus_type;
	union {
		struct tx_isp_mipi_bus 		mipi;
		struct tx_isp_dvp_bus 		dvp;
		struct tx_isp_bt1120_bus 	bt1120;
		struct tx_isp_bt656_bus		bt656bus;
		struct tx_isp_bt601_bus		bt601bus;
	};
	unsigned int max_again;	//the format is .16
	unsigned int max_dgain;	//the format is .16
	unsigned int again;
	unsigned int dgain;
	unsigned short min_integration_time;
	unsigned short min_integration_time_native;
	unsigned short max_integration_time_native;
	unsigned short integration_time_limit;
	unsigned int integration_time;
	unsigned short total_width;
	unsigned short total_height;
	unsigned short max_integration_time;
	unsigned short integration_time_apply_delay;
	unsigned short again_apply_delay;
	unsigned short dgain_apply_delay;
	unsigned short one_line_expr_in_us;
	TX_ISP_SENSOR_CTRL sensor_ctrl;
	void *priv; /* point to struct tx_isp_sensor_board_info */
};

/* define common struct */
enum tx_isp_priv_ioctl_direction {
	TX_ISP_PRIVATE_IOCTL_SET,
	TX_ISP_PRIVATE_IOCTL_GET,
};
enum tx_isp_priv_ioctl_command {
	/* the commands of pipeline are defined as follows. */
	TX_ISP_PRIVATE_IOCTL_MODULE_CLK,
	TX_ISP_PRIVATE_IOCTL_SYNC_VIDEO_IN,
	/* the commands of sensor are defined as follows. */
	TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME,
	TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN,
	TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN,
	TX_ISP_PRIVATE_IOCTL_SENSOR_FPS,
	TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL,
	TX_ISP_PRIVATE_IOCTL_SENSOR_WDR,
	TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE,
	TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE,
	TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE,
	/* the commands of frame-channel are defined as follows. */
	TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_BYPASS_ISP,
	TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_ENUM_FMT,
	TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_TRY_FMT,
	TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SET_FMT,
	TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_CROP_CAP,
	TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SET_CROP,
	TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SCALER_CAP,
	TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SET_SCALER,
	TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_STREAM_ON,
	TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_STREAM_OFF,
	TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_QUEUE_BUFFER,
};
struct isp_private_ioctl {
	enum tx_isp_priv_ioctl_direction dir;
	enum tx_isp_priv_ioctl_command cmd;
	int value;
};

struct frame_image_scalercap {
	unsigned short max_width;
	unsigned short max_height;
	unsigned short min_width;
	unsigned short min_height;
};
struct frame_image_scaler {
	unsigned short out_width;
	unsigned short out_height;
};

/* isp image tuning */
struct isp_image_tuning_default_ctrl {
	enum tx_isp_priv_ioctl_direction dir;
	struct v4l2_control control;
};

#define VIDIOC_ISP_PRIVATE_IOCTL	 _IOW('V', BASE_VIDIOC_PRIVATE, struct isp_private_ioctl)
#define VIDIOC_REGISTER_SENSOR		 _IOW('V', BASE_VIDIOC_PRIVATE + 1, struct v4l2_tx_isp_sensor_register_info)
#define VIDIOC_RELEASE_SENSOR		 _IOW('V', BASE_VIDIOC_PRIVATE + 2, struct v4l2_tx_isp_sensor_register_info)
//#define VIDIOC_DEFAULT_CMD_BYPASS_ISP	 _IOW('V', BASE_VIDIOC_PRIVATE + 3, int)
#define VIDIOC_DEFAULT_CMD_SCALER_CAP	 _IOWR('V', BASE_VIDIOC_PRIVATE + 3, struct frame_image_scalercap)
#define VIDIOC_DEFAULT_CMD_SET_SCALER	 _IOW('V', BASE_VIDIOC_PRIVATE + 4, struct frame_image_scaler)

#define VIDIOC_DEFAULT_CMD_ISP_TUNING	 _IOWR('V', BASE_VIDIOC_PRIVATE + 6, struct isp_image_tuning_default_ctrl)

enum tx_isp_vidioc_default_command {
	TX_ISP_VIDIOC_DEFAULT_CMD_BYPASS_ISP,
	TX_ISP_VIDIOC_DEFAULT_CMD_SCALER_CAP,
	TX_ISP_VIDIOC_DEFAULT_CMD_SET_SCALER,
};

enum tx_isp_frame_channel_bypass_isp{
	TX_ISP_FRAME_CHANNEL_BYPASS_ISP_DISABLE,
	TX_ISP_FRAME_CHANNEL_BYPASS_ISP_ENABLE,
};

enum {
	TX_ISP_STATE_STOP,
	TX_ISP_STATE_START,
	TX_ISP_STATE_RUN,
};

enum {
	TX_ISP_PAD_SOURCE,
	TX_ISP_PAD_LINK,
	TX_ISP_PADS_NUM,
};

struct tx_isp_video_in {
	struct v4l2_mbus_framefmt mbus;
	struct tx_isp_sensor_attribute *attr;
	unsigned int vi_max_width;	//the max width of sensor output setting
	unsigned int vi_max_height;	//the max height of sensor output setting
	unsigned int fps;
	int grp_id;
};

enum tx_isp_notify_statement {
	TX_ISP_NOTIFY_LINK_SETUP = 0x10,
	TX_ISP_NOTIFY_LINK_DESTROY,
};

enum tx_isp_notification {
	TX_ISP_NOTIFY_GET_PIPELINE,
	TX_ISP_NOTIFY_SYNC_VIDEO_IN,
	TX_ISP_NOTIFY_ENABLE_IRQ,
	TX_ISP_NOTIFY_DISABLE_IRQ,
	TX_ISP_NOTIFY_MASK_IRQ,
	TX_ISP_NOTIFY_UNMASK_IRQ,
	TX_ISP_NOTIFY_PRIVATE_IOCTL,
};

struct tx_isp_notify_argument{
	int value;
	int ret;
};

/***************************************
* some structs about pipeline of isp.
***************************************/
#if 1
struct tx_isp_media_pipeline;

/*
 * Media pipeline operations to be called from within a video node,  i.e. the
 * last entity within the pipeline. Implemented by related media device driver.
 */
struct tx_isp_media_pipeline_ops {
	int (*prepare)(struct tx_isp_media_pipeline *p);// struct tx_isp_video_in *vin);
	int (*unprepare)(struct tx_isp_media_pipeline *p);
	int (*reset)(struct tx_isp_media_pipeline *p, int state);
	int (*init)(struct tx_isp_media_pipeline *p, int state);
	int (*set_stream)(struct tx_isp_media_pipeline *p, int state);
};

#define tx_isp_pipeline_call(ent, op, args...)				  \
	(!(ent) ? -ENOENT : (((ent)->ops && (ent)->ops->op) ? \
	(ent)->ops->op(((ent)), ##args) : -ENOIOCTLCMD))

#endif

#define tx_isp_sd_notify(ent, args...)				  \
	(!(ent) ? -ENOENT : (((ent)->v4l2_dev && (ent)->v4l2_dev->notify) ? \
	(ent)->v4l2_dev->notify(((ent)), ##args) : -ENOIOCTLCMD))
/*
 * This structure represents a chain of media entities, including a data
 * source entity (e.g. an image sensor subdevice), a data capture entity
 * - a video capture device node and any remaining entities.
 */
struct tx_isp_media_pipeline {
	struct media_pipeline mp;
	const struct tx_isp_media_pipeline_ops *ops;
	struct v4l2_subdev *subdevs[TX_ISP_MAX_GRP_IDX];
};

#define to_tx_isp_pipeline(_ep) container_of(_ep, struct tx_isp_pipeline, ep)

struct tx_isp_sensor{
	struct v4l2_subdev sd;
	int index;
	unsigned int type;
	struct list_head list;
	struct v4l2_tx_isp_sensor_register_info info;
	struct tx_isp_sensor_attribute attr;
	struct tx_isp_video_in video;
	struct clk *mclk;
	void *priv;
};

#define tx_isp_readl(base, reg)		__raw_readl((base) + (reg))
#define tx_isp_writel(base, reg, value)		__raw_writel((value), ((base) + (reg)))
#define tx_isp_readw(base, reg)		__raw_readw((base) + (reg))
#define tx_isp_writew(base, reg, value)		__raw_writew((value), ((base) + (reg)))
#define tx_isp_readb(base, reg)		__raw_readb((base) + (reg))
#define tx_isp_writeb(base, reg, value)		__raw_writeb((value), ((base) + (reg)))


/* these structs are private */
struct tx_isp_driver_fh {
	enum v4l2_priority prio;
};

#endif /*__TX_ISP_COMMON_H__*/
