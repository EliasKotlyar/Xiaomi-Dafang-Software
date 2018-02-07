#include<stdio.h>
#include <getopt.h>
#include <cstdlib>
#include "sharedmem.h"


int main(int argc, char *argv[]) {

    char key = 0;
    int value = 0;

    // Parse Arguments:
    int c;
    while ((c = getopt(argc, argv, "k:v:")) != -1) {
        switch (c) {
            case 'k':
                key = optarg[0];
                break;
            case 'v':
                value = atoi(optarg);
                break;
            default:
                printf("Invalid Argument %c\n", c);
                exit(EXIT_FAILURE);
        }
    }

    SharedMem &mem = SharedMem::instance();
    shared_conf* conf = mem.getConfig();
    switch (key) {
        case 'f':
            conf->flip = value;
            break;
        case 'n':
            conf->nightmode = value;
            break;
        default:
            printf("Invalid Argument %c\n", c);
            exit(EXIT_FAILURE);
    }
    mem.setConfig();

    return 0;


}
