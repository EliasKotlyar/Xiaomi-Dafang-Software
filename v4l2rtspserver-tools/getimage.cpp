#include<stdio.h>




#include "sharedmem.h"

int main(int argc, char *argv[]) {

    SharedMem& mem = SharedMem::instance();

    return 0;
}
