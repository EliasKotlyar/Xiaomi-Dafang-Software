#include <iostream>
#include "ImpEncoder.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_usage() {
    printf("Usage: jpegSnap -w 1920 -h 1080\n");
}

int main(int argc, char *argv[]) {

    int width = 1920;
    int height = 1080;

    // Parse Arguments:
    int c;
      while ((c = getopt (argc, argv, "w:h:")) != -1){
        switch (c)
                {
                case 'w':
                  width = atoi(optarg);
                  break;
                case 'h':
                  height = atoi(optarg);
                  break;
                default:
                  printf("Invalid Argument %c\n",c);
                  exit(EXIT_FAILURE);
            }
      }





    ImpEncoder* impEncoder = new ImpEncoder(IMP_MODE_JPEG,width,height);
    int bytesRead = impEncoder->snap_jpeg();
    void* buffer = impEncoder->getBuffer();

    int ret;
    //int stream_fd = open("", O_RDWR);
    ret = fwrite(buffer, bytesRead,1,stdout);

    return ret;
}
