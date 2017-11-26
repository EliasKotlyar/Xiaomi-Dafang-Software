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
ImpH264VideoDeviceSource::createNew(UsageEnvironment& env, char const* fileName,
                                unsigned preferredFrameSize,
                                unsigned playTimePerFrame) {
    FILE* fid = OpenInputFile(env, fileName);
    if (fid == NULL) return NULL;

    ImpH264VideoDeviceSource* newSource
            = new ImpH264VideoDeviceSource(env, fid, preferredFrameSize, playTimePerFrame);
    newSource->fFileSize = GetFileSize(fileName, fid);

    return newSource;
}

ImpH264VideoDeviceSource*
ImpH264VideoDeviceSource::createNew(UsageEnvironment& env, FILE* fid,
                                unsigned preferredFrameSize,
                                unsigned playTimePerFrame) {
    if (fid == NULL) return NULL;

    ImpH264VideoDeviceSource* newSource = new ImpH264VideoDeviceSource(env, fid, preferredFrameSize, playTimePerFrame);
    newSource->fFileSize = GetFileSize(NULL, fid);

    return newSource;
}

void ImpH264VideoDeviceSource::seekToByteAbsolute(u_int64_t byteNumber, u_int64_t numBytesToStream) {
    SeekFile64(fFid, (int64_t)byteNumber, SEEK_SET);

    fNumBytesToStream = numBytesToStream;
    fLimitNumBytesToStream = fNumBytesToStream > 0;
}

void ImpH264VideoDeviceSource::seekToByteRelative(int64_t offset, u_int64_t numBytesToStream) {
    SeekFile64(fFid, offset, SEEK_CUR);

    fNumBytesToStream = numBytesToStream;
    fLimitNumBytesToStream = fNumBytesToStream > 0;
}

void ImpH264VideoDeviceSource::seekToEnd() {
    SeekFile64(fFid, 0, SEEK_END);
}

ImpH264VideoDeviceSource::ImpH264VideoDeviceSource(UsageEnvironment& env, FILE* fid,
                                           unsigned preferredFrameSize,
                                           unsigned playTimePerFrame)
        : FramedFileSource(env, fid), fFileSize(0), fPreferredFrameSize(preferredFrameSize),
          fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(0),
          fHaveStartedReading(False), fLimitNumBytesToStream(False), fNumBytesToStream(0) {


    // Test whether the file is seekable
    fFidIsSeekable = FileIsSeekable(fFid);
}

ImpH264VideoDeviceSource::~ImpH264VideoDeviceSource() {
    if (fFid == NULL) return;


    CloseInputFile(fFid);
}

void ImpH264VideoDeviceSource::doGetNextFrame() {
    if (feof(fFid) || ferror(fFid) || (fLimitNumBytesToStream && fNumBytesToStream == 0)) {
        handleClosure();
        return;
    }


    doReadFromFile();

}

void ImpH264VideoDeviceSource::doStopGettingFrames() {
    envir().taskScheduler().unscheduleDelayedTask(nextTask());

}

void ImpH264VideoDeviceSource::fileReadableHandler(ImpH264VideoDeviceSource* source, int /*mask*/) {
    if (!source->isCurrentlyAwaitingData()) {
        source->doStopGettingFrames(); // we're not ready for the data yet
        return;
    }
    source->doReadFromFile();
}

void ImpH264VideoDeviceSource::doReadFromFile() {
    // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
    if (fLimitNumBytesToStream && fNumBytesToStream < (u_int64_t)fMaxSize) {
        fMaxSize = (unsigned)fNumBytesToStream;
    }
    if (fPreferredFrameSize > 0 && fPreferredFrameSize < fMaxSize) {
        fMaxSize = fPreferredFrameSize;
    }

    fFrameSize = fread(fTo, 1, fMaxSize, fFid);

    if (fFrameSize == 0) {
        handleClosure();
        return;
    }
    fNumBytesToStream -= fFrameSize;

    // Set the 'presentation time':
    if (fPlayTimePerFrame > 0 && fPreferredFrameSize > 0) {
        if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
            // This is the first frame, so use the current time:
            gettimeofday(&fPresentationTime, NULL);
        } else {
            // Increment by the play time of the previous data:
            unsigned uSeconds	= fPresentationTime.tv_usec + fLastPlayTime;
            fPresentationTime.tv_sec += uSeconds/1000000;
            fPresentationTime.tv_usec = uSeconds%1000000;
        }

        // Remember the play time of this data:
        fLastPlayTime = (fPlayTimePerFrame*fFrameSize)/fPreferredFrameSize;
        fDurationInMicroseconds = fLastPlayTime;
    } else {
        // We don't know a specific play time duration for this data,
        // so just record the current time as being the 'presentation time':
        gettimeofday(&fPresentationTime, NULL);
    }

    // Inform the reader that he has data:
    // To avoid possible infinite recursion, we need to return to the event loop to do this:
  nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
				(TaskFunc*)FramedSource::afterGetting, this);

}
