#include <iostream>
#include "ImpEncoder.h"
int main() {


    ImpEncoder* impEncoder = new ImpEncoder(IMP_MODE_JPEG,1920,1080);
    int bytesRead = impEncoder->snap_jpeg();
    void* buffer = impEncoder->getBuffer();

    int ret;
    //int stream_fd = open("", O_RDWR);
    ret = fwrite(buffer, bytesRead,1,stdout);

    return ret;
}
