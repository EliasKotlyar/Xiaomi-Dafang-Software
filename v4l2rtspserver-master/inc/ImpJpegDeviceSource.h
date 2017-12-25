#ifndef _WEBCAM_JPEG_DEVICE_SOURCE_HH
#define _WEBCAM_JPEG_DEVICE_SOURCE_HH

#include "DeviceSource.h"
#include "ImpEncoder.h"
#include "logger.h"

class ImpJpegDeviceSource : public V4L2DeviceSource {
public:

    static ImpJpegDeviceSource *
    createNew(UsageEnvironment &env, DeviceInterface *device, int outputFd, unsigned int queueSize, bool useThread);

    ImpJpegDeviceSource(UsageEnvironment &env, DeviceInterface *device, int outputFd, unsigned int queueSize,
                        bool useThread);

    int getNextFrame();

    ImpEncoder *impEncoder;

    int getWidth();

    int getHeight();

    size_t read(char *buffer, size_t bufferSize);

    int getFd();

    unsigned long getBufferSize();


};

#endif // _WEBCAM_JPEG_DEVICE_SOURCE_HH