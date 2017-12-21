#include "ImpJpegDeviceSource.h"

ImpJpegDeviceSource *
ImpJpegDeviceSource::createNew(UsageEnvironment &env, DeviceInterface *device, int outputFd, unsigned int queueSize,
                               bool useThread) {
    return new ImpJpegDeviceSource(env, device, outputFd, queueSize, useThread);
}

ImpJpegDeviceSource::ImpJpegDeviceSource(UsageEnvironment &env, DeviceInterface *device, int outputFd,
                                         unsigned int queueSize, bool useThread)
        : V4L2DeviceSource(env, device, outputFd, queueSize, useThread) {
    impParams params;
    params.width = 320;
    params.height = 240;
    params.mode = IMP_MODE_JPEG;
    params.framerate = 25;
    params.nightvision = false;

    impEncoder = new ImpEncoder(params);
}

int ImpJpegDeviceSource::getNextFrame() {
    timeval ref;
    gettimeofday(&ref, NULL);
    //char buffer[m_device->getBufferSize()];
    int frameSize = impEncoder->snap_jpeg();

    //int frameSize = m_device->read(buffer,  m_device->getBufferSize());
    if (frameSize < 0) {
        LOG(NOTICE) << "V4L2DeviceSource::getNextFrame errno:" << errno << " " << strerror(errno);
    } else if (frameSize == 0) {
        LOG(NOTICE) << "V4L2DeviceSource::getNextFrame no data errno:" << errno << " " << strerror(errno);
    } else {
        timeval tv;
        gettimeofday(&tv, NULL);
        timeval diff;
        timersub(&tv, &ref, &diff);
        m_in.notify(tv.tv_sec, frameSize);
        LOG(DEBUG) << "getNextFrame\ttimestamp:" << ref.tv_sec << "." << ref.tv_usec << "\tsize:" << frameSize
                   << "\tdiff:" << (diff.tv_sec * 1000 + diff.tv_usec / 1000) << "ms";
        processFrame((char*)impEncoder->getBuffer(), frameSize, ref);
        if (m_outfd != -1) {
            write(m_outfd, impEncoder->getBuffer(), frameSize);
        }
    }
    return frameSize;
}


