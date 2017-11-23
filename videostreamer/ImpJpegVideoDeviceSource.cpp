#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include "ImpJpegVideoDeviceSource.h"

ImpJpegVideoDeviceSource* ImpJpegVideoDeviceSource::createNew(UsageEnvironment& env,unsigned timePerFrame)
{
    return new ImpJpegVideoDeviceSource(env,timePerFrame);
};