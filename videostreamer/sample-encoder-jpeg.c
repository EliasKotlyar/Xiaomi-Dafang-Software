/*
 * sample-Encoder-jpeg.c
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 */

#include <stdio.h>

#include <imp/imp_log.h>
#include <imp/imp_common.h>
#include <imp/imp_system.h>
#include <imp/imp_framesource.h>
#include <imp/imp_encoder.h>


#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


#include "sample-common.h"

#define TAG "Sample-Encoder-jpeg"

extern struct chn_conf chn[];

int imp_init(int mode)
{
	int i, ret;

	/* Step.1 System init */
	ret = sample_system_init();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_System_Init() failed\n");
		return -1;
	}

	/* Step.2 FrameSource init */
	ret = sample_framesource_init();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "FrameSource init failed\n");
		return -1;
	}

	for (i = 0; i < FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			ret = IMP_Encoder_CreateGroup(chn[i].index);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_Encoder_CreateGroup(%d) error !\n", i);
				return -1;
			}
		}
	}

    if(mode == 1){
    	/* Step.3 Encoder init */
    	ret = sample_jpeg_init();
    	if (ret < 0) {
    		IMP_LOG_ERR(TAG, "Encoder JPEG init failed\n");
    		return -1;
    	}

    }else{
    	/* Step.3 Encoder init */
    	ret = sample_encoder_init();
    	if (ret < 0) {
    		IMP_LOG_ERR(TAG, "Encoder h264 init failed\n");
    		return -1;
    	}
    }



		/* Step.4 Bind */
	for (i = 0; i < FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			ret = IMP_System_Bind(&chn[i].framesource_chn, &chn[i].imp_encoder);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "Bind FrameSource channel%d and Encoder failed\n",i);
				return -1;
			}
		}
	}

	/* Step.5 Stream On */
	ret = sample_framesource_streamon();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "ImpStreamOn failed\n");
		return -1;
	}

	/* drop several pictures of invalid data */
	sleep(SLEEP_TIME);



	return 0;
}


int imp_shutdown(){
    int i, ret;
	/* Exit sequence as follow... */
	/* Step.a Stream Off */
	ret = sample_framesource_streamoff();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "FrameSource StreamOff failed\n");
		return -1;
	}

	/* Step.b UnBind */
	for (i = 0; i < FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			ret = IMP_System_UnBind(&chn[i].framesource_chn, &chn[i].imp_encoder);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "UnBind FrameSource channel%d and Encoder failed\n",i);
				return -1;
			}
		}
	}

	/* Step.c Encoder exit */
	ret = sample_encoder_exit();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "Encoder exit failed\n");
		return -1;
	}

	/* Step.d FrameSource exit */
	ret = sample_framesource_exit();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "FrameSource exit failed\n");
		return -1;
	}

	/* Step.e System exit */
	ret = sample_system_exit();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "sample_system_exit() failed\n");
		return -1;
	}

}

int imp_get_jpeg(void* buffer)
{
	int i, ret;
	char snap_path[64];
	 int bytesRead = 0;

	for (i = 0; i < FS_CHN_NUM; i++) {
		if (chn[i].enable) {
			ret = IMP_Encoder_StartRecvPic(2 + chn[i].index);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_Encoder_StartRecvPic(%d) failed\n", 2 + chn[i].index);
				return -1;
			}

			/* Polling JPEG Snap, set timeout as 1000msec */
			ret = IMP_Encoder_PollingStream(2 + chn[i].index, 1000);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "Polling stream timeout\n");
				continue;
			}

			IMPEncoderStream stream;
			/* Get JPEG Snap */
			ret = IMP_Encoder_GetStream(chn[i].index + 2, &stream, 1);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_Encoder_GetStream() failed\n");
				return -1;
			}

			ret = save_stream(buffer, &stream);
			bytesRead = ret;
			//extractHeader(buffer,ret);
			//IMP_LOG_ERR(TAG, "JPEG saved!\n");
			if (ret < 0) {

				return -1;
			}

			IMP_Encoder_ReleaseStream(2 + chn[i].index, &stream);



			ret = IMP_Encoder_StopRecvPic(2 + chn[i].index);
			if (ret < 0) {
				IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic() failed\n");
				return -1;
			}
		}
	}
	return bytesRead;
}


int save_stream(void* buffer, IMPEncoderStream *stream)
{
	int ret, i, nr_pack = stream->packCount;

    void* memoryAddress = buffer;
    int bytesRead = 0;
	for (i = 1; i < nr_pack; i++) {
	    memcpy(memoryAddress,(void *)stream->pack[i].virAddr,stream->pack[i].length);
		memoryAddress = memoryAddress + stream->pack[i].length;
		bytesRead = bytesRead + stream->pack[i].length;
	}

	return bytesRead;
}


int imp_get_h264_frame(void* buffer)
{
    int nr_frames = 1;
	int ret;

	/* H264 Channel start receive picture */
	ret = IMP_Encoder_StartRecvPic(ENC_H264_CHANNEL);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_StartRecvPic(%d) failed\n", ENC_H264_CHANNEL);
		return -1;
	}


	int i;
	for (i = 0; i < nr_frames; i++) {
		/* Polling H264 Stream, set timeout as 1000msec */
		ret = IMP_Encoder_PollingStream(ENC_H264_CHANNEL, 1000);
		if (ret < 0) {
			IMP_LOG_ERR(TAG, "Polling stream timeout\n");
			continue;
		}

		IMPEncoderStream stream;
		/* Get H264 Stream */
		ret = IMP_Encoder_GetStream(ENC_H264_CHANNEL, &stream, 1);
		if (ret < 0) {
			IMP_LOG_ERR(TAG, "IMP_Encoder_GetStream() failed\n");
			return -1;
		}

		ret = save_stream(buffer, &stream);
		if (ret < 0) {
			return ret;
		}

		IMP_Encoder_ReleaseStream(ENC_H264_CHANNEL, &stream);
	}



	ret = IMP_Encoder_StopRecvPic(ENC_H264_CHANNEL);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic() failed\n");
		return -1;
	}

	return 0;
}
