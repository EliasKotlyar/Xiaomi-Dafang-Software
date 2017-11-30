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
ImpH264VideoDeviceSource::createNew(UsageEnvironment &env) {
    ImpH264VideoDeviceSource *newSource
            = new ImpH264VideoDeviceSource(env);
    return newSource;
}

ImpH264VideoDeviceSource::ImpH264VideoDeviceSource(UsageEnvironment &env)
        : FramedSource(env) {
    impEncoder = new ImpEncoder(2);

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


    int bytesRead = impEncoder->snap_h264();

    if (bytesRead > (int) fMaxSize) {
        fprintf(stderr,
                "WebcamJPEGDeviceSource::doGetNextFrame(): read maximum buffer size: %d bytes.  Frame may be truncated\n",
                fMaxSize);
    }
    memcpy(fTo, impEncoder->getBuffer(), bytesRead);
    fFrameSize = bytesRead;

    gettimeofday(&fPresentationTime, NULL);


    // Inform the reader that he has data:
    // To avoid possible infinite recursion, we need to return to the event loop to do this:
    nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                                                             (TaskFunc *) FramedSource::afterGetting, this);

}
