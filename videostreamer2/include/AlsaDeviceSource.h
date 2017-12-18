/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AlsaDeviceSource.h
** 
** V4L2 live555 source 
**
** -------------------------------------------------------------------------*/


#ifndef ALSA_DEVICE_SOURCE
#define ALSA_DEVICE_SOURCE

#include <string>
#include <list> 
#include <iostream>
#include <iomanip>

// live555
#include <liveMedia.hh>

// project
#include "snx_lib.h"
//#include "V4l2Capture.h"
//#include "sn98600_v4l2.h"

// ---------------------------------
// V4L2 FramedSource
// ---------------------------------
class AlsaDeviceSource: public FramedSource
{
	public:
		// ---------------------------------
		// Captured frame
		// ---------------------------------
		struct Frame
		{
			Frame(char* buffer, int size, timeval timestamp) : m_buffer(buffer), m_size(size), m_timestamp(timestamp) {};
			Frame(const Frame&);
			Frame& operator=(const Frame&);
			~Frame()  { delete m_buffer; };
			
			char* m_buffer;
			int m_size;
			timeval m_timestamp;
		};
		
		// ---------------------------------
		// Compute simple stats
		// ---------------------------------
		class Stats
		{
			public:
				Stats(const std::string & msg) : m_fps(0), m_fps_sec(0), m_size(0), m_msg(msg) {};
				
			public:
				int notify(int tv_sec, int framesize);
			
			protected:
				int m_fps;
				int m_fps_sec;
				int m_size;
				const std::string m_msg;
		};
		
	public:
		static AlsaDeviceSource* createNew(UsageEnvironment& env, int outputFd, unsigned int queueSize, bool useThread) ;
		std::string getAuxLine() { return m_auxLine; };	
		void audiocallback(const struct timeval *tv, void *data, size_t len, void *cbarg);

	protected:
		AlsaDeviceSource(UsageEnvironment& env, int outputFd, unsigned int queueSize, bool useThread);
		virtual ~AlsaDeviceSource();

	protected:	

		static void* threadStub(void* clientData) { return ((AlsaDeviceSource*) clientData)->thread();};
		void* thread();
		static void deliverFrameStub(void* clientData) {((AlsaDeviceSource*) clientData)->deliverFrame();};
		void deliverFrame();
		static void incomingPacketHandlerStub(void* clientData, int mask) { ((AlsaDeviceSource*) clientData)->getNextFrame(); };
		int getNextFrame();
		void processFrame(char * frame, int frameSize, const timeval &ref);
		void queueFrame(char * frame, int frameSize, const timeval &tv);



		// split packet in frames
		virtual std::list< std::pair<unsigned char*,size_t> > splitFrames(unsigned char* frame, unsigned frameSize);
		
		// overide FramedSource
		virtual void doGetNextFrame();	
		virtual void doStopGettingFrames();
					
	protected:
		
		std::list<Frame*> m_captureQueue;
		Stats m_in;
		Stats m_out;
		EventTriggerId m_eventTriggerId;
		int m_outfd;
		unsigned int m_queueSize;
		pthread_t m_thid;
		pthread_mutex_t m_mutex;
		std::string m_auxLine;
};

#endif
