/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** MJPEGVideoSource.h
** 
** MJPEG Live555 source 
**
** -------------------------------------------------------------------------*/
#ifndef MJPEG_VIDEO_SOURCE
#define MJPEG_VIDEO_SOURCE

#include <string>
#include <list> 
#include <iostream>
#include <iomanip>

#include "JPEGVideoSource.hh"
#include "snx_lib.h"

class MJPEGVideoSource : public JPEGVideoSource
{

        public:
                static MJPEGVideoSource* createNew (UsageEnvironment& env, FramedSource* source,V4L2DeviceParameters params)
                {
                        return new MJPEGVideoSource(env,source, params);
                }
                virtual void doGetNextFrame()
                {
                    //printf("doGetNextFrame\n");

                    //Set to link the current variables, ( fTo, fMaxSize, This ) 
                    //So the Parent FramedSource (V4L2DeviceSource) will deliver frames to here.
                    //Please refer to FramedSource::getNextFrame 
                    if (m_inputSource)
                        m_inputSource->getNextFrame(fTo, fMaxSize, afterGettingFrameSub, this, FramedSource::handleClosure, this);   
                
                }
                virtual void doStopGettingFrames()
                {
                    FramedSource::doStopGettingFrames();
                    if (m_inputSource)
                        m_inputSource->stopGettingFrames();               
                }
                static void afterGettingFrameSub(void* clientData, unsigned frameSize,unsigned numTruncatedBytes,struct timeval presentationTime,unsigned durationInMicroseconds) 
                {
                                MJPEGVideoSource* source = (MJPEGVideoSource*)clientData;
                                source->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
                }        
                void afterGettingFrame(unsigned frameSize,unsigned numTruncatedBytes,struct timeval presentationTime,unsigned durationInMicroseconds)
                {
#if 0
                    int headerSize = 0;
                    bool headerOk = false;
                    fFrameSize = 0;

                    for (unsigned int i = 0; i < frameSize ; ++i)
                    {
                        // SOF
                        if ( (i+8) < frameSize  && fTo[i] == 0xFF && fTo[i+1] == 0xC0 )
                        {
                             m_height = (fTo[i+5]<<5)|(fTo[i+6]>>3);
                             m_width = (fTo[i+7]<<5)|(fTo[i+8]>>3);
                        }
                        // DQT
                        if ( (i+5+64) < frameSize && fTo[i] == 0xFF && fTo[i+1] == 0xDB)
                        {
                            if (fTo[i+4] ==0)
                            {
                                memcpy(m_qTable, fTo + i + 5, 64);
                                m_qTable0Init = true;
                            }
                            else if (fTo[i+4] ==1)
                            {
                                memcpy(m_qTable + 64, fTo + i + 5, 64);
                                m_qTable1Init = true;
                            }
                        }
                        // End of header
                        if ( (i+1) < frameSize && fTo[i] == 0x3F && fTo[i+1] == 0x00 )
                        {
                             headerOk = true;
                             headerSize = i+2;
                             break;
                        }
                    }

                    if (headerOk)
                    {
                        fFrameSize = frameSize - headerSize;
                        memmove( fTo, fTo + headerSize, fFrameSize );
                    }
#endif
                    fFrameSize = frameSize;
                    fNumTruncatedBytes = numTruncatedBytes;
                    fPresentationTime = presentationTime;
                    fDurationInMicroseconds = durationInMicroseconds;
                    afterGetting(this);
    
                }

                /*

                    Fixed JPEG header of SONiX SN986 Series MJPEG encoder.
                */
                
                virtual u_int8_t type() { 
                        //printf("type\n");
                        return 1; 
                };
                virtual u_int8_t qFactor() { 
                        //printf("qfactor\n");
                        return m_qFactor; 
                };
                virtual u_int8_t width() { 
                        //printf("width\n");
                        return m_width; 
                };
                virtual u_int8_t height() { 
                        //printf("height\n");
                        return m_height; 
                };
                u_int8_t const* quantizationTables( u_int8_t& precision, u_int16_t& length )
                {
                    //printf("quantizationTables\n");
                    length = 0;
                    precision = 0;

                    if ( m_qTable0Init && m_qTable1Init )
                    {
                            precision = 8;
                            length = sizeof(m_qTable);
                    }
                    return m_qTable;            
                }

                FramedSource * m_inputSource;

        protected:
                MJPEGVideoSource(UsageEnvironment& env, FramedSource* source, V4L2DeviceParameters params) : JPEGVideoSource(env),
                m_inputSource( source),
                m_width(params.m_width >> 3),
                m_height(params.m_height >> 3),
                m_qFactor(255),
                m_qTable0Init(true),
                m_qTable1Init(false)
                {

                    u_int8_t      qTable[64] ={
                        0x0b, 0x07, 0x07, 0x0b, 0x07, 0x07, 0x0b, 0x0b, 
                        0x0b, 0x0b, 0x0e, 0x0b, 0x0b, 0x0e, 0x12, 0x1d,
                        0x12, 0x12, 0x0e, 0x0e, 0x12, 0x24, 0x19, 0x19, 
                        0x15, 0x1d, 0x2b, 0x24, 0x2b, 0x2b, 0x27, 0x24, 
                        0x27, 0x27, 0x2f, 0x32, 0x40, 0x39, 0x2f, 0x32, 
                        0x3d, 0x32, 0x27, 0x27, 0x39, 0x4f, 0x39, 0x3d, 
                        0x44, 0x48, 0x4b, 0x4b, 0x4b, 0x2b, 0x36, 0x52, 
                        0x56, 0x4f, 0x48, 0x56, 0x40, 0x48, 0x4b, 0x48
                    };
                    memcpy(m_qTable,qTable,sizeof(m_qTable));

                }
                virtual ~MJPEGVideoSource() 
                { 
                    printf("~MJPEGVideoSource\n");

                    Medium::close(m_inputSource); 
                }

        protected:

                u_int8_t      m_width;
                u_int8_t      m_height;
                u_int8_t      m_qFactor;
                u_int8_t      m_qTable[64];
                bool          m_qTable0Init;
                bool          m_qTable1Init;
          
};

#endif