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

#include "WebcamJPEGDeviceSource.hh"
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#ifndef JPEG_TEST
#include <linux/videodev2.h>
#endif

#include "JpegFrameParser.hh"
#include <algorithm> 
#include <iostream>

#ifndef JPEG_TEST
static int xioctl(int fh, int request, void *arg);

static int xioctl(int fh, int request, void *arg)
{
    int r;
    
    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);
    
    return r;
}
#endif

WebcamJPEGDeviceSource*
WebcamJPEGDeviceSource::createNew(UsageEnvironment& env,
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
        return new WebcamJPEGDeviceSource(env, fd, timePerFrame);
    } catch (DeviceException) {
        return NULL;
    }
}

#ifndef JPEG_TEST
int WebcamJPEGDeviceSource::initDevice(UsageEnvironment& env, int fd)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    if(-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        env.setResultErrMsg("QueryCap failed");
        return -1;
    }
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        env.setResultErrMsg("No video capture device");
        return -1;
    }
    if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
        env.setResultErrMsg("Streaming not supported");
        return -1;
    }
    memset(&cropcap, 0, sizeof(cropcap));
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(0==xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;
        if(-1==xioctl(fd, VIDIOC_S_CROP, &crop)) {
            switch(errno) {
                case EINVAL:
                    env.setResultErrMsg("Crop not supported");
                    break;
                default:
                    break;
            }
        }
    }
    
    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc, 0, sizeof(fmtdesc));
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for(fmtdesc.index=0; ; fmtdesc.index++) {
        if(-1==xioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
            if(errno == EINVAL) {
                break;
            }
            continue;
        }
        if(fmtdesc.pixelformat == V4L2_PIX_FMT_MJPEG) {
            break;
        }
    }
    if(fmtdesc.pixelformat != V4L2_PIX_FMT_MJPEG) {
        env.setResultErrMsg("This webcam does not support MJPEG!");
        return -1;
    }
    struct v4l2_frmsizeenum frmsize;
    memset(&frmsize, 0, sizeof(frmsize));
    frmsize.pixel_format = V4L2_PIX_FMT_MJPEG;
    __u32 best_width=0, best_height=0, best_diff=0xffffffff;
    __u32 diff;
    for(frmsize.index = 0; xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0; frmsize.index++) {
        if(frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
            diff = abs(frmsize.discrete.width-640) + abs(frmsize.discrete.height-480);
            if(diff<best_diff) {
                best_diff = diff;
                best_width = frmsize.discrete.width;
                best_height = frmsize.discrete.height;
            }
        } else {
            if(frmsize.stepwise.min_width >= 640) {
                best_width = frmsize.stepwise.min_width;
            } else if(frmsize.stepwise.max_width <= 640) {
                best_width = frmsize.stepwise.max_width;
            } else {
                best_width = (640-frmsize.stepwise.min_width)/frmsize.stepwise.step_width*frmsize.stepwise.step_width + frmsize.stepwise.min_width;
            }
            if(frmsize.stepwise.min_height >= 480) {
                best_height = frmsize.stepwise.min_height;
            } else if(frmsize.stepwise.max_height <= 480) {
                best_height = frmsize.stepwise.max_height;
            } else {
                best_height = (480-frmsize.stepwise.min_height)/frmsize.stepwise.step_height*frmsize.stepwise.step_height + frmsize.stepwise.min_height;
            }
            best_diff = 0;
            break;
        }
    }
    if(best_diff == 0xffffffff) {
        env.setResultErrMsg("Failed to find an appropriate frame size!");
        return -1;
    }
    
    struct v4l2_frmivalenum frmival;
    memset(&frmival, 0, sizeof(frmival));
    frmival.index = 0;
    frmival.pixel_format = V4L2_PIX_FMT_MJPEG;
    frmival.width = best_width;
    frmival.height = best_height;
    __u32 best_ival_num=0, best_ival_den=0;
    float best_ival_diff = 1e6, ival_diff;
    for(frmival.index = 0; xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival)>=0; frmival.index++) {
        if(frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
            ival_diff = abs((float)frmival.discrete.numerator/frmival.discrete.denominator - 0.1);
            if(ival_diff<best_ival_diff) {
                best_ival_diff = ival_diff;
                best_ival_num = frmival.discrete.numerator;
                best_ival_den = frmival.discrete.denominator;
            }
        } else {
            break;
        }
    }
    if(best_ival_diff > 1e5) {
        env.setResultErrMsg("Failed to find an appropriate frame rate!");
        return -1;
    }
    
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = best_width;
    fmt.fmt.pix.height = best_height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if (-1==xioctl(fd, VIDIOC_S_FMT, &fmt)) {
        env.setResultErrMsg("Set format MJPEG failed");
        return -1;
    }
    
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if(-1==xioctl(fd, VIDIOC_REQBUFS, &req)) {
        env.setResultErrMsg("ReqBuf failed");
        return -1;
    }
    if(req.count < 2) {
        env.setResultErrMsg("buffer count <2");
        return -1;
    }
    fBuffers = (struct buffer *)calloc(req.count, sizeof(*fBuffers));
    if(!fBuffers) {
        env.setResultErrMsg("Out of memory");
        return -1;
    }
    for(fNbuffers = 0; fNbuffers < req.count; fNbuffers++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = fNbuffers;
        if(-1==xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
            env.setResultErrMsg("QueryBuf failed");
            return -1;
        }
        fBuffers[fNbuffers].length = buf.length;
        fBuffers[fNbuffers].start = mmap(NULL, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if(MAP_FAILED == fBuffers[fNbuffers].start) {
            env.setResultErrMsg("mmap failed");
            return -1;
        }
    }
    
    for(int i=0;i<fNbuffers;i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if(-1==xioctl(fd,VIDIOC_QBUF, &buf))
        {
            env.setResultErrMsg("QBuf failed");
            return -1;
        }
    }
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1==xioctl(fd,VIDIOC_STREAMON, &type)) {
        env.setResultErrMsg("StreamOn failed");
        return -1;
    }
    return 0;
}
#endif // JPEG_TEST

WebcamJPEGDeviceSource
::WebcamJPEGDeviceSource(UsageEnvironment& env, int fd, unsigned timePerFrame)
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

WebcamJPEGDeviceSource::~WebcamJPEGDeviceSource()
{
#ifdef JPEG_TEST
    delete [] jpeg_dat;
#else
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1==xioctl(fFd, VIDIOC_STREAMOFF, &type)) {
        
    }
    for(int i=0; i< fNbuffers; i++) {
        if(-1==munmap(fBuffers[i].start, fBuffers[i].length)) {
            
        }
    }
    ::close(fFd);
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

void WebcamJPEGDeviceSource::doGetNextFrame()
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
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if(-1==xioctl(fFd, VIDIOC_DQBUF, &buf)) { // this will block if no frames are available
        
    }
    gettimeofday(&fLastCaptureTime, &Idunno);
    if(framecount==0)
        starttime = fLastCaptureTime;
    framecount++;
    fPresentationTime = fLastCaptureTime;
    /*
    if(framecount % 30 == 0)
        printf("frame rate=%f\n", (float)framecount/timeval_diff(&fLastCaptureTime, &starttime));
     */
    if(buf.bytesused > fMaxSize) {
        fprintf(stderr, "WebcamJPEGDeviceSource::doGetNextFrame(): read maximum buffer size: %d bytes.  Frame may be truncated\n", fMaxSize);
    }
    fFrameSize = jpeg_to_rtp(fTo, fBuffers[buf.index].start, std::min(buf.bytesused, fMaxSize));
    if(-1==xioctl(fFd, VIDIOC_QBUF, &buf)) {
        
    }
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

size_t WebcamJPEGDeviceSource::jpeg_to_rtp(void *pto, void *pfrom, size_t len)
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

u_int8_t const * WebcamJPEGDeviceSource::quantizationTables(u_int8_t & precision, u_int16_t & length)
{
    precision = parser.precision();
    return parser.quantizationTables(length);
}

u_int8_t WebcamJPEGDeviceSource::type()
{
    return parser.type();
}

u_int8_t WebcamJPEGDeviceSource::qFactor()
{
    return parser.qFactor();
}

u_int8_t WebcamJPEGDeviceSource::width()
{
    return parser.width();
}

u_int8_t WebcamJPEGDeviceSource::height()
{
    return parser.height();
}
