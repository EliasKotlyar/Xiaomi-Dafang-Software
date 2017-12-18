/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** v4l2DeviceSource.cpp
** 
** V4L2 Live555 source 
**
** -------------------------------------------------------------------------*/

#include <fcntl.h>
#include <iomanip>
#include <sstream>

// libv4l2
#include <linux/videodev2.h>
//#include <libv4l2.h>

// project
#include "V4l2DeviceSource.h"

// ---------------------------------
// V4L2 FramedSource Stats
// ---------------------------------
int  V4L2DeviceSource::Stats::notify(int tv_sec, int framesize)
{
	m_fps++;
	m_size+=framesize;
	if (tv_sec != m_fps_sec)
	{
		//LOG(INFO) << m_msg  << "tv_sec:" <<   tv_sec << " fps:" << m_fps << " bandwidth:"<< (m_size/128) << "kbps\n";
		fprintf(stderr, "%s tv_sec: %d, fps: %d, bandwidth: %d kbps\n", m_msg.c_str(), tv_sec, m_fps, (m_size/128));		
		m_fps_sec = tv_sec;
		m_fps = 0;
		m_size = 0;
	}
	return m_fps;
}

// ---------------------------------
// V4L2 FramedSource
// ---------------------------------
V4L2DeviceSource* V4L2DeviceSource::createNew(UsageEnvironment& env, V4L2DeviceParameters params, int outputFd, bool useThread) 
{ 	
	V4L2DeviceSource* source = NULL;
	source = new V4L2DeviceSource(env, params, outputFd, useThread);
	return source;
}

// Constructor
V4L2DeviceSource::V4L2DeviceSource(UsageEnvironment& env, V4L2DeviceParameters params, int outputFd, bool useThread) 
	: FramedSource(env), 
	m_params(params), 
	m_in("in"), 
	m_out("out") , 
	m_outfd(outputFd),
	m_queueSize(params.m_queueSize)
{

	//m_device->cb = callback;
	m_eventTriggerId = envir().taskScheduler().createEventTrigger(V4L2DeviceSource::deliverFrameStub);
	memset(&m_mutex, 0, sizeof(m_mutex));
	pthread_mutex_init(&m_mutex, NULL);
#if 0
	if (m_device)
	{
		if (useThread)
		{
			pthread_mutex_init(&m_mutex, NULL);
			pthread_create(&m_thid, NULL, threadStub, this);		
		}
		else
		{
			envir().taskScheduler().turnOnBackgroundReadHandling( m_device->getFd(), V4L2DeviceSource::incomingPacketHandlerStub, this);
		}
	}
#endif
}

// Destructor
V4L2DeviceSource::~V4L2DeviceSource()
{	
	printf("~V4L2DeviceSource\n");
	envir().taskScheduler().deleteEventTrigger(m_eventTriggerId);
	pthread_mutex_destroy(&m_mutex);
#if 0	
	pthread_join(m_thid, NULL);	
#endif
}

// thread mainloop
void* V4L2DeviceSource::thread()
{
#if 0
	int stop=0;
	fd_set fdset;
	FD_ZERO(&fdset);
	timeval tv;
	
	//LOG(NOTICE) << "begin thread\n"; 
	while (!stop) 
	{
		FD_SET(m_device->getFd(), &fdset);
		tv.tv_sec=1;
		tv.tv_usec=0;	
		int ret = select(m_device->getFd()+1, &fdset, NULL, NULL, &tv);
		if (ret == 1)
		{
			if (FD_ISSET(m_device->getFd(), &fdset))
			{
				if (this->getNextFrame() <= 0)
				{
				//	LOG(ERROR) << "error:" << strerror(errno) << "\n"; 						
					stop=1;
				}
			}
		}
		else if (ret == -1)
		{
		//	LOG(ERROR) << "stop " << strerror(errno) << "\n"; 
			stop=1;
		}
	}
	//LOG(NOTICE) << "end thread\n"; 
#endif
	return NULL;
}

// getting FrameSource callback
void V4L2DeviceSource::doGetNextFrame()
{
	int isQueueEmpty = 0;
	//printf("V4L2DeviceSource::doGetNextFrame\n");
	pthread_mutex_lock (&m_mutex);
	isQueueEmpty = m_captureQueue.empty();
	pthread_mutex_unlock (&m_mutex);
	
	if(!isQueueEmpty)
		deliverFrame();

}

// stopping FrameSource callback
void V4L2DeviceSource::doStopGettingFrames()
{
	//LOG(NOTICE) << "V4L2DeviceSource::doStopGettingFrames\n";	
	FramedSource::doStopGettingFrames();
}

// deliver frame to the sink
void V4L2DeviceSource::deliverFrame()
{
	int isQueueEmpty = 0;
	if (isCurrentlyAwaitingData()) 
	{
		//fprintf(stderr, "V4L2DeviceSource::isCurrentlyAwaitingData\n");
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

			//LOG(DEBUG) << "deliverFrame\ttimestamp:" << fPresentationTime.tv_sec << "." << fPresentationTime.tv_usec << "\tsize:" << fFrameSize <<"\tdiff" <<  (diff.tv_sec*1000+diff.tv_usec/1000) << "ms";		
			//fprintf(stderr, "deliverFrame\ttimestamp: %u.%u\tsize: %d \tdiff: %d ms\n",fPresentationTime.tv_sec, fPresentationTime.tv_usec, fFrameSize, (diff.tv_sec*1000+diff.tv_usec/1000));

			memcpy(fTo, frame->m_buffer, fFrameSize);
			delete frame;
		}
		
		// send Frame to the consumer
		FramedSource::afterGetting(this);			
	}
}
	
// FrameSource callback on read event
int V4L2DeviceSource::getNextFrame() 
{
#if 0
	char buffer[m_device->getBufferSize()];	
	timeval ref;
	gettimeofday(&ref, NULL);											
	int frameSize = m_device->read(buffer,  m_device->getBufferSize());
	
	if (frameSize < 0)
	{
		//LOG(NOTICE) << "V4L2DeviceSource::getNextFrame errno:" << errno << " "  << strerror(errno) << "\n";		
		handleClosure(this);
	}
	else if (frameSize == 0)
	{
		//LOG(NOTICE) << "V4L2DeviceSource::getNextFrame no data errno:" << errno << " "  << strerror(errno) << "\n";		
		handleClosure(this);
	}
	else
	{
		timeval tv;
		gettimeofday(&tv, NULL);												
		timeval diff;
		timersub(&tv,&ref,&diff);
		m_in.notify(tv.tv_sec, frameSize);
		//LOG(DEBUG) << "getNextFrame\ttimestamp:" << ref.tv_sec << "." << ref.tv_usec << "\tsize:" << frameSize <<"\tdiff" <<  (diff.tv_sec*1000+diff.tv_usec/1000) << "ms\tqueue:" << m_captureQueue.size();
		processFrame(buffer,frameSize,ref);
	}			

	return frameSize;
#else 

	//fprintf(stderr, "V4L2DeviceSource::getNextFrame\n");
	//doGetNextFrame();
	return 0;
#endif
}	

void V4L2DeviceSource::videocallback( const struct timeval *tv, 
			void *data, 
			size_t len,
			int keyFrame )
{

	static unsigned long serialNum = 0;

	
	if(data == NULL || len == 0) {
		fprintf(stderr, "data is null or len is zero\n");
		return;
	}
	// Do something here

// JPEG HEADER PARSER
//	if (m_params.m_format == V4L2_PIX_FMT_MJPEG) {
#if 0
					int headerSize = 0;
                    bool headerOk = false;
                    char * buf = (char *)data;

                    unsigned int SOF_POS = 13;
                    unsigned int DQT_POS = 32;
                    unsigned int EOH_POS = 598;
                    //fprintf(stderr, "frame size : %d\n", (unsigned int)len);

                    //for (unsigned int i = SOF_POS; i < (unsigned int)len ; i++)
                    {
                    	unsigned int i = SOF_POS;

                    	//fprintf(stderr, "0x%x, ", buf[i]);
                        // SOF
                        if ( (i+8) < (unsigned int)len   && buf[i] == 0xFF && buf[i+1] == 0xC0 )
                        {
#if 0
                             _height = (buf[i+5]<<5)|(buf[i+6]>>3);
                             _width = (buf[i+7]<<5)|(buf[i+8]>>3);
                             //fprintf(stderr, "SOF %d , width : %d  height : %d\n", i, _width, _height);
#endif
                        }

                        i = DQT_POS;

                        // DQT
                        if ( (i+5+64) < (unsigned int)len  && buf[i] == 0xFF && buf[i+1] == 0xDB)
                        {
#if 0
                        	//fprintf(stderr, "DQT %d\n", i);
                            if (buf[i+4] ==0)
                            {
                                memcpy(_qTable, buf + i + 5, 64);
                                _qTable0Init = true;
                            }
                            else if (buf[i+4] ==1)
                            {
                                memcpy(_qTable + 64, buf + i + 5, 64);
                                _qTable1Init = true;
                            }
#endif
                        }

                        i = EOH_POS;
                        // End of header
                        if ( (i+1) < (unsigned int)len  && buf[i] == 0x3F && buf[i+1] == 0x00 )
                        {

                        	//fprintf(stderr, "EOH %d\n", i);
                            headerOk = true;
                            headerSize = i+2;
                             //break;
                        }
                        if (headerOk) {

                        	//fprintf(stderr, "width %u, height %d, _qTable0Init %d, _qTable1Init %d\n", _width, _height, _qTable0Init, _qTable1Init);
							//len = (size_t)((unsigned int)len - headerSize);
							//data = buf + headerSize;
						}
                    }
#endif

 //   } else {
    		processFrame((char *)data,(int)len,*tv);
 //   }

	//fprintf(stderr, "[%d]video_h264hd_cb serN:%lu, len = %d \n", ret, serialNum, len);

}

		
void V4L2DeviceSource::processFrame(char * frame, int frameSize, const timeval &ref) 
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
void V4L2DeviceSource::queueFrame(char * frame, int frameSize, const timeval &tv) 
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
std::list< std::pair<unsigned char*,size_t> > V4L2DeviceSource::splitFrames(unsigned char* frame, unsigned frameSize) 
{				
	std::list< std::pair<unsigned char*,size_t> > frameList;
	if (frame != NULL)
	{
		frameList.push_back(std::make_pair<unsigned char*,size_t>(frame, frameSize));
	}
	return frameList;
}


	
