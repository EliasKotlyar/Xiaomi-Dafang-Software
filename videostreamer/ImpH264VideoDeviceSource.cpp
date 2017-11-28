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

ImpH264VideoDeviceSource*
ImpH264VideoDeviceSource::createNew(UsageEnvironment& env) {
    ImpH264VideoDeviceSource* newSource
            = new ImpH264VideoDeviceSource(env);
    return newSource;
}

ImpH264VideoDeviceSource::ImpH264VideoDeviceSource(UsageEnvironment& env)
        : FramedSource(env) {

}

ImpH264VideoDeviceSource::~ImpH264VideoDeviceSource() {

}

void ImpH264VideoDeviceSource::doGetNextFrame() {


    doReadFromFile();

}



void ImpH264VideoDeviceSource::doReadFromFile() {
    // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
    if (fLimitNumBytesToStream && fNumBytesToStream < (u_int64_t)fMaxSize) {
        fMaxSize = (unsigned)fNumBytesToStream;
    }
    if (fPreferredFrameSize > 0 && fPreferredFrameSize < fMaxSize) {
        fMaxSize = fPreferredFrameSize;
    }

    //fFrameSize = fread(fTo, 1, fMaxSize, fFid);

    if (fFrameSize == 0) {
        handleClosure();
        return;
    }
    fNumBytesToStream -= fFrameSize;


   gettimeofday(&fPresentationTime, NULL);


    // Inform the reader that he has data:
    // To avoid possible infinite recursion, we need to return to the event loop to do this:
  nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
				(TaskFunc*)FramedSource::afterGetting, this);

}
