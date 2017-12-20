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
#include "ImpEncoder.h"

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

ImpJpegVideoDeviceSource *
ImpJpegVideoDeviceSource::createNew(UsageEnvironment &env,
                                    impParams params) {
    //int fd = -1;
#ifndef JPEG_TEST

#endif
    try {
        return new ImpJpegVideoDeviceSource(env, params);
    } catch (DeviceException) {
        return NULL;
    }
}

#ifndef JPEG_TEST

int ImpJpegVideoDeviceSource::initDevice(impParams params) {
    impEncoder = new ImpEncoder(params);
    unsigned timePerFrame = 1000000 / params.framerate;
    fTimePerFrame = timePerFrame;
    return 0;
}

#endif // JPEG_TEST

ImpJpegVideoDeviceSource
::ImpJpegVideoDeviceSource(UsageEnvironment &env, impParams params)
        : JPEGVideoSource(env), fFd(0) {
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
    if (initDevice(params)) {
        throw DeviceException();
    }
#endif
}

ImpJpegVideoDeviceSource::~ImpJpegVideoDeviceSource() {
#ifdef JPEG_TEST
    delete [] jpeg_dat;
#else
    delete impEncoder;
#endif
}


static struct timezone Idunno;

void ImpJpegVideoDeviceSource::doGetNextFrame() {
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
    gettimeofday(&fLastCaptureTime, &Idunno);
    if (framecount == 0)
        starttime = fLastCaptureTime;
    framecount++;
    fPresentationTime = fLastCaptureTime;

    int bytesRead = impEncoder->snap_jpeg();

    if (bytesRead > (int) fMaxSize) {
        fprintf(stderr,
                "WebcamJPEGDeviceSource::doGetNextFrame(): read maximum buffer size: %d bytes.  Frame may be truncated\n",
                fMaxSize);
    }


    fFrameSize = jpeg_to_rtp(fTo, impEncoder->getBuffer(), bytesRead);

#endif // JPEG_TEST
    // Switch to another task, and inform the reader that he has data:
    nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                                                             (TaskFunc *) FramedSource::afterGetting, this);
}


size_t ImpJpegVideoDeviceSource::jpeg_to_rtp(void *pto, void *pfrom, size_t len) {
    unsigned char *to = (unsigned char *) pto, *from = (unsigned char *) pfrom;
    unsigned int datlen;
    unsigned char const *dat;
    if (parser.parse(from, len) == 0) { // successful parsing
        dat = parser.scandata(datlen);
        memcpy(to, dat, datlen);
        to += datlen;
        return datlen;
    }
    return 0;
}

u_int8_t const *ImpJpegVideoDeviceSource::quantizationTables(u_int8_t &precision, u_int16_t &length) {
    precision = parser.precision();
    return parser.quantizationTables(length);
}

u_int8_t ImpJpegVideoDeviceSource::type() {
    return parser.type();
}

u_int8_t ImpJpegVideoDeviceSource::qFactor() {
    return parser.qFactor();
}

u_int8_t ImpJpegVideoDeviceSource::width() {
    return parser.width();
}

u_int8_t ImpJpegVideoDeviceSource::height() {
    return parser.height();
}
