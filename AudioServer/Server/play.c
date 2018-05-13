#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <soundcard.h>
#include <string.h>


typedef struct {
     unsigned char *buf;
     unsigned int head;
     unsigned int tail;
     unsigned int size;
     unsigned int capacity;
} fifo_t;

// Sizeof buffer that will be written in device
#define SIZEWRITE 1024
static fifo_t _Fifo;
static unsigned char _Buffer[SIZEWRITE*4];


//This initializes the FIFO structure with the given buffer and size
void fifo_init(fifo_t * f, unsigned char * buf, unsigned int capacity){
     f->head = 0;
     f->tail = 0;
     f->capacity = capacity;
     f->buf = buf;
     f->size=0;
}

//This reads nbytes bytes from the FIFO
//The number of bytes read is returned
static unsigned  fifo_read(fifo_t * f, void * buf, unsigned int nbytes){
     unsigned int i;
     char *p = buf;

     for(i=0; i < nbytes; i++)
     {
        if( f->tail != f->head )
        { //see if any data is available
            f->size--;
            *p++ = f->buf[f->tail];  //grab a byte from the buffer
            f->tail++;  //increment the tail
            if( f->tail == f->capacity ){  //check for wrap-around
                f->tail = 0;
            }
          } else {
               return i; //number of bytes read
          }
     }
     return nbytes;
}

//This writes up to nbytes bytes to the FIFO
//If the head runs in to the tail, not all bytes are written
//The number of bytes written is returned
static unsigned int fifo_write(fifo_t * f, void * buf, unsigned int nbytes){
     unsigned int i;
     const char * p = buf;
     for(i=0; i < nbytes; i++)
     {
        //first check to see if there is space in the buffer
        if( (f->head + 1 == f->tail) ||
            ( (f->head + 1 == f->capacity) && (f->tail == 0) ))
        {
            return i; //no more room
        } else {
            f->size++;
            f->buf[f->head] = *p++;
            f->head++;  //increment the head
            if( f->head == f->capacity ){  //check for wrap-around
                f->head = 0;
            }
        }
     }
     return nbytes;
}


void open_device (int * fd, int inSampleRate)
{

  *fd = open("/dev/dsp", O_WRONLY| O_EXCL);
  if (*fd < 0) {
    perror("open of /dev/dsp failed");
    return;
  }

  int tmp = 0;

  ioctl (*fd, SNDCTL_DSP_RESET, NULL);


  //ioctl (*fd, SNDCTL_DSP_COOKEDMODE, &tmp);

  tmp = APF_NORMAL;
  ioctl (*fd, SNDCTL_DSP_PROFILE, &tmp);


  int format= AFMT_S16_LE;
  if (ioctl(*fd, SNDCTL_DSP_SETFMT, &format)==-1)
  { // Fatal error /
        perror("Cant set format...");
  }

  int stereo = 1;
  if (ioctl(*fd, SNDCTL_DSP_CHANNELS, &stereo)==-1)
  { /* Fatal error */
       perror("Cant set Mono/Stereo ..." );
  }

  int speed =  inSampleRate;

  if (ioctl(*fd, SNDCTL_DSP_SPEED, &speed)==-1)
  { /* Fatal error */
        perror("Cant set Speed ...");
  }
  fifo_init(&_Fifo, _Buffer, sizeof(_Buffer));
}


void close_device (int * dspFd)
{
    ioctl (*dspFd, SNDCTL_DSP_RESET, NULL);
    ioctl (*dspFd, SNDCTL_DSP_HALT_OUTPUT, NULL);
    close(*dspFd);
    *dspFd = -1;
}


void play (int * dsp, int len, unsigned char* buf)
{
    int status;
    unsigned char tmpBuf[SIZEWRITE];

    int s=fifo_write(&_Fifo, buf, len);
    while (1)
    {
        if (_Fifo.size >= SIZEWRITE)
        {
            s=fifo_read(&_Fifo, tmpBuf, SIZEWRITE);

            status = write(*dsp, tmpBuf, SIZEWRITE);
            if (status != SIZEWRITE)
                    perror("wrote wrong number of bytes");

        } else {
            break;
        }

    }

    return 0;
}

