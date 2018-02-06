#ifndef SHAREDMEM_H
#define SHAREDMEM_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

struct shared_conf {
    int nightmode;
    int flip;
};


class SharedMem {
public:
    SharedMem();
    void getImage();

    static SharedMem& instance()
    {
        static SharedMem _instance;
        return _instance;
    }


    void readMemory(char key, void *memory, int memorylenght);

    int getMemorySize(char key);

    void writeMemory(char key, void *memory, int memorylenght);

    ~SharedMem();


private:



};


#endif