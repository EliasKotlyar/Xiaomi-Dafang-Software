#include "sharedmem.h"

SharedMem::SharedMem() {

}

SharedMem::~SharedMem() {

}

void SharedMem::readMemory(char key, void *memory, int memorylenght) {
    void *shared_mem;
    key_t key1;
    key1 = ftok("/usr/include", key);
    int shm_id = shmget(key1, 0, 0);
    shared_mem = shmat(shm_id, NULL, 0);
    memcpy(memory, shared_mem, memorylenght);
    shmdt(shared_mem);
}

int SharedMem::getMemorySize(char key) {
    key_t key1;
    key1 = ftok("/usr/include", key);
    int shm_id = shmget(key1, 0, 0);
    struct shmid_ds buf;
    shmctl(shm_id, IPC_STAT, &buf);
    int memlen = buf.shm_segsz;
    return memlen;
}

void SharedMem::writeMemory(char key, void *memory, int memorylenght) {
    key_t key1;
    key1 = ftok("/usr/include", key);
    int shm_id;

    shm_id = shmget(key1, 0, 0);
    if (shm_id != -1) {
        int memlen = this->getMemorySize(key);
        if (memlen != memorylenght) {
            shmctl(shm_id, IPC_RMID, NULL);
        }
    }


    shm_id = shmget(key1, memorylenght, IPC_CREAT);
    if (shm_id != -1) {
        void *shared_mem;
        shared_mem = shmat(shm_id, NULL, 0);
        memcpy(shared_mem, memory, memorylenght);
        shmdt(shared_mem);
    } else {

    }


}

void SharedMem::getImage() {


}