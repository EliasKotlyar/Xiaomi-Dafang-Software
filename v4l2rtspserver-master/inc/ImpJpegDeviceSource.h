#ifndef _WEBCAM_JPEG_DEVICE_SOURCE_HH
#define _WEBCAM_JPEG_DEVICE_SOURCE_HH

#include "DeviceSource.h"

class ImpJpegDeviceSource : public V4L2DeviceSource {
public:

    static ImpJpegDeviceSource* createNew(UsageEnvironment& env, DeviceInterface * device, int outputFd, unsigned int queueSize, bool useThread) ;

};

#endif // _WEBCAM_JPEG_DEVICE_SOURCE_HH