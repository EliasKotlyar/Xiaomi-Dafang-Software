/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** ServerMediaSubsession.cpp
** 
** -------------------------------------------------------------------------*/

#include <sstream>

// libv4l2
#include <linux/videodev2.h>

// live555
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include <Base64.hh>
#include "JPEGVideoRTPSink.hh"

// project
#include "ServerMediaSubsession.h"
#include "V4l2DeviceSource.h"
#include "MJPEGVideoSource.h"


// ---------------------------------
//   BaseServerMediaSubsession
// ---------------------------------
FramedSource* BaseServerMediaSubsession::createSource(UsageEnvironment& env, FramedSource * videoES, int format, V4L2DeviceParameters params)
{
	FramedSource* source = NULL;
	switch (format)
	{
		case V4L2_PIX_FMT_H264 : source = H264VideoStreamDiscreteFramer::createNew(env, videoES); break;

		case V4L2_PIX_FMT_MJPEG: source = MJPEGVideoSource::createNew(env, videoES, params); break;

#ifdef 	V4L2_PIX_FMT_VP8	
		case V4L2_PIX_FMT_VP8  : source = videoES; break;
#endif
		case WA_PCMA : source = videoES; break;
	}
	return source;
}

RTPSink*  BaseServerMediaSubsession::createSink(UsageEnvironment& env, Groupsock * rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, int format)
{
	RTPSink* videoSink = NULL;
	switch (format)
	{
		case V4L2_PIX_FMT_H264 : videoSink = H264VideoRTPSink::createNew(env, rtpGroupsock,rtpPayloadTypeIfDynamic); break;

		case V4L2_PIX_FMT_MJPEG : videoSink = JPEGVideoRTPSink::createNew(env, rtpGroupsock); break;

#ifdef 	V4L2_PIX_FMT_VP8	
		case V4L2_PIX_FMT_VP8  : videoSink = VP8VideoRTPSink::createNew(env, rtpGroupsock,rtpPayloadTypeIfDynamic); break;
#endif
		case WA_PCMA :
					char const* mimeType;
  					unsigned char payloadFormatCode = 8; // by default, unless a static RTP payload type can be used
					unsigned const samplingFrequency = SAMPLE_RATE;
					unsigned char const numChannels =1;
				if (format == WA_PCMU) {
				    mimeType = "PCMU";
				    if (samplingFrequency == 8000 && numChannels == 1) {
				      payloadFormatCode = 0; // a static RTP payload type                                                                          
				    }
				  } else if (format == WA_PCMA) {
				    mimeType = "PCMA";
				    if (samplingFrequency == 8000 && numChannels == 1) {
				      payloadFormatCode = 8; // a static RTP payload type                                                                          
				    } 
				  } else if (format == WA_IMA_ADPCM) {
				    mimeType = "DVI4";
				    // Use a static payload type, if one is defined:                                                                               
				    if (numChannels == 1) {
					      if (samplingFrequency == 8000) {
								payloadFormatCode = 5; // a static RTP payload type                                                                        
					      } else if (samplingFrequency == 16000) {
								payloadFormatCode = 6; // a static RTP payload type                                                                        
					      } else if (samplingFrequency == 11025) {
								payloadFormatCode = 16; // a static RTP payload type                                                                       
					      } else if (samplingFrequency == 22050) {
								payloadFormatCode = 17; // a static RTP payload type                                                                       
					      }
				    }
				  } else { //unknown format                                                                                                        
				    //*env << "Unknown audio format code \"" << format << "\" in WAV file header\n";
				    fprintf(stderr, "Error Unknown audio format code \n");

				  }

				  	fprintf(stderr, "create audio sink %d %d %s \n",  payloadFormatCode, samplingFrequency, mimeType);

					videoSink = SimpleRTPSink::createNew(env, rtpGroupsock, payloadFormatCode, samplingFrequency,
						"audio", mimeType, numChannels);
			break;
	}
	return videoSink;
}

char const* BaseServerMediaSubsession::getAuxLine(V4L2DeviceSource* source,unsigned char rtpPayloadType)
{
	const char* auxLine = NULL;
	if (source)
	{
		std::ostringstream os; 
		os << "a=fmtp:" << int(rtpPayloadType) << " ";				
		os << source->getAuxLine();				
		os << "\r\n";				
		auxLine = strdup(os.str().c_str());
		printf("auxLine %s\n", auxLine); //haoweilo
	} 
	return auxLine;
}

// -----------------------------------------
//    ServerMediaSubsession for Multicast
// -----------------------------------------
MulticastServerMediaSubsession* MulticastServerMediaSubsession::createNew(UsageEnvironment& env
									, struct in_addr destinationAddress
									, Port rtpPortNum, Port rtcpPortNum
									, int ttl
									, StreamReplicator* replicator
									, int format
									, V4L2DeviceParameters params
									) 
{ 
	// Create a source
	FramedSource* source = replicator->createStreamReplica();

	FramedSource* videoSource = createSource(env, source, format, params);

	// Create RTP/RTCP groupsock
	Groupsock* rtpGroupsock = new Groupsock(env, destinationAddress, rtpPortNum, ttl);
	Groupsock* rtcpGroupsock = new Groupsock(env, destinationAddress, rtcpPortNum, ttl);

	// Create a RTP sink
	RTPSink* videoSink = createSink(env, rtpGroupsock, 96, format);

	// Create 'RTCP instance'
	const unsigned maxCNAMElen = 100;
	unsigned char CNAME[maxCNAMElen+1];
	gethostname((char*)CNAME, maxCNAMElen);
	CNAME[maxCNAMElen] = 'SONiX Camera\0'; 
	RTCPInstance* rtcpInstance = RTCPInstance::createNew(env, rtcpGroupsock,  500, CNAME, videoSink, NULL);

	// Start Playing the Sink
	videoSink->startPlaying(*videoSource, NULL, NULL);
	
	return new MulticastServerMediaSubsession(replicator, videoSink, rtcpInstance);
}
		
char const* MulticastServerMediaSubsession::sdpLines() 
{
	if (m_SDPLines.empty())
	{
		// Ugly workaround to give SPS/PPS that are get from the RTPSink 
		m_SDPLines.assign(PassiveServerMediaSubsession::sdpLines());
		m_SDPLines.append(getAuxSDPLine(m_rtpSink,NULL));
	}
	return m_SDPLines.c_str();
}

char const* MulticastServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink,FramedSource* inputSource)
{
	return this->getAuxLine(dynamic_cast<V4L2DeviceSource*>(m_replicator->inputSource()), rtpSink->rtpPayloadType());
}
		
// -----------------------------------------
//    ServerMediaSubsession for Unicast
// -----------------------------------------
UnicastServerMediaSubsession* UnicastServerMediaSubsession::createNew(UsageEnvironment& env, StreamReplicator* replicator, int format, V4L2DeviceParameters params) 
{ 
	return new UnicastServerMediaSubsession(env,replicator,format,params);
}
					
FramedSource* UnicastServerMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
	FramedSource* source = m_replicator->createStreamReplica();
	//printf("UnicastServerMediaSubsession::createNewStreamSource\n");
	return createSource(envir(), source, m_format, m_params);
}

		
RTPSink* UnicastServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,  unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
	//printf("UnicastServerMediaSubsession::createNewRTPSink\n");
	return createSink(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, m_format);
}
		
char const* UnicastServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink,FramedSource* inputSource)
{
	return this->getAuxLine(dynamic_cast<V4L2DeviceSource*>(m_replicator->inputSource()), rtpSink->rtpPayloadType());
}
