/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** ALSACapture.h
** 
** V4L2 RTSP streamer                                                                 
**                                                                                    
** ALSA capture overide of V4l2Capture
**                                                                                    
** -------------------------------------------------------------------------*/

#ifndef ALSA_CAPTURE
#define ALSA_CAPTURE


#include "lame.h"
#include "logger.h"
#include "../../opus/include/opus.h"
#include "../../v4l2rtspserver-tools/sharedmem.h"
typedef enum
{
    ENCODE_MP3,
    ENCODE_OPUS,
    ENCODE_PCM
} audioencoding;


struct ALSACaptureParameters 
{
	ALSACaptureParameters(const char* devname,  unsigned int inSampleRate, unsigned int outSampleRate, int verbose, audioencoding encode) :
		m_devName(devname), m_inSampleRate(inSampleRate),m_outSampleRate(outSampleRate), m_channels(1), m_verbose(verbose), m_encode(encode) {};


	std::string      m_devName;
	unsigned int     m_inSampleRate;
	unsigned int     m_outSampleRate;
	unsigned int     m_channels; //Always 1
	int              m_verbose;
	audioencoding    m_encode;

};
#define RECBUF_SIZE		8192*2
class ALSACapture 
{
	public:
		static ALSACapture* createNew(const ALSACaptureParameters & params) ;
		virtual ~ALSACapture();
		void close();
	
	protected:
		ALSACapture(const ALSACaptureParameters & params);
		size_t readMP3(char* buffer, size_t bufferSize);
		size_t readOpus(char* buffer, size_t bufferSize);
		size_t readPCM(char* buffer, size_t bufferSize);
        short filter(short val,bool swap, int num_sample =0);

	public:
		virtual size_t read(char* buffer, size_t bufferSize);		
		virtual int getFd();
		
		virtual unsigned long getBufferSize() { return RECBUF_SIZE; };
		virtual int getWidth()  {return -1;}
		virtual int getHeight() {return -1;}	
		
		unsigned long getInSampleRate() { return m_params.m_inSampleRate; };
		unsigned long getOutSampleRate() { return m_params.m_outSampleRate; };
		unsigned long getChannels  () { return m_params.m_channels; };
		
	private:
		unsigned long         m_bufferSize;
		unsigned long         m_periodSize;
		ALSACaptureParameters m_params;
        int fd;
		lame_global_flags *gfp;
		OpusEncoder *encoder;
		SharedMem m_sharedMem;
		struct shared_conf m_currentConfig;
		shared_conf *m_newConfig;
		int m_Filtermethod;
};

#endif


