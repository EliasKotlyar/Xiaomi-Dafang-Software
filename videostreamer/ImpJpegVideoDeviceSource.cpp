#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include "ImpJpegVideoDeviceSource.h"

ImpJpegVideoDeviceSource* ImpJpegVideoDeviceSource::createNew(UsageEnvironment& env,unsigned timePerFrame)
{
    return new ImpJpegVideoDeviceSource(env,timePerFrame);
};


ImpJpegVideoDeviceSource::ImpJpegVideoDeviceSource(UsageEnvironment& env,unsigned timePerFrame) : JPEGVideoSource(env)
{

};

void ImpJpegVideoDeviceSource::doGetNextFrame(){

}
u_int8_t ImpJpegVideoDeviceSource::type(){
    return 1;
}
u_int8_t ImpJpegVideoDeviceSource::qFactor(){
    // Quality:
    return 100;
}
u_int8_t ImpJpegVideoDeviceSource::width(){
 return 1980;
}
u_int8_t ImpJpegVideoDeviceSource::height(){
return 1280;
}