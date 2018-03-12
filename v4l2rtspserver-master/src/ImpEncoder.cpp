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
#include <imp/imp_ivs.h>
#include <imp/imp_ivs_move.h>


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
#include "logger.h"

#define TAG "Sample-Encoder-jpeg"


/*
   {
   .index = CH1_INDEX,
   .enable = CHN1_EN,
   .fs_chn_attr = {
   .pixFmt = PIX_FMT_NV12,
   .outFrmRateNum = SENSOR_FRAME_RATE_NUM,
   .outFrmRateDen = SENSOR_FRAME_RATE_DEN,
   .nrVBs = 3,
   .type = FS_PHY_CHANNEL,

   .crop.enable = CROP_EN,
   .crop.top = 0,
   .crop.left = 0,
   .crop.width = SENSOR_WIDTH,
   .crop.height = SENSOR_HEIGHT,

   .scaler.enable = 1,
   .scaler.outwidth = SENSOR_WIDTH_SECOND,
   .scaler.outheight = SENSOR_HEIGHT_SECOND,

   .picWidth = SENSOR_WIDTH_SECOND,
   .picHeight = SENSOR_HEIGHT_SECOND,
   },
   .framesource_chn =    {DEV_ID_FS, 1, 0},
   .imp_encoder = {DEV_ID_ENC, 1, 0},
   },
 */

// ---- OSD
//

//#include <time.h>
#include "Fontmap.h"
#include "FontMapBig.h"
#include "sharedmem.h"
#include "../../v4l2rtspserver-tools/sharedmem.h"
#include "../inc/imp/imp_encoder.h"

#define OSD_REGION_HEIGHT               CHARHEIGHT_BIG

int grpNum = 0;
unsigned int gRegionH = 0;
unsigned gRegionW = 0;
IMPRgnHandle *prHander = NULL;
IMPOSDGrpRgnAttr grAttrFont;
IMPRgnHandle rHanderFont;
int gwidth;
int gheight;
int gpos;


static void set_osd_posY(int width, int height, int fontSize, int posY) {
    int ret = 0;
    IMPOSDRgnAttr rAttrFont;
    memset(&rAttrFont, 0, sizeof(IMPOSDRgnAttr));
    rAttrFont.type = OSD_REG_PIC;
    rAttrFont.rect.p0.x = 0;
    rAttrFont.rect.p0.y = posY;
    rAttrFont.rect.p1.x= rAttrFont.rect.p0.x + width -1;
    rAttrFont.rect.p1.y = rAttrFont.rect.p0.y + OSD_REGION_HEIGHT - 1;
    rAttrFont.fmt = PIX_FMT_BGRA;
    gRegionH = OSD_REGION_HEIGHT;
    gRegionW = width;

    rAttrFont.data.picData.pData = NULL;
    ret= IMP_OSD_SetRgnAttr(rHanderFont, &rAttrFont);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_OSD_SetRgnAttr TimeStamp error !\n");
    }
}

static void set_osd_pos(int width, int height, int fontSize, int pos) {

    // 1 is down
    if (pos == 1) {
        set_osd_posY(width, height, fontSize, height - (fontSize));
    } else {
        set_osd_posY(width, height, fontSize, 0);
    }
}

static IMPRgnHandle *sample_osd_init(int grpNum, int width, int height, int pos) {
    int ret;

    gwidth= width;
    gheight = height;
    gpos = pos;

    prHander = (IMPRgnHandle *) malloc(1 * sizeof(IMPRgnHandle));
    if (prHander <= 0) {
        IMP_LOG_ERR(TAG, "malloc() error !\n");
        return NULL;
    }

    rHanderFont = IMP_OSD_CreateRgn(NULL);
    if (rHanderFont == INVHANDLE) {
        IMP_LOG_ERR(TAG, "IMP_OSD_CreateRgn TimeStamp error !\n");
        return NULL;
    }

    ret = IMP_OSD_RegisterRgn(rHanderFont, grpNum, NULL);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IVS IMP_OSD_RegisterRgn failed\n");
        return NULL;
    }

    set_osd_pos(width,height,OSD_REGION_HEIGHT, pos);

    if (IMP_OSD_GetGrpRgnAttr(rHanderFont, grpNum, &grAttrFont) < 0) {
        IMP_LOG_ERR(TAG, "IMP_OSD_GetGrpRgnAttr Logo error !\n");
        return NULL;

    }
    memset(&grAttrFont, 0, sizeof(IMPOSDGrpRgnAttr));
    grAttrFont.show = 0;

    /* Disable Font global alpha, only use pixel alpha. */
    grAttrFont.gAlphaEn = 0; 
    grAttrFont.fgAlhpa = 0;
    grAttrFont.bgAlhpa = 0;
    grAttrFont.layer = 1;

    if (IMP_OSD_SetGrpRgnAttr(rHanderFont, grpNum, &grAttrFont) < 0) {
        IMP_LOG_ERR(TAG, "IMP_OSD_SetGrpRgnAttr Logo error !\n");
        return NULL;
    }

    ret = IMP_OSD_Start(grpNum);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_OSD_Start TimeStamp, Logo, Cover and Rect error !\n");
        return NULL;
    }

    prHander[0] = rHanderFont;
    return prHander;
}

static int osd_show(void) {
    int ret;

    ret = IMP_OSD_ShowRgn(prHander[0], grpNum, 1);
    if (ret != 0) {
        IMP_LOG_ERR(TAG, "IMP_OSD_ShowRgn() timeStamp error\n");
        return -1;
    }
    return 0;
}
static uint32_t colorMap[] = { OSD_WHITE,OSD_BLACK, OSD_RED, OSD_GREEN, OSD_BLUE, OSD_GREEN | OSD_BLUE, OSD_RED|OSD_GREEN, OSD_BLUE|OSD_RED};

static void *update_thread(void *p) {
    int ret;

    /*generate time*/
    char DateStr[STRING_MAX_SIZE];
    time_t currTime;
    struct tm *currDate;
    char osdTimeDisplay[STRING_MAX_SIZE];
    IMPOSDRgnAttrData rAttrData;
    bitmapinfo_t * fontmap = gBgramap; 
    int fontSize = CHARHEIGHT;
    int fontWidth = fontmap['W' - STARTCHAR].width; // Take 'W' as the biggest char  

    uint32_t *data = (uint32_t *) malloc(gRegionW * gRegionH * 4);
    //strcpy(osdTimeDisplay, (char *) p);

    if (data == NULL) {
        IMP_LOG_ERR(TAG, "malloc timeStampData error\n");
        return NULL;
    }

    struct shared_conf currentConfig;
    shared_conf *newConfig;
    SharedMem &sharedMem = SharedMem::instance();
    newConfig = sharedMem.getConfig();
    memcpy(&currentConfig, newConfig, sizeof(shared_conf));

    ret = osd_show();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "OSD show error\n");
        return NULL;
    }

    while (1) {
        int penpos_t = 0;
        int fontadv = 0;
        void *dateData;
        time(&currTime);
        currDate = localtime(&currTime);
        memset(DateStr, 0, sizeof(DateStr));
        memset(data, 0, gRegionW * gRegionH * 4);
        strftime(DateStr, STRING_MAX_SIZE, osdTimeDisplay, currDate);
        //strftime(DateStr, 40, "%Y-%m-%d %I:%M:%S", currDate);
        // For all char in string
        for (int i = 0; DateStr[i] != 0; i++) {
            if (DateStr[i] == ' ') {
                penpos_t += SPACELENGHT * 2;
            }
            //Check if the char is in the font
            else if (DateStr[i] >= STARTCHAR && DateStr[i] <= ENDCHAR) {
                // Get the right font pointer
                dateData = (void *) fontmap[DateStr[i] - STARTCHAR].pdata;
                // Fonts are stored in bytes
                fontadv = fontmap[DateStr[i] - STARTCHAR].widthInByte * 8;
                //Check if their is still room
                if (penpos_t + fontmap[DateStr[i] - STARTCHAR].width <= gRegionW - 80) {
                    for (int j = 0; j < fontSize; j++) {
                        for (int x = 0 ; x < fontadv ;x++)
                        {
                            if (((uint32_t *) dateData)[x+fontadv*j]) {
                                ((uint32_t *) data) [j * (gRegionW) + x + penpos_t] = colorMap[currentConfig.osdColor]; //((uint32_t *) dateData)[x+fontadv*j]; 
                            }
                            else {
                                ((uint32_t *) data) [j * (gRegionW) + x + penpos_t] = 0; 
                            }
                        }
                    }
                    // Move the cursor to the next position, depending on configured width and/or space between chars
                    if (currentConfig.osdFixedWidth == true)
                        penpos_t += fontWidth+currentConfig.osdSpace; 
                    else
                        penpos_t += fontadv+currentConfig.osdSpace; 
                } else {
                    LOG(NOTICE) << "No more space to display " << DateStr + i;
                    break;
                }
            } else {
                LOG(NOTICE) << "Character " << DateStr[i] << " is not supported";
            }
        }
        rAttrData.picData.pData = data;
        IMP_OSD_UpdateRgnAttrData(prHander[0], &rAttrData);

        sleep(1);

        //IMP_LOG_ERR(TAG, "THread Running...%d,%d\n",newConfig->flip,newConfig->nightmode);

        sharedMem.readConfig();
        if (currentConfig.flip != newConfig->flip) {
            IMP_LOG_ERR(TAG, "Changed FLIP\n");
            if (newConfig->flip == 1) {
                IMP_ISP_Tuning_SetISPVflip(IMPISP_TUNING_OPS_MODE_ENABLE);
                IMP_ISP_Tuning_SetISPHflip(IMPISP_TUNING_OPS_MODE_ENABLE);
            } else {
                IMP_ISP_Tuning_SetISPVflip(IMPISP_TUNING_OPS_MODE_DISABLE);
                IMP_ISP_Tuning_SetISPHflip(IMPISP_TUNING_OPS_MODE_DISABLE);
            }
        }

        if (currentConfig.nightmode != newConfig->nightmode) {
            IMP_LOG_ERR(TAG, "Changed NIGHTVISION\n");
            ImpEncoder::setNightVision(newConfig->nightmode);
        }
        if (currentConfig.bitrate != newConfig->bitrate) {
            IMP_LOG_ERR(TAG, "Changed Bitrate\n");
            IMPEncoderRcAttr attr;
            IMP_Encoder_GetChnRcAttr(0, &attr);
            attr.attrH264Cbr.outBitRate = (uint)newConfig->bitrate;
            IMP_Encoder_SetChnRcAttr(0, &attr);
        }

        if (strcmp(currentConfig.osdTimeDisplay, newConfig->osdTimeDisplay) != 0) {
            IMP_LOG_ERR(TAG, "Changed OSD\n");
            strcpy(osdTimeDisplay, newConfig->osdTimeDisplay);
        }

        if (currentConfig.osdColor != newConfig->osdColor) {
            if ((unsigned int)newConfig->osdColor<sizeof(colorMap) / sizeof(colorMap[0])) {
                IMP_LOG_ERR(TAG, "Changed OSD color\n");
                currentConfig.osdColor = newConfig->osdColor;
            }
            else {
                newConfig->osdColor = currentConfig.osdColor;
            }
        }

        if ((currentConfig.osdSize != newConfig->osdSize) ||
                (currentConfig.osdPosY != newConfig->osdPosY)) {
            currentConfig.osdSize = newConfig->osdSize;
            currentConfig.osdPosY = newConfig->osdPosY;
            if (currentConfig.osdSize == 0) {
                fontmap = gBgramap; 
                fontSize = CHARHEIGHT;
            } else {
                fontmap = gBgramapBig;
                fontSize = CHARHEIGHT_BIG;
            }
            fontWidth = fontmap['W' - STARTCHAR].width; // Take 'W' as the biggest char  

            // As the size changed, re-display the OSD
            set_osd_posY(gwidth,gheight,fontSize, currentConfig.osdPosY);
            IMP_LOG_ERR(TAG, "Changed OSD size and/or OSD pos\n");
        }

        if (currentConfig.osdSpace != newConfig->osdSpace) {
            currentConfig.osdSpace = newConfig->osdSpace;
            // As the size changed, re-display the OSD
            IMP_LOG_ERR(TAG, "Changed OSD space\n");
        }
        if (currentConfig.osdFixedWidth != newConfig->osdFixedWidth) {
            currentConfig.osdFixedWidth = newConfig->osdFixedWidth;
            // As the size changed, re-display the OSD
            IMP_LOG_ERR(TAG, "Changed OSD FixedWidth\n");
        }
        memcpy(&currentConfig, newConfig, sizeof(shared_conf));
    }

    return NULL;
}
// ---- END OSD
//


void *ImpEncoder::getBuffer() {
    return buffer;
}

int ImpEncoder::getBufferSize() {
    return bufferSize;
}


static int sample_ivs_move_start(int grp_num, int chn_num, IMPIVSInterface **interface, int width, int height)
{
    int ret = 0;
    IMP_IVS_MoveParam param;

    memset(&param, 0, sizeof(IMP_IVS_MoveParam));
    // Skip to 50 avoid detection at startup, not sure it impacts when running
    param.skipFrameCnt = 50;
    param.frameInfo.width = width;
    param.frameInfo.height = height;
    // Define the detection region, for now only one of the size of the video
    param.roiRectCnt = 1;
    // Sensibility (to make configurable)
    param.sense[0] = 4;

    param.roiRect[0].p0.x = 0;
    param.roiRect[0].p0.y = 0;
    param.roiRect[0].p1.x = param.frameInfo.width - 1;
    param.roiRect[0].p1.y = param.frameInfo.height  - 1;
    LOG(NOTICE) << "Detection region= ((" << param.roiRect[0].p0.x << "," << param.roiRect[0].p0.y << ")-("<< param.roiRect[0].p1.x << "," << param.roiRect[0].p1.y << "))\n";


    *interface = IMP_IVS_CreateMoveInterface(&param);
    if (*interface == NULL) {
        IMP_LOG_ERR(TAG, "IMP_IVS_CreateGroup(%d) failed\n", grp_num);
        return -1;
    }

    ret = IMP_IVS_CreateChn(chn_num, *interface);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_IVS_CreateChn(%d) failed\n", chn_num);
        return -1;
    }

    ret = IMP_IVS_RegisterChn(grp_num, chn_num);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_IVS_RegisterChn(%d, %d) failed\n", grp_num, chn_num);
        return -1;
    }

    ret = IMP_IVS_StartRecvPic(chn_num);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_IVS_StartRecvPic(%d) failed\n", chn_num);
        return -1;
    }

    return 0;
}

static void *sample_ivs_move_get_result_process(void *arg)
{
    int i = 0, ret = 0;
    int chn_num = 0; 
    IMP_IVS_MoveOutput *result = NULL;
    bool isWasOn = false;

    while (1) {
        ret = IMP_IVS_PollingResult(chn_num, IMP_IVS_DEFAULT_TIMEOUTMS);
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "IMP_IVS_PollingResult(%d, %d) failed\n", chn_num, IMP_IVS_DEFAULT_TIMEOUTMS);
            return (void *)-1;
        }
        ret = IMP_IVS_GetResult(chn_num, (void **)&result);
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "IMP_IVS_GetResult(%d) failed\n", chn_num);
            return (void *)-1;
        }

        // Detection !!!
        if (isWasOn == false &&
                result->retRoi[0] == 1)
        {
            isWasOn = true;
            system("/system/sdcard/scripts/detectionOn.sh");
            LOG(NOTICE) << "Detect !!\n";
        }

        if (isWasOn == true &&
                result->retRoi[0] == 0)
        {
            isWasOn = false;
            system("/system/sdcard/scripts/detectionOff.sh");
            LOG(NOTICE) << "Detect finished!!\n";
        }

        IMP_LOG_INFO(TAG, "result->retRoi(%d)\n", result->retRoi[0]);

        ret = IMP_IVS_ReleaseResult(chn_num, (void *)result);
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "IMP_IVS_ReleaseResult(%d) failed\n", chn_num);
            return (void *)-1;
        }
    }

    return (void *)0;
}


ImpEncoder::ImpEncoder(impParams params) {
    currentParams = params;
    framesCount = 0;

    // Init Structure:
    memset(&chn, 0, sizeof(chn_conf));

    chn.index = 0;
    chn.enable = 1;
    chn.fs_chn_attr.pixFmt = PIX_FMT_NV12;
    chn.fs_chn_attr.outFrmRateNum = currentParams.framerate;
    chn.fs_chn_attr.outFrmRateDen = 1;
    chn.fs_chn_attr.nrVBs = 3;
    chn.fs_chn_attr.type = FS_PHY_CHANNEL;

    chn.fs_chn_attr.crop.enable = 0;
    chn.fs_chn_attr.crop.width = currentParams.width;
    chn.fs_chn_attr.crop.height = currentParams.height;
    chn.fs_chn_attr.crop.top = 0;
    chn.fs_chn_attr.crop.left = 0;

    chn.fs_chn_attr.scaler.enable = 1;
    chn.fs_chn_attr.scaler.outwidth = currentParams.width;
    chn.fs_chn_attr.scaler.outheight = currentParams.height;


    chn.fs_chn_attr.picWidth = currentParams.width;
    chn.fs_chn_attr.picHeight = currentParams.height;

    chn.framesource_chn.deviceID = DEV_ID_FS;
    chn.framesource_chn.groupID = 0;
    chn.framesource_chn.outputID = 0;

    chn.imp_encoder.deviceID = DEV_ID_ENC;
    chn.imp_encoder.groupID = 0;
    chn.imp_encoder.outputID = 0;

    chn.OSD_Cell.deviceID = DEV_ID_OSD;
    chn.OSD_Cell.groupID = 0;
    chn.OSD_Cell.outputID = 0;


    encoderMode = currentParams.mode;
    int width = currentParams.width;
    int height = currentParams.height;
    int ret;

    bufferSize = width * height;
    buffer = malloc(bufferSize);

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


    ret = IMP_Encoder_CreateGroup(0);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_CreateGroup(%d) error !\n", 0);

    }

    ret = IMP_Encoder_CreateGroup(1);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_CreateGroup(%d) error !\n", 1);
    }


    /* Step.3 Encoder init */
    ret = sample_jpeg_init();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "Encoder JPEG init failed\n");

    }


    /* Step.3 Encoder init */
    ret = sample_encoder_init();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "Encoder h264 init failed\n");

    }


    // ----- OSD implementation: Init
    //
    if (IMP_OSD_CreateGroup(0) < 0) {
        IMP_LOG_ERR(TAG, "IMP_OSD_CreateGroup(0) error !\n");
    }
    int osdPos = 0; // 0 = UP,1 = down
    prHander = sample_osd_init(0, currentParams.width, currentParams.height, osdPos);
    if (prHander <= 0) {
        IMP_LOG_ERR(TAG, "OSD init failed\n");
    }

    /* Step Bind */
    ret = IMP_System_Bind(&chn.framesource_chn, &chn.OSD_Cell);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "Bind FrameSource channel0 and OSD failed\n");
    }

    ret = IMP_System_Bind(&chn.OSD_Cell, &chn.imp_encoder);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "Bind OSD and Encoder failed\n");
    }

    pthread_t tid;

    // ----- Motion implementation: Init
    //
    // Motion detection stuff, not sure it is optimized, maybe some calls are useless
    IMPCell ivs_grp0 = { DEV_ID_IVS , 0, 0};
    ret = IMP_IVS_CreateGroup(0);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_IVS_CreateGroup(0) failed\n");
    }

    ret = IMP_System_Bind (&chn.framesource_chn, &ivs_grp0);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_System_Binf for \n");
    }


    // --- OSD and other stuffs thread
    pthread_t tid;
    ret = pthread_create(&tid, NULL, update_thread, NULL);
    sleep(0);
    if (ret) {
        IMP_LOG_ERR(TAG, "thread create error\n");
    }

    ret = sample_framesource_streamon();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "ImpStreamOn failed\n");

    }


    // --- Motion 
    IMPIVSInterface *inteface = NULL;

    /*  ivs move start */
    ret = sample_ivs_move_start(0, 0, &inteface, currentParams.width, currentParams.height);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "sample_ivs_move_start(0, 0) failed\n");
    }

    /*  start to get ivs move result */
    if (pthread_create(&tid, NULL, sample_ivs_move_get_result_process, NULL)) {
        IMP_LOG_ERR(TAG, "create sample_ivs_move_get_result_process failed\n");
    }

    /* drop several pictures of invalid data */
    sleep(SLEEP_TIME);

    // JPEG
    ret = IMP_Encoder_StartRecvPic(1);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_StartRecvPic(%d) failed\n", 2);
    }

    // H264
    ret = IMP_Encoder_StartRecvPic(0);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_StartRecvPic(%d) failed\n", 0);
    }

    memset(&m_mutex, 0, sizeof(m_mutex));
    pthread_mutex_init(&m_mutex, NULL);


}


ImpEncoder::~ImpEncoder() {
    int ret;


    /* Step.b UnBind */
    ret = IMP_Encoder_StopRecvPic(1);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic() failed\n");

    }

    ret = IMP_Encoder_StopRecvPic(0);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic() failed\n");

    }






    /* Exit sequence as follow... */
    /* Step.a Stream Off */
    ret = sample_framesource_streamoff();
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "FrameSource StreamOff failed\n");

    }

    /* Step.b UnBind */

    ret = IMP_System_UnBind(&chn.framesource_chn, &chn.imp_encoder);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "UnBind FrameSource channel%d and Encoder failed\n", 0);

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
    int ret;
    int bytesRead = 0;



    /* Polling JPEG Snap, set timeout as 1000msec */
    ret = IMP_Encoder_PollingStream(1, 1000);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "Polling stream timeout\n");
        return -1;
    }

    IMPEncoderStream stream;
    /* Get JPEG Snap */
    ret = IMP_Encoder_GetStream(1, &stream, 1);
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

    IMP_Encoder_ReleaseStream(1, &stream);


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

    if (framesCount == currentParams.framerate * 2) {
        framesCount = 0;
        //
        //IMP_Encoder_FlushStream(0);
        //requestIDR();
    } else {
        framesCount++;
    }


    int i;
    for (i = 0; i < nr_frames; i++) {
        /* Polling H264 Stream, set timeout as 1000msec */
        ret = IMP_Encoder_PollingStream(0, 1000);
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "Polling stream timeout\n");
            continue;
        }

        IMPEncoderStream stream;
        /* Get H264 Stream */
        ret = IMP_Encoder_GetStream(0, &stream, 1);
        if (ret < 0) {
            IMP_LOG_ERR(TAG, "IMP_Encoder_GetStream() failed\n");
            return -1;
        }

        ret = save_stream(buffer, &stream);
        bytesRead = ret;
        if (ret < 0) {
            return ret;
        }

        IMP_Encoder_ReleaseStream(0, &stream);
    }


    return bytesRead;
}

bool ImpEncoder::listEmpty() {
    pthread_mutex_lock(&m_mutex);
    bool listEmpty = frameList.empty();
    pthread_mutex_unlock(&m_mutex);
    return listEmpty;
}

IMPEncoderPack ImpEncoder::getFrame() {
    pthread_mutex_lock(&m_mutex);
    IMPEncoderPack frame = frameList.front();
    frameList.pop_front();
    pthread_mutex_unlock(&m_mutex);
    return frame;
}


void ImpEncoder::requestIDR() {
    IMP_Encoder_RequestIDR(0);
}


int ImpEncoder::sample_system_init() {


    int ret = 0;

    memset(&sensor_info, 0, sizeof(IMPSensorInfo));
    memcpy(sensor_info.name, SENSOR_NAME, sizeof(SENSOR_NAME));
    sensor_info.cbus_type = SENSOR_CUBS_TYPE;
    memcpy(sensor_info.i2c.type, SENSOR_NAME, sizeof(SENSOR_NAME));
    sensor_info.i2c.addr = SENSOR_I2C_ADDR;

    //IMP_LOG_ERR(TAG, "Imp Log %d\n", IMP_Log_Get_Option());
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





    /*
       ret = IMP_ISP_Tuning_SetWDRAttr(IMPISP_TUNING_OPS_MODE_DISABLE);
       if (ret < 0) {
       IMP_LOG_ERR(TAG, "failed to set WDR\n");
       return -1;
       }
     */





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


    int ret = 0;
    /* Enable channels */

    ret = IMP_FrameSource_EnableChn(0);
    if (ret < 0) {
        dup2(saved_stdout, STDOUT_FILENO);
        IMP_LOG_ERR(TAG, "IMP_FrameSource_EnableChn(%d) error: %d\n", ret, 0);
        return -1;
    } else {
        fflush(stdout);
        dup2(saved_stdout, STDOUT_FILENO);
    }


    return 0;
}

int ImpEncoder::sample_framesource_streamoff() {
    int ret = 0;
    /* Enable channels */

    ret = IMP_FrameSource_DisableChn(0);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_FrameSource_DisableChn(%d) error: %d\n", ret, 0);
        return -1;
    }

    return 0;
}

int ImpEncoder::sample_framesource_init() {
    int ret;


    ret = IMP_FrameSource_CreateChn(0, &chn.fs_chn_attr);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_FrameSource_CreateChn(chn%d) error !\n", 0);
        return -1;
    }

    ret = IMP_FrameSource_SetChnAttr(0, &chn.fs_chn_attr);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_FrameSource_SetChnAttr(chn%d) error !\n", 0);
        return -1;
    }

    ret = IMP_FrameSource_CreateChn(1, &chn.fs_chn_attr);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_FrameSource_CreateChn(chn%d) error !\n", 1);
        return -1;
    }

    ret = IMP_FrameSource_SetChnAttr(1, &chn.fs_chn_attr);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_FrameSource_SetChnAttr(chn%d) error !\n", 1);
        return -1;
    }


    return 0;
}

int ImpEncoder::sample_framesource_exit() {
    int ret;


    /*Destroy channel i*/
    ret = IMP_FrameSource_DestroyChn(0);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_FrameSource_DestroyChn() error: %d\n", ret);
        return -1;
    }

    return 0;
}

int ImpEncoder::sample_jpeg_init() {
    int ret;
    IMPEncoderAttr *enc_attr;
    IMPEncoderCHNAttr channel_attr;
    IMPFSChnAttr *imp_chn_attr_tmp;


    imp_chn_attr_tmp = &chn.fs_chn_attr;
    memset(&channel_attr, 0, sizeof(IMPEncoderCHNAttr));
    enc_attr = &channel_attr.encAttr;
    enc_attr->enType = PT_JPEG;
    enc_attr->bufSize = 0;
    enc_attr->profile = 0;
    enc_attr->picWidth = imp_chn_attr_tmp->picWidth;
    enc_attr->picHeight = imp_chn_attr_tmp->picHeight;

    /* Create Channel */
    ret = IMP_Encoder_CreateChn(1, &channel_attr);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_CreateChn(%d) error: %d\n",
                0, ret);
        return -1;
    }

    /* Resigter Channel */
    ret = IMP_Encoder_RegisterChn(0, 1);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_RegisterChn(0, %d) error: %d\n",
                0, ret);
        return -1;
    }

    return 0;
}

int ImpEncoder::sample_encoder_init() {

    int ret;
    IMPEncoderAttr *enc_attr;
    IMPEncoderRcAttr *rc_attr;
    IMPFSChnAttr *imp_chn_attr_tmp;
    IMPEncoderCHNAttr channel_attr;


    imp_chn_attr_tmp = &chn.fs_chn_attr;
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
    rc_attr->attrH264Cbr.maxGop =
        2 * rc_attr->attrH264Cbr.outFrmRate.frmRateNum / rc_attr->attrH264Cbr.outFrmRate.frmRateDen;
    rc_attr->attrH264Cbr.outBitRate = currentParams.bitrate;
    rc_attr->attrH264Cbr.maxQp = 38;
    rc_attr->attrH264Cbr.minQp = 15;
    rc_attr->attrH264Cbr.maxFPS = imp_chn_attr_tmp->outFrmRateNum;
    rc_attr->attrH264Cbr.minFPS = 1;
    rc_attr->attrH264Cbr.IBiasLvl = 2;
    rc_attr->attrH264Cbr.FrmQPStep = 3;
    rc_attr->attrH264Cbr.GOPQPStep = 15;
    rc_attr->attrH264Cbr.AdaptiveMode = false;
    rc_attr->attrH264Cbr.GOPRelation = false;


    rc_attr->attrH264Denoise.enable = false;
    rc_attr->attrH264Denoise.dnType = 2;
    rc_attr->attrH264Denoise.dnIQp = 1;
    rc_attr->attrH264Denoise.dnPQp = 1;


    rc_attr->attrH264FrmUsed.enable = false;
    rc_attr->attrH264FrmUsed.frmUsedMode = ENC_FRM_SKIP;
    rc_attr->attrH264FrmUsed.frmUsedTimes = 2000;

    /*
       rc_attr->attrH264FrmUsed.enable = true;
       rc_attr->attrH264FrmUsed.dnIQp = ENC_FRM_REUSED ;
       rc_attr->attrH264FrmUsed.frmUsedTimes = 50;
     */




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
       100 ;
       rc_attr->attrH264Vbr.changePos = 50;
       rc_attr->attrH264Vbr.FrmQPStep = 3;
       rc_attr->attrH264Vbr.GOPQPStep = 15;
       rc_attr->attrH264FrmUsed.enable = 1;
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
       100 ;
       rc_attr->attrH264Vbr.changePos = 50;
       rc_attr->attrH264Vbr.FrmQPStep = 3;
       rc_attr->attrH264Vbr.GOPQPStep = 15;
     */


    ret = IMP_Encoder_CreateChn(0, &channel_attr);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_CreateChn(%d) error !\n", 0);
        return -1;
    }

    ret = IMP_Encoder_RegisterChn(0, 0);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_RegisterChn(%d, %d) error: %d\n",
                0, 0, ret);
        return -1;
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

    ret = encoder_chn_exit(0);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "Encoder Channel %d exit  error: %d\n",
                0, ret);
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


void ImpEncoder::geth264frames() {




    // Request it every 2 Seconds:



    if (framesCount == currentParams.framerate * 8) {
        framesCount = 0;
        //requestIDR();
        //IMP_Encoder_FlushStream(0);
    } else {
        framesCount++;
    }




    //requestIDR();


    int ret;
    /* H264 Channel start receive picture */


    unsigned int i;
    /* Polling H264 Stream, set timeout as 1000msec */
    ret = IMP_Encoder_PollingStream(0, 1000);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "Polling stream timeout\n");
    }

    IMPEncoderStream stream;
    /* Get H264 Stream */
    ret = IMP_Encoder_GetStream(0, &stream, 1);
    if (ret < 0) {
        IMP_LOG_ERR(TAG, "IMP_Encoder_GetStream() failed\n");

    }

    for (i = 0; i < stream.packCount; i++) {
        //printf("1. Got Frame with size %d\n",stream.pack[i].length);
        //if(stream.pack[i].dataType.h264Type == 5){
        IMPEncoderPack frame = stream.pack[i];
        if (i == 0) {
            void *frameAdr = (void *) (frame.virAddr);
            int frameSize = frame.length;
            frameAdr = (void *) ((int) (frameAdr) + 4);
            frameSize = frameSize - 4;
            frame.virAddr = (uint32_t) frameAdr;
            frame.length = frameSize;
        }
        pthread_mutex_lock(&m_mutex);
        frameList.push_back(frame);
        pthread_mutex_unlock(&m_mutex);
        //}

    }

    IMP_Encoder_ReleaseStream(0, &stream);

}

void ImpEncoder::setNightVision(bool state) {
    IMPISPRunningMode isprunningmode;
    IMPISPSceneMode sceneMode;
    IMPISPColorfxMode colormode;
    int ret;
    if (state) {
        isprunningmode = IMPISP_RUNNING_MODE_NIGHT;
        sceneMode = IMPISP_SCENE_MODE_NIGHT;
        colormode = IMPISP_COLORFX_MODE_BW;
    } else {
        isprunningmode = IMPISP_RUNNING_MODE_DAY;
        sceneMode = IMPISP_SCENE_MODE_AUTO;
        colormode = IMPISP_COLORFX_MODE_AUTO;
    }
    ret = IMP_ISP_Tuning_SetISPRunningMode(isprunningmode);
    if (ret) {
        IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetISPRunningMode error !\n");
    }
    ret = IMP_ISP_Tuning_SetSceneMode(sceneMode);
    if (ret) {
        IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetSceneMode error !\n");
    }

    ret = IMP_ISP_Tuning_SetColorfxMode(colormode);
    if (ret) {
        IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetColorfxMode error !\n");
    }


}

