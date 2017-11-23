#ifndef __SAMPLE_ENCODER_JPEG_H__
#define __SAMPLE_ENCODER_JPEG_H__

#if __cplusplus
extern "C"
{
#endif

int imp_init(int mode);
int imp_get_jpeg(void* buffer);
int imp_shutdown();
int imp_get_h264_frame(void* buffer);

#if __cplusplus
}
#endif

#endif