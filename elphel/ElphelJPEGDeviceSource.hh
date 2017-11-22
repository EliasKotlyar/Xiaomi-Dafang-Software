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
// C++ header

#ifndef _ELPHEL_JPEG_DEVICE_SOURCE_HH
#define _ELPHEL_JPEG_DEVICE_SOURCE_HH

#ifndef _JPEG_VIDEO_SOURCE_HH
#include "JPEGVideoSource.hh"
#endif

#define JPEG_HEADER_SIZE 623

class ElphelJPEGDeviceSource: public JPEGVideoSource {
public:
  static ElphelJPEGDeviceSource* createNew(UsageEnvironment& env,
					   unsigned timePerFrame);
  // "timePerFrame" is in microseconds

protected:
  ElphelJPEGDeviceSource(UsageEnvironment& env,
			 FILE* fid, unsigned timePerFrame);
  // called only by createNew()
  virtual ~ElphelJPEGDeviceSource();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual u_int8_t type();
  virtual u_int8_t qFactor();
  virtual u_int8_t width();
  virtual u_int8_t height();

private:
  static void newFrameHandler(ElphelJPEGDeviceSource* source, int mask);
  void newFrameHandler1();
  void deliverFrameToClient();
  void startCapture();
  void setParamsFromHeader();

private:
  FILE* fFid;
  unsigned fTimePerFrame;
  struct timeval fLastCaptureTime;
  u_int8_t fLastQFactor, fLastWidth, fLastHeight;
  Boolean fNeedAFrame;
  unsigned char fJPEGHeader[JPEG_HEADER_SIZE];
};

#endif
