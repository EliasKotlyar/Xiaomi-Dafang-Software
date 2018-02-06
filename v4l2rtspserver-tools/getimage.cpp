#include<stdio.h>
#include <cstdlib>


#include "sharedmem.h"

int main(int argc, char *argv[]) {

    SharedMem& mem = SharedMem::instance();

    int memlen = mem.getMemorySize('x');
    void* memory = malloc(memlen);
    mem.readMemory('x',memory,memlen);
    fwrite(memory, memlen, 1, stdout);


    return 0;
}
