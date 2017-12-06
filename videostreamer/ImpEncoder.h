#ifndef __SAMPLE_ENCODER_JPEG_H__
#define __SAMPLE_ENCODER_JPEG_H__

#include "impfuncs.h"
#include <imp/imp_encoder.h>
#define IMP_MODE_JPEG 1
#define IMP_MODE_H264_STREAM 2
#define IMP_MODE_H264_SNAP 3
#include <list>

struct impParams {
    int mode;
    int width;
    int height;
    int bitrate;
};


class ImpEncoder {
public:
    ImpEncoder(impParams params);
    ~ImpEncoder();
    int snap_jpeg();
    std::list <IMPEncoderPack> geth264frames();
    void requestIDR();
    int snap_h264();
    void* getBuffer();

private:
    int save_stream(void *buffer, IMPEncoderStream *stream);
    void* buffer;
    int encoderMode;
};



#endif