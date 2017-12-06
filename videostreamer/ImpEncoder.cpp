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


#include "ImpEncoder.h"
#include <stdexcept>

#define TAG "Sample-Encoder-jpeg"

extern struct chn_conf chn[];


void *ImpEncoder::getBuffer() {
    return buffer;
}


ImpEncoder::ImpEncoder(impParams params) {

    currentParams = params;
    encoderMode = currentParams.mode;
    int width = currentParams.width;
    int height = currentParams.height;
    int ret;
    int i;

    buffer = malloc(width * height);

    /* Step.1 System init */
    ret = sample_system_init();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_System_Init() failed\n");
    }

    /* Step.2 FrameSource init */
    ret = sample_framesource_init();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "FrameSource init failed\n");

    }


    if (chn[0].enable) {
        ret = IMP_Encoder_CreateGroup(chn[0].index);
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "IMP_Encoder_CreateGroup(%d) error !\n", 0);

        }
    }


    if (encoderMode == IMP_MODE_JPEG) {
        /* Step.3 Encoder init */
        ret = sample_jpeg_init();
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "Encoder JPEG init failed\n");

        }

    } else {
        /* Step.3 Encoder init */
        ret = sample_encoder_init();
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "Encoder h264 init failed\n");

        }
    }



    /* Step.4 Bind */
    for (i = 0; i < FS_CHN_NUM; i++) {
        if (chn[i].enable) {
            ret = IMP_System_Bind(&chn[i].framesource_chn, &chn[i].imp_encoder);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "Bind FrameSource channel%d and Encoder failed\n", i);
            }
        }
    }


    /* Step.5 Stream On */
    ret = sample_framesource_streamon();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "ImpStreamOn failed\n");

    }
    //exit(0);

    /* drop several pictures of invalid data */
    sleep(SLEEP_TIME);

    if (encoderMode == IMP_MODE_JPEG) {
        for (i = 0; i < FS_CHN_NUM; i++) {
            if (chn[i].enable) {
                ret = IMP_Encoder_StartRecvPic(2 + chn[i].index);
                if (ret < 0) {
                    IMP_LOG_ERR(TAG, "IMP_Encoder_StartRecvPic(%d) failed\n", 2 + chn[i].index);
                }


            }
        }

    } else {
        ret = IMP_Encoder_StartRecvPic(ENC_H264_CHANNEL);
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "IMP_Encoder_StartRecvPic(%d) failed\n", ENC_H264_CHANNEL);
        }

    }


}


ImpEncoder::~ImpEncoder() {
    int i, ret;

    if (encoderMode == IMP_MODE_JPEG) {
        /* Step.b UnBind */
        ret = IMP_Encoder_StopRecvPic(2 + chn[0].index);
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic() failed\n");

        }
    } else {
        ret = IMP_Encoder_StopRecvPic(ENC_H264_CHANNEL);
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic() failed\n");

        }

    }





    /* Exit sequence as follow... */
    /* Step.a Stream Off */
    ret = sample_framesource_streamoff();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "FrameSource StreamOff failed\n");

    }

    /* Step.b UnBind */
    for (i = 0; i < FS_CHN_NUM; i++) {
        if (chn[i].enable) {
            ret = IMP_System_UnBind(&chn[i].framesource_chn, &chn[i].imp_encoder);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "UnBind FrameSource channel%d and Encoder failed\n", i);

            }
        }
    }

    /* Step.c Encoder exit */
    ret = sample_encoder_exit();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "Encoder exit failed\n");

    }

    /* Step.d FrameSource exit */
    ret = sample_framesource_exit();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "FrameSource exit failed\n");

    }

    /* Step.e System exit */
    ret = sample_system_exit();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "sample_system_exit() failed\n");

    }

}

int ImpEncoder::snap_jpeg() {
    int i, ret;
    int bytesRead = 0;

    for (i = 0; i < FS_CHN_NUM; i++) {
        if (chn[i].enable) {

            /* Polling JPEG Snap, set timeout as 1000msec */
            ret = IMP_Encoder_PollingStream(2 + chn[i].index, 100);
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
            // IMP_LOG_ERR(TAG,  "Read %d bytes \n", ret);
            //extractHeader(buffer,ret);
            // IMP_LOG_ERR(TAG,  "JPEG saved!\n");
            if (ret < 0) {

                return -1;
            }

            IMP_Encoder_ReleaseStream(2 + chn[i].index, &stream);


        }
    }
    return bytesRead;
}


int ImpEncoder::save_stream(void *buffer, IMPEncoderStream *stream) {
    int i, nr_pack = stream->packCount;
    // IMP_LOG_ERR(TAG,  "Pack count: %d\n", nr_pack);

    void *memoryAddress = buffer;
    int bytesRead = 0;
    for (i = 0; i < nr_pack; i++) {
        int packLen = stream->pack[i].length;
        memcpy(memoryAddress, (void *) stream->pack[i].virAddr, packLen);
        memoryAddress = (void *) ((int) memoryAddress + packLen);
        bytesRead = bytesRead + packLen;
        // IMP_LOG_ERR(TAG,  "Pack Len: %d\n", packLen);
    }

    return bytesRead;
}


int ImpEncoder::snap_h264() {
    int nr_frames = 1;
    int ret;
    int bytesRead = 0;
    /* H264 Channel start receive picture */



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
        bytesRead = ret;
        if (ret < 0) {
            return ret;
        }

        IMP_Encoder_ReleaseStream(ENC_H264_CHANNEL, &stream);
    }




    return bytesRead;
}


std::list <IMPEncoderPack> ImpEncoder::geth264frames() {

    std::list <IMPEncoderPack> frameList;

    int ret;
    /* H264 Channel start receive picture */



    unsigned int i;
    /* Polling H264 Stream, set timeout as 1000msec */
    ret = IMP_Encoder_PollingStream(ENC_H264_CHANNEL, 1000);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "Polling stream timeout\n");
    }

    IMPEncoderStream stream;
    /* Get H264 Stream */
    ret = IMP_Encoder_GetStream(ENC_H264_CHANNEL, &stream, 1);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_GetStream() failed\n");

    }

    for (i = 0; i < stream.packCount; i++) {
        //printf("1. Got Frame with size %d\n",stream.pack[i].length);
        frameList.push_back(stream.pack[i]);
    }

    IMP_Encoder_ReleaseStream(ENC_H264_CHANNEL, &stream);


    return frameList;
}
void ImpEncoder::requestIDR(){
    IMP_Encoder_RequestIDR(ENC_H264_CHANNEL);
}


int ImpEncoder::sample_system_init() {

    chn[0].fs_chn_attr.crop.width = currentParams.width;
    chn[0].fs_chn_attr.crop.height = currentParams.height;
    chn[0].fs_chn_attr.picWidth = currentParams.width;
    chn[0].fs_chn_attr.picHeight = currentParams.height;



    int ret = 0;

    memset(&sensor_info, 0, sizeof(IMPSensorInfo));
    memcpy(sensor_info.name, SENSOR_NAME, sizeof(SENSOR_NAME));
    sensor_info.cbus_type = SENSOR_CUBS_TYPE;
    memcpy(sensor_info.i2c.type, SENSOR_NAME, sizeof(SENSOR_NAME));
    sensor_info.i2c.addr = SENSOR_I2C_ADDR;

    IMP_LOG_ERR(TAG, "Imp Log %d\n",IMP_Log_Get_Option());
    //IMP_Log_Set_Option()

    ret = IMP_ISP_Open();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "failed to open ISP\n");
        return -1;
    }


    ret = IMP_ISP_AddSensor(&sensor_info);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "failed to AddSensor\n");
        return -1;
    }

    ret = IMP_ISP_EnableSensor();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "failed to EnableSensor\n");
        return -1;
    }


    ret = IMP_System_Init();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_System_Init failed\n");
        return -1;
    }




    /* enable turning, to debug graphics */

    ret = IMP_ISP_EnableTuning();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_ISP_EnableTuning failed\n");
        return -1;
    }


    ret = IMP_ISP_Tuning_SetWDRAttr(IMPISP_TUNING_OPS_MODE_DISABLE);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "failed to set WDR\n");
        return -1;
    }




    //IMP_LOG_ERR(TAG, "ImpSystemInit success\n");

    return 0;
}



int ImpEncoder::sample_system_exit() {
    int ret = 0;

    IMP_LOG_ERR(TAG, "sample_system_exit start\n");


    IMP_System_Exit();

    ret = IMP_ISP_DisableSensor();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "failed to EnableSensor\n");
        return -1;
    }

    ret = IMP_ISP_DelSensor(&sensor_info);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "failed to AddSensor\n");
        return -1;
    }

    ret = IMP_ISP_DisableTuning();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_ISP_DisableTuning failed\n");
        return -1;
    }

    if (IMP_ISP_Close()) {
        IMP_LOG_ERR(TAG, "failed to open ISP\n");
        return -1;
    }

    IMP_LOG_ERR(TAG, " sample_system_exit success\n");

    return 0;
}

int ImpEncoder::sample_framesource_streamon() {

    int out_pipe[2];
    int saved_stdout;
    saved_stdout = dup(STDOUT_FILENO);
    pipe(out_pipe);
    dup2(out_pipe[1], STDOUT_FILENO);


    int ret = 0, i = 0;
    /* Enable channels */
    for (i = 0; i < FS_CHN_NUM; i++) {
        if (chn[i].enable) {
            ret = IMP_FrameSource_EnableChn(chn[i].index);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "IMP_FrameSource_EnableChn(%d) error: %d\n", ret, chn[i].index);
                return -1;
            }
        }
    }
    fflush(stdout);

    dup2(saved_stdout, STDOUT_FILENO);
    return 0;
}

int ImpEncoder::sample_framesource_streamoff() {
    int ret = 0, i = 0;
    /* Enable channels */
    for (i = 0; i < FS_CHN_NUM; i++) {
        if (chn[i].enable) {
            ret = IMP_FrameSource_DisableChn(chn[i].index);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "IMP_FrameSource_DisableChn(%d) error: %d\n", ret, chn[i].index);
                return -1;
            }
        }
    }
    return 0;
}

int ImpEncoder::sample_framesource_init() {
    int i, ret;

    for (i = 0; i < FS_CHN_NUM; i++) {
        if (chn[i].enable) {
            ret = IMP_FrameSource_CreateChn(chn[i].index, &chn[i].fs_chn_attr);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "IMP_FrameSource_CreateChn(chn%d) error !\n", chn[i].index);
                return -1;
            }

            ret = IMP_FrameSource_SetChnAttr(chn[i].index, &chn[i].fs_chn_attr);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "IMP_FrameSource_SetChnAttr(chn%d) error !\n", chn[i].index);
                return -1;
            }
        }
    }

    return 0;
}

int ImpEncoder::sample_framesource_exit() {
    int ret, i;

    for (i = 0; i < FS_CHN_NUM; i++) {
        if (chn[i].enable) {
            /*Destroy channel i*/
            ret = IMP_FrameSource_DestroyChn(i);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "IMP_FrameSource_DestroyChn() error: %d\n", ret);
                return -1;
            }
        }
    }
    return 0;
}

int ImpEncoder::sample_jpeg_init() {
    int i, ret;
    IMPEncoderAttr *enc_attr;
    IMPEncoderCHNAttr channel_attr;
    IMPFSChnAttr *imp_chn_attr_tmp;

    for (i = 0; i < FS_CHN_NUM; i++) {
        if (chn[i].enable) {
            imp_chn_attr_tmp = &chn[i].fs_chn_attr;
            memset(&channel_attr, 0, sizeof(IMPEncoderCHNAttr));
            enc_attr = &channel_attr.encAttr;
            enc_attr->enType = PT_JPEG;
            enc_attr->bufSize = 0;
            enc_attr->profile = 0;
            enc_attr->picWidth = imp_chn_attr_tmp->picWidth;
            enc_attr->picHeight = imp_chn_attr_tmp->picHeight;

            /* Create Channel */
            ret = IMP_Encoder_CreateChn(2 + chn[i].index, &channel_attr);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "IMP_Encoder_CreateChn(%d) error: %d\n",
                            chn[i].index, ret);
                return -1;
            }

            /* Resigter Channel */
            ret = IMP_Encoder_RegisterChn(i, 2 + chn[i].index);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "IMP_Encoder_RegisterChn(0, %d) error: %d\n",
                            chn[i].index, ret);
                return -1;
            }
        }
    }

    return 0;
}

int ImpEncoder::sample_encoder_init() {
    int i, ret;
    IMPEncoderAttr *enc_attr;
    IMPEncoderRcAttr *rc_attr;
    IMPFSChnAttr *imp_chn_attr_tmp;
    IMPEncoderCHNAttr channel_attr;

    for (i = 0; i < FS_CHN_NUM; i++) {
        if (chn[i].enable) {
            imp_chn_attr_tmp = &chn[i].fs_chn_attr;
            memset(&channel_attr, 0, sizeof(IMPEncoderCHNAttr));
            enc_attr = &channel_attr.encAttr;
            enc_attr->enType = PT_H264;
            enc_attr->bufSize = 0;
            enc_attr->profile = 0;
            enc_attr->picWidth = imp_chn_attr_tmp->picWidth;
            enc_attr->picHeight = imp_chn_attr_tmp->picHeight;
            rc_attr = &channel_attr.rcAttr;



            rc_attr->rcMode = ENC_RC_MODE_H264CBR;
            rc_attr->attrH264Cbr.outFrmRate.frmRateNum = imp_chn_attr_tmp->outFrmRateNum;
            rc_attr->attrH264Cbr.outFrmRate.frmRateDen = imp_chn_attr_tmp->outFrmRateDen;
            rc_attr->attrH264Cbr.maxGop = 2 * rc_attr->attrH264Cbr.outFrmRate.frmRateNum / rc_attr->attrH264Cbr.outFrmRate.frmRateDen;
            rc_attr->attrH264Cbr.outBitRate = 2000 * (imp_chn_attr_tmp->picWidth * imp_chn_attr_tmp->picHeight) / (1280 * 720);
            rc_attr->attrH264Cbr.maxQp = 38;
            rc_attr->attrH264Cbr.minQp = 15;
            rc_attr->attrH264Cbr.maxFPS = 100;
            rc_attr->attrH264Cbr.minFPS = 1;
            rc_attr->attrH264Cbr.IBiasLvl = 2;
            rc_attr->attrH264Cbr.FrmQPStep = 3;
            rc_attr->attrH264Cbr.GOPQPStep = 15;
            rc_attr->attrH264Cbr.AdaptiveMode = false;
            rc_attr->attrH264Cbr.GOPRelation = false;

            /*
            rc_attr->attrH264FrmUsed.enable = true;
            rc_attr->attrH264FrmUsed.frmUsedMode = ENC_FRM_REUSED ;
            rc_attr->attrH264FrmUsed.frmUsedTimes = 50;
            */


            /*
            rc_attr->rcMode = ENC_RC_MODE_H264VBR;
            rc_attr->attrH264Vbr.outFrmRate.frmRateNum = imp_chn_attr_tmp->outFrmRateNum;
            rc_attr->attrH264Vbr.outFrmRate.frmRateDen = imp_chn_attr_tmp->outFrmRateDen;
            rc_attr->attrH264Vbr.maxGop =
                    1 * rc_attr->attrH264Vbr.outFrmRate.frmRateNum / rc_attr->attrH264Vbr.outFrmRate.frmRateDen;
            rc_attr->attrH264Vbr.maxQp = 38;
            rc_attr->attrH264Vbr.minQp = 15;
            rc_attr->attrH264Vbr.staticTime = 1;
            rc_attr->attrH264Vbr.maxBitRate =
                    100 * (imp_chn_attr_tmp->picWidth * imp_chn_attr_tmp->picHeight) / (1920 * 1080);
            rc_attr->attrH264Vbr.changePos = 50;
            rc_attr->attrH264Vbr.FrmQPStep = 3;
            rc_attr->attrH264Vbr.GOPQPStep = 15;
            rc_attr->attrH264FrmUsed.enable = 1;
             */


            ret = IMP_Encoder_CreateChn(chn[i].index, &channel_attr);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "IMP_Encoder_CreateChn(%d) error !\n", i);
                return -1;
            }

            ret = IMP_Encoder_RegisterChn(chn[i].index, chn[i].index);
            if (ret < 0) {
                IMP_LOG_ERR(TAG, "IMP_Encoder_RegisterChn(%d, %d) error: %d\n",
                            chn[i].index, chn[i].index, ret);
                return -1;
            }
        }
    }

    return 0;
}

int ImpEncoder::encoder_chn_exit(int encChn) {
    int ret;
    IMPEncoderCHNStat chn_stat;
    ret = IMP_Encoder_Query(encChn, &chn_stat);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_Query(%d) error: %d\n",
                    encChn, ret);
        return -1;
    }

    if (chn_stat.registered) {
        ret = IMP_Encoder_UnRegisterChn(encChn);
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "IMP_Encoder_UnRegisterChn(%d) error: %d\n",
                        encChn, ret);
            return -1;
        }

        ret = IMP_Encoder_DestroyChn(encChn);
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "IMP_Encoder_DestroyChn(%d) error: %d\n",
                        encChn, ret);
            return -1;
        }
    }

    return 0;
}

int ImpEncoder::sample_encoder_exit(void) {
    int ret;

    ret = encoder_chn_exit(ENC_H264_CHANNEL);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "Encoder Channel %d exit  error: %d\n",
                    ENC_H264_CHANNEL, ret);
        return -1;
    }

    ret = encoder_chn_exit(ENC_JPEG_CHANNEL);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "Encoder Channel %d exit  error: %d\n",
                    ENC_JPEG_CHANNEL, ret);
        return -1;
    }

    ret = IMP_Encoder_DestroyGroup(0);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_DestroyGroup(0) error: %d\n", ret);
        return -1;
    }

    return 0;
}
