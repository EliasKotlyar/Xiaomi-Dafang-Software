/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2017 Live Networks, Inc.  All rights reserved.
// A file source that is a plain byte stream (rather than frames)
// Implementation

#include "ImpH264VideoDeviceSource.h"
#include "InputFile.hh"
#include "GroupsockHelper.hh"

////////// ImpH264VideoDeviceSource //////////

ImpH264VideoDeviceSource *
ImpH264VideoDeviceSource::createNew(UsageEnvironment &env, impParams params) {
    ImpH264VideoDeviceSource *newSource
            = new ImpH264VideoDeviceSource(env,params);
    return newSource;
}

ImpH264VideoDeviceSource::ImpH264VideoDeviceSource(UsageEnvironment &env, impParams params)
        : FramedSource(env) {
    impEncoder = new ImpEncoder(IMP_MODE_H264,params.width,params.height);
    framesCount=0;

}

ImpH264VideoDeviceSource::~ImpH264VideoDeviceSource() {
    delete impEncoder;
}

void ImpH264VideoDeviceSource::doGetNextFrame() {


    doReadFromFile();

}

void ImpH264VideoDeviceSource::doStopGettingFrames() {
    envir().taskScheduler().unscheduleDelayedTask(nextTask());

}

void ImpH264VideoDeviceSource::doReadFromFile() {


    if(frameList.empty()){
        framesCount++;
        if(framesCount == 50){
            framesCount = 0;
            impEncoder->requestIDR();
        }
        frameList = impEncoder->geth264frames();
    }
    IMPEncoderPack frame = frameList.front();
    void* frameAdr = (void *) ((int)frame.virAddr +4) ;
    int frameSize = frame.length-4;

    if (frameSize > (int) fMaxSize) {
        /*fprintf(stderr,
                "WebcamJPEGDeviceSource::doGetNextFrame(): read maximum buffer size: %d bytes.  Frame may be truncated\n",
                fMaxSize);
                */

        fNumTruncatedBytes = frameSize - fMaxSize;
        frameSize = fMaxSize;
    }

    memcpy(fTo, frameAdr, frameSize);
    fFrameSize = frameSize;
    gettimeofday(&fPresentationTime, NULL);
    //fPresentationTime =
    //memcpy(&fPresentationTime,&frame.timestamp,sizeof(timeval));
    //fPresentationTime.tv_usec = frame.timestamp;
    frameList.pop_front();
    //printf("Got Frame with size %d & with the type of %d, seconds: %d, miliseconds %d \n",frameSize,frame.dataType.h264Type,(int)fPresentationTime.tv_sec,(int)fPresentationTime.tv_usec);


    FramedSource::afterGetting(this);
}
