#include "ImpCapture.h"


ImpCapture::ImpCapture() {
    impParams params;
    params.width = 320;
    params.height = 240;
    params.mode = IMP_MODE_JPEG;
    params.framerate = 25;
    params.nightvision = false;
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
    return this->getWidth() * this->getHeight();
}



