#ifndef __SNX_LIB_H__
#define __SNX_LIB_H__

#include <iostream>


// ---------------------------------
// V4L2 Capture parameters
// ---------------------------------
struct V4L2DeviceParameters 
{
	V4L2DeviceParameters(const char* devname, unsigned int format, unsigned int width, unsigned int height, int fps, int isp_fps, int verbose, int bitrate, int m2m_en, int gop, int mjpeg_qp, int queueSize) : 
		m_devName(devname), m_format(format), m_width(width), m_height(height), m_fps(fps), m_isp_fps(isp_fps), m_verbose(verbose), m_bitrate(bitrate), m_m2m_en(m2m_en), m_gop(gop), m_mjpeg_qp(mjpeg_qp), m_queueSize(queueSize) {};
		
	std::string m_devName;
	unsigned int m_format;
	unsigned int m_width;
	unsigned int m_height;
	int m_fps;
	int m_isp_fps;		
	int m_verbose;

	int m_bitrate;
	int m_m2m_en;
	int m_gop;
	int m_mjpeg_qp;
	int m_queueSize;
};



#ifdef __cplusplus
extern "C" {
#endif


enum RESOLUTION_TYPE
{
	//RESOLUTION_VGA_MJ
	RESOLUTION_HD_MJ,
	RESOLUTION_VGA_264,
	RESOLUTION_HD,
};

#define    IMG_DEV_NAME    "/dev/video0"
#define    CAP_DEV_NAME    "/dev/video1"
#define    CAP1_DEV_NAME    "/dev/video2"


/* recording setting */
#define AUDIO_RECORD_DEV    	"snx_audio_alaw"


/* audio recording setting */
#define    SAMPLE_RATE               8000        // 8K
#define    FORMAT_BIT                8
#define     READ_BYTE                800
#define MAX_FRAME_NUM                8192
#define AUDIO_COUNT_TIME_INTERVAL       ((1000 * READ_BYTE) / SAMPLE_RATE )      //unit: ms
#define AUDIO_ACCMULATION_COUNT         ((RECORDING_TIME * SAMPLE_RATE) / READ_BYTE)
#define AUDIO_SKIP_THRESHOLD         1 * (AUDIO_COUNT_TIME_INTERVAL)
#define AUDIO_INSERT_THRESHOLD       2 * (AUDIO_COUNT_TIME_INTERVAL)


#ifdef __cplusplus
}



#endif

#endif //__SNX_LIB_H__
