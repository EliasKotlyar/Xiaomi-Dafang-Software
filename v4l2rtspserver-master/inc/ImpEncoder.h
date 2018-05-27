#ifndef __SAMPLE_ENCODER_JPEG_H__
#define __SAMPLE_ENCODER_JPEG_H__

#include <imp/imp_common.h>
#include <imp/imp_osd.h>
#include <imp/imp_framesource.h>
#include <imp/imp_isp.h>
#include <unistd.h>
#include <imp/imp_encoder.h>
#include <list>

#define IMP_MODE_JPEG 1
#define IMP_MODE_H264_STREAM 2
#define IMP_MODE_H264_SNAP 3
#define CH0_INDEX  0
#define CH1_INDEX  1
#define CHN_ENABLE 1
#define CHN_DISABLE 0
#define CHN0_EN                 1
#define CHN1_EN                 1
#define CROP_EN                    1
#define SENSOR_FRAME_RATE_NUM        25
#define SENSOR_FRAME_RATE_DEN        1
#define SENSOR_WIDTH_SECOND        640
#define SENSOR_HEIGHT_SECOND        360

#define FS_CHN_NUM            1
#define ENC_H264_CHANNEL        0
#define ENC_JPEG_CHANNEL        1
#define SLEEP_TIME            1
#define SENSOR_NAME                "jxf22"
#define SENSOR_CUBS_TYPE        TX_SENSOR_CONTROL_INTERFACE_I2C
#define SENSOR_I2C_ADDR            0x40
#define SENSOR_WIDTH            320
#define SENSOR_HEIGHT            240
#define STRING_MAX_SIZE          256

struct impParams {
    int mode;
    int width;
    int height;
    int bitrate;
    int framerate;
};
struct chn_conf {
    unsigned int index;//0 for main channel ,1 for second channel
    unsigned int enable;
    IMPFSChnAttr fs_chn_attr;
    IMPCell framesource_chn;
    IMPCell imp_encoder;
    IMPCell OSD_Cell;
};

class ImpEncoder {
public:
    ImpEncoder(impParams params);

    ~ImpEncoder();

    void requestIDR();

    int snap_h264(char *buffer);

    bool listEmpty();

    IMPEncoderPack getFrame();

    void static setNightVision(bool state);

private:
    int save_stream(void *buffer, IMPEncoderStream *stream);

    int encoderMode;

    int sample_system_init();


    int sample_system_exit();

    int sample_framesource_streamon();

    int sample_framesource_streamoff();

    int sample_framesource_init();

    int sample_framesource_exit();

    int sample_encoder_init();

    int sample_jpeg_init();

    int sample_encoder_exit(void);

    //IMPRgnHandle *sample_osd_init(int grpNum, int, int,int);

    int sample_osd_exit(IMPRgnHandle *prHandle, int grpNum);


    IMPSensorInfo sensor_info;

    impParams currentParams;

    int encoder_chn_exit(int encChn);

    chn_conf chn;

  //  std::list <IMPEncoderPack> frameList;
    pthread_mutex_t m_mutex;


};


#endif
