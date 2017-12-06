#include <iostream>
#include "ImpEncoder.h"
int main() {

    impParams params;
    params.width = 1280;
    params.height = 720;
    params.mode = IMP_MODE_H264_SNAP;
    params.framerate = 25;
    params.bitrate = 2000;

    ImpEncoder* impEncoder = new ImpEncoder(params);

    int ret;
    while(1){
        int bytesRead = impEncoder->snap_h264();
        void* buffer = impEncoder->getBuffer();
        ret = fwrite(buffer, bytesRead,1,stdout);
    }
    return ret;
}
