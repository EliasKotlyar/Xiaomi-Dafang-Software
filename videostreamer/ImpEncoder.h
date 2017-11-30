#ifndef __SAMPLE_ENCODER_JPEG_H__
#define __SAMPLE_ENCODER_JPEG_H__

#include "impfuncs.h"
#include <imp/imp_encoder.h>
#define IMP_BUFFER_SIZE 200000
#define IMP_MODE_JPEG 1
#define IMP_MODE_H264 2

class ImpEncoder {
public:
    ImpEncoder(int mode,int width,int height);
    ~ImpEncoder();
    int snap_jpeg();
    int snap_h264();
    void* getBuffer();

private:
    int save_stream(void *buffer, IMPEncoderStream *stream);
    void* buffer;
    int encoderMode;
};



#endif