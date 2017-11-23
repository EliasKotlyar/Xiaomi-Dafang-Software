#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include "ImpJpegVideoDeviceSource.h"
#include "sample-encoder-jpeg.h"
ImpJpegVideoDeviceSource* ImpJpegVideoDeviceSource::createNew(UsageEnvironment& env,unsigned timePerFrame)
{
    return new ImpJpegVideoDeviceSource(env,timePerFrame);
};


ImpJpegVideoDeviceSource::ImpJpegVideoDeviceSource(UsageEnvironment& env,unsigned timePerFrame) : JPEGVideoSource(env)
{
    imp_init();
};

ImpJpegVideoDeviceSource::~ImpJpegVideoDeviceSource(){
    imp_shutdown();
}

void ImpJpegVideoDeviceSource::doGetNextFrame(){

  //imp_get_h264_frame(fTo);
  imp_get_jpeg(fTo);
  // Switch to another task, and inform the reader that he has data:
  nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
			   (TaskFunc*)FramedSource::afterGetting, this);
}



u_int8_t ImpJpegVideoDeviceSource::type(){
    return 1;
}
u_int8_t ImpJpegVideoDeviceSource::qFactor(){
    // Quality:
    return 100;
}
u_int8_t ImpJpegVideoDeviceSource::width(){
 return 1280;
}
u_int8_t ImpJpegVideoDeviceSource::height(){
return 720;
}