#include <iostream>
#include "ImpEncoder.h"
int main() {


    ImpEncoder* impEncoder = new ImpEncoder(IMP_MODE_H264,320,240);
    int ret;
    while(1){
        int bytesRead = impEncoder->snap_h264();
        void* buffer = impEncoder->getBuffer();
        ret = fwrite(buffer, bytesRead,1,stdout);
    }
    return ret;
}
