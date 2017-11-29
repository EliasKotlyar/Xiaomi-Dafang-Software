#include <iostream>
#include "ImpEncoder.h"
int main() {


    ImpEncoder* impEncoder = new ImpEncoder(1);
    int bytesRead = impEncoder->snap_jpeg();
    void* buffer = impEncoder->getBuffer();

    //int stream_fd = open("", O_RDWR);
    int ret = fwrite(buffer, bytesRead,1,stdout);

    return ret;
}
