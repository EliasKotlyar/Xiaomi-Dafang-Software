#ifndef _WEBCAM_JPEG_DEVICE_SOURCE_HH
#define _WEBCAM_JPEG_DEVICE_SOURCE_HH

class ImpJpegDeviceSource : public JPEGVideoSource {
public:

    static ImpJpegDeviceSource* createNew(UsageEnvironment& env, DeviceInterface * device, int outputFd, unsigned int queueSize, bool useThread) ;

};

#endif // _WEBCAM_JPEG_DEVICE_SOURCE_HH