#include<stdio.h>
#include <cstdlib>


#include "sharedmem.h"

int main(int argc, char *argv[]) {

    SharedMem& mem = SharedMem::instance();


    fwrite(memory, memlen, 1, stdout);


    return 0;
}
