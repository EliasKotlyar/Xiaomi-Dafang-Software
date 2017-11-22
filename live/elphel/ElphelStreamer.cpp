/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**********/
// Copyright (c) 1996-2003, Live Networks, Inc.  All rights reserved
// A test program that reads JPEG frames from the Elphel camera,
// and streams them via RTP/RTCP
// main program

#include "liveMedia.hh"
#include "GroupsockHelper.hh"

#include "BasicUsageEnvironment.hh"
#include "ElphelJPEGDeviceSource.hh"

UsageEnvironment* env;
char* progName;
int fps;

void play(); // forward

void usage() {
  *env << "Usage: " << progName << " <frames-per-second>\n";
  exit(1);
}

int main(int argc, char** argv) {
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  //OutPacketBuffer::numPacketsLimit = 100;
  // Allow for up to 100 RTP packets per JPEG frame

  progName = argv[0];
  if (argc != 2) usage();

  if (sscanf(argv[1], "%d", &fps) != 1 || fps <= 0) {
    usage();
  }

  play();

  return 0;
}

void afterPlaying(void* clientData); // forward

// A structure to hold the state of the current session.
// It is used in the "afterPlaying()" function to clean up the session.
struct sessionState_t {
  FramedSource* source;
  RTPSink* sink;
  RTCPInstance* rtcpInstance;
  Groupsock* rtpGroupsock;
  Groupsock* rtcpGroupsock;
  RTSPServer* rtspServer;
} sessionState;

void play() {
  // Open the Elphel camera:
  unsigned timePerFrame = 1000000/fps; // microseconds
  sessionState.source
    = ElphelJPEGDeviceSource::createNew(*env, timePerFrame);
  if (sessionState.source == NULL) {
    *env << "Unable to open Elphel camera: "
	 << env->getResultMsg() << "\n";
    exit(1);
  }

  // Create 'groupsocks' for RTP and RTCP:
  struct in_addr destinationAddress;
  destinationAddress.s_addr = chooseRandomIPv4SSMAddress(*env);

  const unsigned short rtpPortNum = 2222;
  const unsigned short rtcpPortNum = rtpPortNum+1;
  const unsigned char ttl = 255;
  
  const Port rtpPort(rtpPortNum);
  const Port rtcpPort(rtcpPortNum);
  
  sessionState.rtpGroupsock
    = new Groupsock(*env, destinationAddress, rtpPort, ttl);
  sessionState.rtpGroupsock->multicastSendOnly(); // we're a SSM source
  sessionState.rtcpGroupsock
    = new Groupsock(*env, destinationAddress, rtcpPort, ttl);
  sessionState.rtcpGroupsock->multicastSendOnly(); // we're a SSM source
  
  // Create an appropriate RTP sink from the RTP 'groupsock':
  sessionState.sink
    = JPEGVideoRTPSink::createNew(*env, sessionState.rtpGroupsock);
  
  // Create (and start) a 'RTCP instance' for this RTP sink:
  unsigned const averageFrameSizeInBytes = 35000; // estimate
  const unsigned totalSessionBandwidth
    = (8*1000*averageFrameSizeInBytes)/timePerFrame;
      // in kbps; for RTCP b/w share
  const unsigned maxCNAMElen = 100;
  unsigned char CNAME[maxCNAMElen+1];
  //gethostname((char*)CNAME, maxCNAMElen);
  sprintf((char*)CNAME, "Elphel camera"); // "gethostname()" isn't supported
  CNAME[maxCNAMElen] = '\0'; // just in case
  sessionState.rtcpInstance
    = RTCPInstance::createNew(*env, sessionState.rtcpGroupsock,
			      totalSessionBandwidth, CNAME,
			      sessionState.sink, NULL /* we're a server */,
			      True /* we're a SSM source*/);
  // Note: This starts RTCP running automatically

  // Create and start a RTSP server to serve this stream:
  sessionState.rtspServer
    = RTSPServer::createNew(*env, 7070);
  if (sessionState.rtspServer == NULL) {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }
ServerMediaSession* sms
  = ServerMediaSession::createNew(*env, NULL, progName,
	   "Session streamed by the Elphel camera", True/*SSM*/);
 sms->addSubsession(PassiveServerMediaSubsession
		    ::createNew(*sessionState.sink));
 sessionState.rtspServer->addServerMediaSession(sms);
 
 char* url = sessionState.rtspServer->rtspURL(sms);
 *env << "Play this stream using the URL \"" << url << "\"\n";
 delete[] url;

  // Finally, start the streaming:
  *env << "Beginning streaming...\n";
  sessionState.sink->startPlaying(*sessionState.source, afterPlaying, NULL);

  env->taskScheduler().doEventLoop();
}


void afterPlaying(void* /*clientData*/) {
  *env << "...done streaming\n";

  // End by closing the media:
  Medium::close(sessionState.rtspServer);
  Medium::close(sessionState.sink);
  delete sessionState.rtpGroupsock;
  Medium::close(sessionState.source);
  Medium::close(sessionState.rtcpInstance);
  delete sessionState.rtcpGroupsock;

  // We're done:
  exit(0);
}
