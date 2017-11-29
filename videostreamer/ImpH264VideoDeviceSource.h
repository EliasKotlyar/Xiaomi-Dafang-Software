//
// Created by eko on 26.11.17.
//

#ifndef DROPBEAR_DAFANG_IMPH264VIDEODEVICESOURCE_CPP_H
#define DROPBEAR_DAFANG_IMPH264VIDEODEVICESOURCE_CPP_H


#define IMP_BUFFER_SIZE 200000


#ifndef _FRAMED_FILE_SOURCE_HH
#include "FramedFileSource.hh"
#endif

#include "ImpEncoder.h"

class ImpH264VideoDeviceSource: public FramedSource {
public:
    static ImpH264VideoDeviceSource* createNew(UsageEnvironment& env);

protected:
    ImpH264VideoDeviceSource(UsageEnvironment& env);
    // called only by createNew()

    virtual ~ImpH264VideoDeviceSource();

    static void fileReadableHandler(ImpH264VideoDeviceSource* source, int mask);
    void doReadFromFile();

private:
    // redefined virtual functions:
    virtual void doGetNextFrame();
    virtual void doStopGettingFrames();


private:
    unsigned fPreferredFrameSize;
    unsigned fPlayTimePerFrame;
    Boolean fFidIsSeekable;
    unsigned fLastPlayTime;
    Boolean fHaveStartedReading;
    Boolean fLimitNumBytesToStream;
    u_int64_t fNumBytesToStream; // used iff "fLimitNumBytesToStream" is True
    ImpEncoder* impEncoder;
};

#endif //DROPBEAR_DAFANG_IMPH264VIDEODEVICESOURCE_CPP_H
