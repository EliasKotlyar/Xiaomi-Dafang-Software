#ifndef PLAY
#define PLAY



void open_device (int * dspFd, int inSampleRate);
void close_device (int * dspFd);

void play (int * dsp, int len, unsigned char* buff);

#endif