#include <iostream>
#include "ImpEncoder.h"
int main() {


    ImpEncoder* impEncoder = new ImpEncoder(2);
    int ret,i;
    for(i = 0; i < 1000; i++){
        int bytesRead = impEncoder->snap_h264();
        void* buffer = impEncoder->getBuffer();
        ret = fwrite(buffer, bytesRead,1,stdout);
    }

    return ret;
}
