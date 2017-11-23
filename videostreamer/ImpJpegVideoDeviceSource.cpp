/*
 Copyright (C) 2015, Kyle Zhou <kyle.zhou at live.com>
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
// Webcam MJPEG camera input device
// Implementation

#include "ImpJpegVideoDeviceSource.h"
#include "sample-encoder-jpeg.h"
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#ifndef JPEG_TEST
#endif

#include "JpegFrameParser.hh"
#include <algorithm> 
#include <iostream>

ImpJpegVideoDeviceSource*
ImpJpegVideoDeviceSource::createNew(UsageEnvironment& env,
				  unsigned timePerFrame) {
    int fd = -1;
#ifndef JPEG_TEST
    fd = open("/dev/video0", O_RDWR, 0); // TODO: use argv instead of hardcoded dev
    if (fd == -1) {
        env.setResultErrMsg("Failed to open input device file");
        return NULL;
    }
#endif
    try {
        return new ImpJpegVideoDeviceSource(env, fd, timePerFrame);
    } catch (DeviceException) {
        return NULL;
    }
}

#ifndef JPEG_TEST
int ImpJpegVideoDeviceSource::initDevice(UsageEnvironment& env, int fd)
{
    imp_init(1);
    return 0;
}
#endif // JPEG_TEST

ImpJpegVideoDeviceSource
::ImpJpegVideoDeviceSource(UsageEnvironment& env, int fd, unsigned timePerFrame)
  : JPEGVideoSource(env), fFd(fd), fTimePerFrame(timePerFrame)
{
#ifdef JPEG_TEST
    jpeg_dat = new unsigned char [MAX_JPEG_FILE_SZ];
    FILE *fp = fopen("test.jpg", "rb");
    if(fp==NULL) {
        env.setResultErrMsg("could not open test.jpg.\n");
        throw DeviceException();
    }
    jpeg_datlen = fread(jpeg_dat, 1, MAX_JPEG_FILE_SZ, fp);
    fclose(fp);
#else
    if(initDevice(env, fd)) {
        throw DeviceException();
    }
#endif
}

ImpJpegVideoDeviceSource::~ImpJpegVideoDeviceSource()
{
#ifdef JPEG_TEST
    delete [] jpeg_dat;
#else
    imp_shutdown();
#endif
}

static int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
    if(x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if(x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;
    return x->tv_sec < y->tv_sec;
}

static float timeval_diff(struct timeval *x, struct timeval *y)
{
    struct timeval result;
    timeval_subtract(&result, x, y);
    return result.tv_sec + result.tv_usec/1000000.0;
}

static struct timezone Idunno;

void ImpJpegVideoDeviceSource::doGetNextFrame()
{
    static unsigned long framecount = 0;
    static struct timeval starttime;
    
#ifdef JPEG_TEST
    fFrameSize = jpeg_to_rtp(fTo, jpeg_dat, jpeg_datlen);
    gettimeofday(&fLastCaptureTime, &Idunno);
    if(framecount==0)
        starttime = fLastCaptureTime;
    framecount++;
    fPresentationTime = fLastCaptureTime;
    fDurationInMicroseconds = fTimePerFrame;
#else


    //fFrameSize = jpeg_to_rtp(fTo, fBuffers[buf.index].start, std::min(buf.bytesused, fMaxSize));

#endif // JPEG_TEST
    // Switch to another task, and inform the reader that he has data:
    nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                    (TaskFunc*)FramedSource::afterGetting, this);
}

static unsigned char calcQ(unsigned char const *qt);

static unsigned char calcQ(unsigned char const *qt)
{
    unsigned int q;
    q = (qt[0]*100-50)/16;
    //q = (qt[64]*100-50)/17;
    if(q>5000)
        q = 5000;
    if(q<2)
        q = 2;
    if(q>100)
        q = 5000/q;
    else
        q = (200-q)/2;
    return (unsigned char) q;
}

size_t ImpJpegVideoDeviceSource::jpeg_to_rtp(void *pto, void *pfrom, size_t len)
{
    unsigned char *to=(unsigned char*)pto, *from=(unsigned char*)pfrom;
    unsigned int datlen;
    unsigned char const * dat;
    if(parser.parse(from, len) == 0) { // successful parsing
        dat = parser.scandata(datlen);
        memcpy(to, dat, datlen);
        to += datlen;
        return datlen;
    }
    return 0;
}

u_int8_t const * ImpJpegVideoDeviceSource::quantizationTables(u_int8_t & precision, u_int16_t & length)
{
    precision = parser.precision();
    return parser.quantizationTables(length);
}

u_int8_t ImpJpegVideoDeviceSource::type()
{
    return parser.type();
}

u_int8_t ImpJpegVideoDeviceSource::qFactor()
{
    return parser.qFactor();
}

u_int8_t ImpJpegVideoDeviceSource::width()
{
    return parser.width();
}

u_int8_t ImpJpegVideoDeviceSource::height()
{
    return parser.height();
}
