/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** AlsaDeviceSource.cpp
** 
** V4L2 Live555 source 
**
** -------------------------------------------------------------------------*/

#include <fcntl.h>
#include <sstream>

// libv4l2
//#include <linux/videodev2.h>
//#include <libv4l2.h>

// project
#include "AlsaDeviceSource.h"

// ---------------------------------
// V4L2 FramedSource Stats
// ---------------------------------
int  AlsaDeviceSource::Stats::notify(int tv_sec, int framesize)
{
	m_fps++;
	m_size+=framesize;
	if (tv_sec != m_fps_sec)
	{
		//LOG(INFO) << m_msg  << "tv_sec:" <<   tv_sec << " fps:" << m_fps << " bandwidth:"<< (m_size/128) << "kbps\n";
		fprintf(stderr, "audio %s tv_sec: %d, framecount: %d, bitrate: %d kbps\n", m_msg.c_str(), tv_sec, m_fps, (m_size/128));		
		m_fps_sec = tv_sec;
		m_fps = 0;
		m_size = 0;
	}
	return m_fps;
}

// ---------------------------------
// V4L2 FramedSource
// ---------------------------------
AlsaDeviceSource* AlsaDeviceSource::createNew(UsageEnvironment& env, int outputFd, unsigned int queueSize, bool useThread) 
{ 	
	AlsaDeviceSource* source = NULL;
	source = new AlsaDeviceSource(env, outputFd, queueSize, useThread);
	return source;
}

// Constructor
AlsaDeviceSource::AlsaDeviceSource(UsageEnvironment& env, int outputFd, unsigned int queueSize, bool useThread) 
	: FramedSource(env), 
	m_in("in"), 
	m_out("out") , 
	m_outfd(outputFd),
	m_queueSize(queueSize)
{

	m_eventTriggerId = envir().taskScheduler().createEventTrigger(AlsaDeviceSource::deliverFrameStub);
	memset(&m_mutex, 0, sizeof(m_mutex));
	pthread_mutex_init(&m_mutex, NULL);
}

// Destructor
AlsaDeviceSource::~AlsaDeviceSource()
{	
	envir().taskScheduler().deleteEventTrigger(m_eventTriggerId);
	pthread_mutex_destroy(&m_mutex);
}

// thread mainloop
void* AlsaDeviceSource::thread()
{
	return NULL;
}

// getting FrameSource callback
void AlsaDeviceSource::doGetNextFrame()
{

	int isQueueEmpty = 0;
	//printf("AlsaDeviceSource::doGetNextFrame\n");
	pthread_mutex_lock (&m_mutex);
	isQueueEmpty = m_captureQueue.empty();
	pthread_mutex_unlock (&m_mutex);
	
	if(!isQueueEmpty)
		deliverFrame();

}

// stopping FrameSource callback
void AlsaDeviceSource::doStopGettingFrames()
{
	FramedSource::doStopGettingFrames();
}

// deliver frame to the sink
void AlsaDeviceSource::deliverFrame()
{			
	int isQueueEmpty = 0;
	if (isCurrentlyAwaitingData()) 
	{
		fDurationInMicroseconds = 0;
		fFrameSize = 0;

		pthread_mutex_lock (&m_mutex);
		isQueueEmpty = m_captureQueue.empty();
		pthread_mutex_unlock (&m_mutex);
		
		//if (m_captureQueue.empty())
		if(isQueueEmpty)
		{
			//LOG(DEBUG) << "Queue is empty \n";		
		}
		else
		{				
			gettimeofday(&fPresentationTime, NULL);

			pthread_mutex_lock (&m_mutex);		

			Frame * frame = m_captureQueue.front();
			m_captureQueue.pop_front();

			pthread_mutex_unlock (&m_mutex);
	
			m_out.notify(fPresentationTime.tv_sec, frame->m_size);
			if (frame->m_size > fMaxSize) 
			{
				fFrameSize = fMaxSize;
				fNumTruncatedBytes = frame->m_size - fMaxSize;
			} 
			else 
			{
				fFrameSize = frame->m_size;
			}
			timeval diff;
			timersub(&fPresentationTime,&(frame->m_timestamp),&diff);

			//LOG(DEBUG) << "deliverFrame\ttimestamp:" << fPresentationTime.tv_sec << "." << fPresentationTime.tv_usec << "\tsize:" << fFrameSize <<"\tdiff" <<  (diff.tv_sec*1000+diff.tv_usec/1000) << "ms\tqueue:" << m_captureQueue.size();		
			
			memcpy(fTo, frame->m_buffer, fFrameSize);
			delete frame;
		}
		
		// send Frame to the consumer
		FramedSource::afterGetting(this);			
	}
}
	
// FrameSource callback on read event
int AlsaDeviceSource::getNextFrame() 
{

	return 0;

}	


void AlsaDeviceSource::audiocallback(const struct timeval *tv, 
		void *data, 
		size_t len,
		void *cbarg)
{

	if(data == NULL || len == 0) {
		fprintf(stderr, "data is null or len is zero\n");
		return;
	}
	// Do something here

	processFrame((char *)data,(int)len,*tv);

}

		
void AlsaDeviceSource::processFrame(char * frame, int frameSize, const timeval &ref) 
{
	timeval tv;
	gettimeofday(&tv, NULL);												
	timeval diff;
	timersub(&tv,&ref,&diff);
		
	std::list< std::pair<unsigned char*,size_t> > frameList = this->splitFrames((unsigned char*)frame, frameSize);
	while (!frameList.empty())
	{
		std::pair<unsigned char*,size_t> & frame = frameList.front();
		size_t size = frame.second;
		char* buf = new char[size];
		memcpy(buf, frame.first, size);
		queueFrame(buf,size,ref);

		//LOG(DEBUG) << "queueFrame\ttimestamp:" << ref.tv_sec << "." << ref.tv_usec << "\tsize:" << frameSize <<"\tdiff" <<  (diff.tv_sec*1000+diff.tv_usec/1000) << "ms\tqueue:" << m_captureQueue.size();		
		if (m_outfd != -1) write(m_outfd, buf, size);

		frameList.pop_front();
	}			
}	

// post a frame to fifo
void AlsaDeviceSource::queueFrame(char * frame, int frameSize, const timeval &tv) 
{
	pthread_mutex_lock (&m_mutex);
	while (m_captureQueue.size() >= m_queueSize)
	{
		//LOG(DEBUG) << "Queue full size drop frame size:"  << (int)m_captureQueue.size() << " \n";

		delete m_captureQueue.front();
		m_captureQueue.pop_front();
	}
	m_captureQueue.push_back(new Frame(frame, frameSize, tv));
	pthread_mutex_unlock (&m_mutex);
	
	// post an event to ask to deliver the frame
	envir().taskScheduler().triggerEvent(m_eventTriggerId, this);
}	

// split packet in frames					
std::list< std::pair<unsigned char*,size_t> > AlsaDeviceSource::splitFrames(unsigned char* frame, unsigned frameSize) 
{				
	std::list< std::pair<unsigned char*,size_t> > frameList;
	if (frame != NULL)
	{
		frameList.push_back(std::make_pair<unsigned char*,size_t>(frame, frameSize));
	}
	return frameList;
}


	
