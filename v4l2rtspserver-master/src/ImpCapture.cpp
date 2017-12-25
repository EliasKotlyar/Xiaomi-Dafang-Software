#include "ImpCapture.h"


ImpCapture::ImpCapture(impParams params) {
    impEncoder = new ImpEncoder(params);
    height = params.height;
    width = params.width;
}


int ImpCapture::getWidth() {
    return this->width;
};

int ImpCapture::getHeight() {
    return this->height;
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



