#include "ImpCapture.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
ImpCapture::ImpCapture(impParams params) {
    impEncoder = new ImpEncoder(params);
    height = params.height;
    width = params.width;
    mode = params.mode;
}


int ImpCapture::getWidth() {
    return this->width;
};

int ImpCapture::getHeight() {
    return this->height;
};


size_t ImpCapture::read(char *buffer, size_t bufferSize) {
    int frameSize;

    // Save to JPG
    frameSize = impEncoder->snap_jpeg();

    key_t key1;
    key1 = ftok("/usr/include", 'x');
    int shm_id;

    shm_id = shmget( key1, 0, 0);
    if(shm_id != -1){
        struct shmid_ds buf;
        shmctl(shm_id, IPC_STAT, &buf);
        int memlen = buf.shm_segsz;
        if(memlen !=  impEncoder->getBufferSize()){
            shmctl(shm_id, IPC_RMID, NULL);
        }
    }



    shm_id = shmget( key1, impEncoder->getBufferSize(), IPC_CREAT);
    if(shm_id != -1 ){
        void* shared_mem;
        shared_mem = shmat( shm_id, NULL, 0);
        memcpy(shared_mem,impEncoder->getBuffer(),frameSize);
        shmdt(shared_mem);
    }else{

    }




    if (mode != IMP_MODE_JPEG) {
        frameSize = impEncoder->snap_h264();
    }
    memcpy(buffer, impEncoder->getBuffer(), frameSize);
    return frameSize;
}

int ImpCapture::getFd() {
    return 0;
}

unsigned long ImpCapture::getBufferSize() {
    return impEncoder->getBufferSize();
}



