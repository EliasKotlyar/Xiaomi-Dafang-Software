#ifndef __SAMPLE_ENCODER_JPEG_H__
#define __SAMPLE_ENCODER_JPEG_H__

#if __cplusplus
extern "C"
{
#endif

void imp_init();
int imp_get_jpeg(void* buffer);
void imp_shutdown();


#if __cplusplus
}
#endif

#endif