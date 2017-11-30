#include <iostream>
#include "ImpEncoder.h"
int main() {


    ImpEncoder* impEncoder = new ImpEncoder(2);
    int bytesRead = impEncoder->snap_h264();
    void* buffer = impEncoder->getBuffer();
    int ret;
    ret = fwrite(buffer, bytesRead,1,stdout);
    return ret;
}
