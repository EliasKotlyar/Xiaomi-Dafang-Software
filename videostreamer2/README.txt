### About snx_rtsp_server ###

snx_rtsp_server is a small rtsp streaming server developed base on live555 library. snx_rtsp_server supports,

@  Support H264 / MJPEG Video stream
@  Support G.711 Alaw Audio stream
@  UDP over RTP/RTSP 
@  TCP over RTP/RTSP 
@  TCP over RTP/RTSP over HTTP
@  unicast and multicast
@  Single stream support (one video stream + one audio stream) 
@  Frame buffer tunning for streaming server (default is 10 frames buffering (audio/video))
@  customized RTSP port and http port

# advantage

@ low DDR and CPU loading 

# usage


   snx_rtsp_server [-a] [-j mjpeg_qp] [-m] [-P RTSP port][-T RTSP/HTTP port][-Q queueSize] [-M groupaddress]]
         -Q length: Number of frame queue  (default 10)
         RTSP options :
         -u url   : unicast url (default unicast)
         -m url   : multicast url (default multicast)
         -M addr  : multicast group   (default is a random address)
         -P port  : RTSP port (default 8554)
         -T port  : RTSP over HTTP port (default 0)
         V4L2 options :
         -F fps   : V4L2 capture framerate (default 30)
         -W width : V4L2 capture width (default 1280)
         -H height: V4L2 capture height (default 720)
         V4L2 H264 options :
         -b bitrate: V4L2 capture bitrate kbps(default 1024 kbps)
         -g gop   : V4L2 capture gop (default 30 )
         device   : V4L2 capture device (default /dev/video1)
         V4L2 MJPEG options :
         -j  mjpeg_qp     : MJPEG streaming and qp (default is 60)
         -a       : enable A-law pcm streaming 
         H264 example   : /etc/snx_rtsp_server -a -Q 5 -u media/stream1 -P 554
         MJPEG example   : /etc/snx_rtsp_server -W 640 -H 480 -j 60 -Q 5 -u media/stream1 -P 554


Reference: 

   modified from https://github.com/mpromonet/h264_v4l2_rtspserver
   #version c01325cabebcf3b50bb5c39cde7d5777f5f38270

   MJPEG modified from http://stackoverflow.com/questions/12158716/jpeg-streaming-with-live555/20584296#20584296

##############################
