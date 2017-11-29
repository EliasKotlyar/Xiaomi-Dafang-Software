#ifndef __SAMPLE_ENCODER_JPEG_H__
#define __SAMPLE_ENCODER_JPEG_H__


class ImpEncoder {
public:
    ImpEncoder(int mode);
    ~ImpEncoder();
    int snap_jpeg();
    int snap_h264();
    void* getBuffer();
};



#endif