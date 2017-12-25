#include "ImpCapture.h"


ImpCapture::ImpCapture(impParams params) {
    impEncoder = new ImpEncoder(params);
}


int ImpCapture::getWidth() {
    return 320;
};

int ImpCapture::getHeight() {
    return 240;
};


size_t ImpCapture::read(char *buffer, size_t bufferSize) {
    int frameSize = impEncoder->snap_jpeg();
    memcpy(buffer,impEncoder->getBuffer(),frameSize);
    return frameSize;
}

int ImpCapture::getFd() {
    return 0;
}
unsigned long ImpCapture::getBufferSize(){
    return impEncoder->getBufferSize();
}



