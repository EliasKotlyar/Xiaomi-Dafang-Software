/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2002 Live Networks, Inc.  All rights reserved.
// Elphel JPEG camera input device
// Implementation

#include "ElphelJPEGDeviceSource.hh"
#include <sys/ioctl.h>
#include <sys/time.h>

ElphelJPEGDeviceSource*
ElphelJPEGDeviceSource::createNew(UsageEnvironment& env,
				  unsigned timePerFrame) {
  FILE* fid = fopen("/dev/ccam_dma.raw", "rb");
  if (fid == NULL) {
    env.setResultErrMsg("Failed to open input device file");
    return NULL;
  }
  return new ElphelJPEGDeviceSource(env, fid, timePerFrame);
}

ElphelJPEGDeviceSource
::ElphelJPEGDeviceSource(UsageEnvironment& env, FILE* fid,
			 unsigned timePerFrame)
  : JPEGVideoSource(env),
    fFid(fid), fTimePerFrame(timePerFrame), fNeedAFrame(False) {

  // Ask to be notified when data becomes available on the camera's socket:


  // Start getting frames:
  startCapture();
}

ElphelJPEGDeviceSource::~ElphelJPEGDeviceSource() {
}

void ElphelJPEGDeviceSource::doGetNextFrame() {


}

void ElphelJPEGDeviceSource
::newFrameHandler(ElphelJPEGDeviceSource* source, int /*mask*/) {
  source->newFrameHandler1();
}

void ElphelJPEGDeviceSource::newFrameHandler1() {
  if (fNeedAFrame) deliverFrameToClient();
}

void ElphelJPEGDeviceSource::deliverFrameToClient() {
  fNeedAFrame = False;

  // Set the 'presentation time': the time that this frame was captured
  fPresentationTime = fLastCaptureTime;

  // Start capturing the next frame:
  startCapture();
  fread(fJPEGHeader, 1, JPEG_HEADER_SIZE, fFid);
  setParamsFromHeader();

  // Then, the JPEG payload:
  fFrameSize = fread(fTo, 1, fMaxSize, fFid);
  if (fFrameSize == fMaxSize) {
    fprintf(stderr, "ElphelJPEGDeviceSource::doGetNextFrame(): read maximum buffer size: %d bytes.  Frame may be truncated\n", fMaxSize);
  }

  clearerr(fFid); // clears EOF flag (for next time)

  // Switch to another task, and inform the reader that he has data:
  nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
			   (TaskFunc*)FramedSource::afterGetting, this);
}

u_int8_t ElphelJPEGDeviceSource::type() {
  return 1;
  }
u_int8_t ElphelJPEGDeviceSource::qFactor() {
  return fLastQFactor;
}
u_int8_t ElphelJPEGDeviceSource::width() {
  return fLastWidth;
}
u_int8_t ElphelJPEGDeviceSource::height() {
  return fLastHeight;
}

void ElphelJPEGDeviceSource::startCapture() {

  // Consider the capture as having occurred now:
  gettimeofday(&fLastCaptureTime, (struct timezone *)0);
}

void ElphelJPEGDeviceSource::setParamsFromHeader() {
  // Look for the "SOF0" marker (0xFF 0xC0), to get the frame
  // width and height:
  Boolean foundIt = False;
  for (int i = 0; i < JPEG_HEADER_SIZE-8; ++i) {
    if (fJPEGHeader[i] == 0xFF && fJPEGHeader[i+1] == 0xC0) {
      fLastHeight = (fJPEGHeader[i+5]<<5)|(fJPEGHeader[i+6]>>3);
      fLastWidth = (fJPEGHeader[i+7]<<5)|(fJPEGHeader[i+8]>>3);
      foundIt = True;
      break;
    }
  }
  if (!foundIt) fprintf(stderr, "ElphelJPEGDeviceSource: Failed to find SOF0 marker in header!\n");


}
