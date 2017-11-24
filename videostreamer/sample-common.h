/*
 * sample-common.h
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 */

#ifndef __SAMPLE_COMMON_H__
#define __SAMPLE_COMMON_H__

#include <imp/imp_common.h>
#include <imp/imp_osd.h>
#include <imp/imp_framesource.h>
#include <imp/imp_isp.h>
#include <unistd.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

#define SENSOR_FRAME_RATE_NUM		25
#define SENSOR_FRAME_RATE_DEN		1


#define SENSOR_NAME				"jxf22"
#define SENSOR_CUBS_TYPE        TX_SENSOR_CONTROL_INTERFACE_I2C
#define SENSOR_I2C_ADDR			0x40
#define SENSOR_WIDTH			640
#define SENSOR_HEIGHT			480
#define CHN0_EN                 1
#define CHN1_EN                 1
#define CROP_EN					1


#define SENSOR_WIDTH_SECOND		640
#define SENSOR_HEIGHT_SECOND		360

#define NR_FRAMES_TO_SAVE		100
#define STREAM_BUFFER_SIZE		(1 * 1024 * 1024)

#define ENC_H264_CHANNEL		0
#define ENC_JPEG_CHANNEL		1

#define STREAM_FILE_PATH_PREFIX		"/tmp"
#define SNAP_FILE_PATH_PREFIX		"/tmp"

#define OSD_REGION_WIDTH		16
#define OSD_REGION_HEIGHT		34
#define OSD_REGION_WIDTH_SEC		8
#define OSD_REGION_HEIGHT_SEC   	18


#define SLEEP_TIME			1

#define FS_CHN_NUM			1  //MIN 1,MAX 2
#define IVS_CHN_ID          1

#define CH0_INDEX  0
#define CH1_INDEX  1
#define CHN_ENABLE 1
#define CHN_DISABLE 0

/*#define SUPPORT_RGB555LE*/

struct chn_conf{
	unsigned int index;//0 for main channel ,1 for second channel
	unsigned int enable;
	IMPFSChnAttr fs_chn_attr;
	IMPCell framesource_chn;
	IMPCell imp_encoder;
};

#define  CHN_NUM  ARRAY_SIZE(chn)

int sample_system_init();
int sample_system_exit();

int sample_framesource_streamon();
int sample_framesource_streamoff();

int sample_framesource_init();
int sample_framesource_exit();

int sample_encoder_init();
int sample_jpeg_init();
int sample_encoder_exit(void);

IMPRgnHandle *sample_osd_init(int grpNum);
int sample_osd_exit(IMPRgnHandle *prHandle,int grpNum);

int sample_do_get_h264_stream(int nr_frames);
int sample_get_h264_stream();
int sample_do_get_jpeg_snap(void);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SAMPLE_COMMON_H__ */
