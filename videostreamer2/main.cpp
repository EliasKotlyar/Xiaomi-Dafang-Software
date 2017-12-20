/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** main.cpp
** 
** V4L2 RTSP streamer                                                                 
**                                                                                    
** H264 capture using middleware_video                                                            
** RTSP using live555                                                                 
**                                                                                    
** -------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sstream>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include "ImpJpegVideoDeviceSource.h"

// libv4l2
#include <linux/videodev2.h>

// live555
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>

// project

#include "H264_V4l2DeviceSource.h"
#include "ServerMediaSubsession.h"
#include "AlsaDeviceSource.h"


#define SNX_RTSP_SERVER_VERSION        ("V00.01.04")

// create live555 environment
UsageEnvironment *env = NULL;
TaskScheduler *scheduler = NULL;

// create RTSP server
RTSPServer *rtspServer = NULL;
FramedSource *videoES = NULL;

StreamReplicator *replicator = NULL;

#if AUDIO_STREAM

AlsaDeviceSource*  audioES = NULL;
#endif


// -----------------------------------------
//    add an RTSP session
// -----------------------------------------
void addSession(RTSPServer *rtspServer, const char *sessionName, ServerMediaSubsession *subSession,
                ServerMediaSubsession *audio_subSession) {
    UsageEnvironment &env(rtspServer->envir());
    ServerMediaSession *sms = ServerMediaSession::createNew(env, sessionName);
    sms->addSubsession(subSession);

    if (audio_subSession)
        sms->addSubsession(audio_subSession);

    rtspServer->addServerMediaSession(sms);

    char *url = rtspServer->rtspURL(sms);

    fprintf(stderr, "lay this stream using the URL: \"%s\"\n", url);

    delete[] url;
}




// -----------------------------------------
//    signal handler
// -----------------------------------------
/*
 * NOTE: Please finish this program by kill -2
 */

char quit = 0;

void sighandler(int n) {
    printf("Signal received (%d)\n", n);
    quit = 1;
#if 0
#if AUDIO_STREAM
    if(audioES)
        Medium::close(audioES);
#endif

    if(videoES)
        Medium::close(videoES);

#if AUDIO_STREAM

#endif



    if (rtspServer)
    Medium::close(rtspServer);

    if (env)
        env->reclaim();
    delete scheduler;
#endif
}


// -----------------------------------------
//    entry point
// -----------------------------------------
int main(int argc, char **argv) {

#if AUDIO_STREAM

    StreamReplicator* audio_replicator = NULL;
#endif
    // default parameters
    const char *dev_name = "/dev/video1";
    int format = V4L2_PIX_FMT_H264;
    int width = 1280;
    int height = 720;
    int queueSize = 10;
    int fps = 30;
    int isp_fps = 30;
    int bitrate = 1024; //(Kbps)
    int mjpeg_qp = 120;
    int m2m_en = 1;
    int gop = fps;
    unsigned short rtpPortNum = 20000;
    unsigned short rtcpPortNum = rtpPortNum + 1;
    unsigned char ttl = 5;
    struct in_addr destinationAddress;
    unsigned short rtspPort = 554;
    unsigned short rtspOverHTTPPort = 0;
    bool multicast = false;
    int verbose = 0;
    std::string outputFile;
    //bool useMmap = true;
    std::string url = "unicast";
    std::string murl = "multicast";
    bool useThread = true;
    in_addr_t maddr = INADDR_NONE;


    // decode parameters
    int c = 0;
    //while ((c = getopt (argc, argv, "hW:H:Q:P:F:v::O:T:m:u:rsM:")) != -1)
#if AUDIO_STREAM
    bool audio_en = false;
    while ((c = getopt (argc, argv, "hb:W:H:g:Q:P:F:i:O:T:m:u:M:aj:")) != -1)
#else
    while ((c = getopt(argc, argv, "hb:W:H:g:Q:P:F:i:O:T:m:u:M:j:")) != -1)
#endif
    {
        switch (c) {
            case 'O':
                outputFile = optarg;
                break;
            case 'v':
                verbose = 1;
                if (optarg && *optarg == 'v') verbose++;
                break;
            case 'm':
                multicast = true;
                if (optarg) murl = optarg;
                break;
            case 'M':
                multicast = true;
                if (optarg) maddr = inet_addr(optarg);
                break;
            case 'g':
                gop = atoi(optarg);
                break;
            case 'b':
                bitrate = atoi(optarg);
                break;
            case 'W':
                width = atoi(optarg);
                break;
            case 'H':
                height = atoi(optarg);
                break;
            case 'Q':
                queueSize = atoi(optarg);
                break;
            case 'P':
                rtspPort = atoi(optarg);
                break;
            case 'T':
                rtspOverHTTPPort = atoi(optarg);
                break;
            case 'F':
                fps = atoi(optarg);
                break;
            case 'i':
                isp_fps = atoi(optarg);
                break;
                //case 'r':	useMmap =  false; break;
                //case 's':	useThread =  false; break;
            case 'u':
                url = optarg;
                break;
#if AUDIO_STREAM
                case 'a':	audio_en = true; break;
#endif
            case 'j':
                format = V4L2_PIX_FMT_MJPEG;
                mjpeg_qp = atoi(optarg);
                break;
            case 'h':
            default: {
                std::cout << argv[0] << "Version:" << SNX_RTSP_SERVER_VERSION << std::endl;
                std::cout << "Usage :" << std::endl;
                std::cout << "\t " << argv[0]
                          << " [-a] [-j mjpeg_qp] [-m] [-P RTSP port][-T RTSP/HTTP port][-Q queueSize] [-M groupaddress] [-b bitrate] [-W width] [-H height] [-F fps] [-i isp_fps] [device]"
                          << std::endl;

                std::cout << "\t -Q length: Number of frame queue  (default " << queueSize << ")" << std::endl;
                std::cout << "\t RTSP options :" << std::endl;
                std::cout << "\t -u url     : unicast url (default " << url << ")" << std::endl;
                std::cout << "\t -m url     : multicast url (default " << murl << ")" << std::endl;
                std::cout << "\t -M addr    : multicast group   (default is a random address)" << std::endl;
                std::cout << "\t -P port    : RTSP port (default " << rtspPort << ")" << std::endl;
                std::cout << "\t -T port    : RTSP over HTTP port (default " << rtspOverHTTPPort << ")" << std::endl;
                std::cout << "\t V4L2 options :" << std::endl;
                //std::cout << "\t -r       : V4L2 capture using read interface (default use memory mapped buffers)" << std::endl;
                //std::cout << "\t -s       : V4L2 capture using live555 mainloop (default use a separated reading thread)" << std::endl;
                std::cout << "\t -F fps     : V4L2 capture framerate (default " << fps << ")" << std::endl;
                std::cout << "\t -i isp_fps : ISP capture framerate (default " << isp_fps << ")" << std::endl;
                std::cout << "\t -W width   : V4L2 capture width (default " << width << ")" << std::endl;
                std::cout << "\t -H height  : V4L2 capture height (default " << height << ")" << std::endl;

                std::cout << "\t V4L2 H264 options :" << std::endl;

                std::cout << "\t -b bitrate : V4L2 capture bitrate kbps(default " << bitrate << " kbps)" << std::endl;
                std::cout << "\t -g gop     : V4L2 capture gop (default " << gop << " )" << std::endl;
                std::cout << "\t device     : V4L2 capture device (default " << dev_name << ")" << std::endl;

                std::cout << "\t V4L2 MJPEG options :" << std::endl;
                std::cout << "\t -j mjpeg_qp : MJPEG streaming and qp (default is 60)" << std::endl;

#if AUDIO_STREAM
                std::cout << "\t -a         : enable A-law pcm streaming "											 << std::endl;
                std::cout << "\t H264 example : "<< argv[0] << " -a -Q 5 -u media/stream1 -P 554"                       << std::endl;
#else
                std::cout << "\t H264 example : " << argv[0] << " -Q 5 -u media/stream1 -P 554" << std::endl;
#endif
                std::cout << "\t MJPEG example : " << argv[0] << " -W 640 -H 480 -j 120 -Q 5 -u media/stream1 -P 554"
                          << std::endl;
                exit(0);
            }
        }
    }
    if (optind < argc) {
        dev_name = argv[optind];
    }

    // create live555 environment
    scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

    // create RTSP server
    rtspServer = RTSPServer::createNew(*env, rtspPort);
    if (rtspServer == NULL) {
        //LOG(ERROR) << "Failed to create RTSP server: " << env->getResultMsg();
        fprintf(stderr, "Failed to create RTSP server: %s \n", env->getResultMsg());
    } else {
        // set http tunneling
        if (rtspOverHTTPPort) {
            rtspServer->setUpTunnelingOverHTTP(rtspOverHTTPPort);
        }

        // Init capture
        //LOG(NOTICE) << "Create V4L2 Source..." << dev_name;
        fprintf(stderr, "create Video source = %s \n", dev_name);


#if AUDIO_STREAM

#endif

#if AUDIO_STREAM
        /*
            Start Audio Device

        */

#endif
        /* Determind which Class to use */
        if (format == V4L2_PIX_FMT_H264)
            videoES = H264_V4L2DeviceSource::createNew(*env, 0, useThread);
        else {
            impParams params;
            params.width = width;
            params.height = height;
            params.mode = IMP_MODE_JPEG;
            params.framerate = fps;
            params.nightvision = false;
            //videoES = ImpJpegVideoDeviceSource::createNew(*env, params);


            //videoES = V4L2DeviceSource::createNew(*env, 0, useThread);
            //mjpeg_qp
        }

        /*  check if create a Device source success */
        if (videoES != NULL) {
            //LOG(FATAL) << "Unable to create source for device " << dev_name;
            fprintf(stderr, "Unable to create source for device  %s \n", dev_name);
        } else {

            //videoCapture->devicesource = videoES;

            // Setup the outpacket size;
            if (m2m_en) {
                //OutPacketBuffer::maxSize = (unsigned int)videoCapture->m2m->isp_buffers->length;
                OutPacketBuffer::maxSize = bitrate << 8;    //2X Bitrate as the max packet size
            } else {

                OutPacketBuffer::maxSize = width * height * 3 / 2;
            }

#if AUDIO_STREAM
            /*
                create Alsa Device source Class
            */

#endif

            replicator = StreamReplicator::createNew(*env, videoES, false);

#if AUDIO_STREAM

#endif
            // Create Server Multicast Session
            if (multicast) {
                ServerMediaSubsession *multicast_video_subSession = NULL;
                ServerMediaSubsession *multicast_audio_subSession = NULL;
                if (maddr == INADDR_NONE) maddr = chooseRandomIPv4SSMAddress(*env);
                destinationAddress.s_addr = maddr;
                //LOG(NOTICE) << "Mutlicast address " << inet_ntoa(destinationAddress);
                fprintf(stderr, "Mutlicast address  %s \n", inet_ntoa(destinationAddress));


                multicast_video_subSession = MulticastServerMediaSubsession::createNew(*env, destinationAddress,
                                                                                       Port(rtpPortNum),
                                                                                       Port(rtcpPortNum), ttl,
                                                                                       replicator, format);
#if AUDIO_STREAM

#endif
                addSession(rtspServer, murl.c_str(), multicast_video_subSession, multicast_audio_subSession);

            }

            ServerMediaSubsession *video_subSession = NULL;
            ServerMediaSubsession *audio_subSession = NULL;

            video_subSession = UnicastServerMediaSubsession::createNew(*env, replicator, format);

#if AUDIO_STREAM

#endif
            // Create Server Unicast Session
            addSession(rtspServer, url.c_str(), video_subSession, audio_subSession);

            // main loop
            signal(SIGINT, sighandler);
            env->taskScheduler().doEventLoop(&quit);

            fprintf(stderr, "Exiting....  \n");

#if AUDIO_STREAM
            if (audioES)
            {
                Medium::close(audioES);
            }
#endif
            Medium::close(videoES);
        }
#if AUDIO_STREAM

#endif


    }
    Medium::close(rtspServer);


    env->reclaim();
    delete scheduler;

    return 0;
}



