/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** ALSACapture.cpp
** 
** V4L2 RTSP streamer                                                                 
**                                                                                    
** ALSA capture overide of V4l2Capture
**                                                                                    
** -------------------------------------------------------------------------*/

#ifdef HAVE_ALSA

#include "ALSACapture.h"
#include "soundcard.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>


ALSACapture* ALSACapture::createNew(const ALSACaptureParameters & params) 
{ 
    ALSACapture* capture = new ALSACapture(params);
    if (capture)
    {
        if (capture->getFd() == -1)
        {
            delete capture;
            capture = NULL;
        }
    }
    return capture;
}

ALSACapture::~ALSACapture()
{
    this->close();
}

void ALSACapture::close()
{

}

ALSACapture::ALSACapture(const ALSACaptureParameters & params) : m_bufferSize(0), m_periodSize(0), m_params(params)
{
    // Taken from : http://www.4front-tech.com/pguide/audio.html#channels

    LOG(NOTICE) << "Open ALSA device: \"" << params.m_devName << "\"";


    if ((fd = ::open(params.m_devName.c_str(), O_RDONLY, 0)) == -1)
    {
        LOG(ERROR) << "cannot open audio device: " << params.m_devName;
    }


    int format;

    format = AFMT_S16_LE;
    if (::ioctl(fd, SNDCTL_DSP_SETFMT, &format)==-1)
    { /* Fatal error */
        LOG(ERROR) << "Cant set format..." << params.m_devName;
    }
    int stereo = params.m_channels-1;
    LOG(NOTICE) << "Channel Count:" << params.m_channels;
    if (::ioctl(fd, SNDCTL_DSP_STEREO, &stereo)==-1)
    { /* Fatal error */
       LOG(ERROR) << "Cant set Mono/Stereo ..." << params.m_devName;
    }


    int speed = params.m_sampleRate;




    if (ioctl(fd, SNDCTL_DSP_SPEED, &speed)==-1)
    { /* Fatal error */
        LOG(ERROR) << "Cant set Speed ..." << params.m_devName;
    }


    // Lame Init:
    gfp = lame_init();
    lame_set_num_channels(gfp, 1);
    //lame_set_mode(gfp, 3);

    int ret_code = lame_init_params(gfp);
    if (ret_code < 0)
    { /* Fatal error */
        LOG(ERROR) << "Cant init Lame";
    }
    lame_print_config(gfp);


}

size_t ALSACapture::read(char* buffer, size_t bufferSize)
{
    int num_samples = bufferSize / sizeof(short);
    short localBuffer[ num_samples ];
    int bytesRead = ::read (fd, &localBuffer, bufferSize);

    num_samples = bytesRead / sizeof(short);
    int mp3buf_size = 1.25*num_samples + 7200;

    bytesRead = lame_encode_buffer( gfp,         localBuffer, NULL,  num_samples,(unsigned char*)buffer,mp3buf_size);
    //LOG(ERROR) << "Bytes Converted to MP3:" << bytesRead;
    if(bytesRead == 0){
        LOG(ERROR) << "Error converting to MP3";
        //LOG(ERROR) << "Buffersize " << bufferSize;
        bytesRead = 1;
    }
    return bytesRead;


}

int ALSACapture::getFd()
{
/*
    unsigned int nbfs = 1;
    struct pollfd pfds[nbfs];
    pfds[0].fd = -1;

    if (m_pcm != 0)
    {
        int count = snd_pcm_poll_descriptors_count (m_pcm);
        int err = snd_pcm_poll_descriptors(m_pcm, pfds, count);
        if (err < 0) {
            fprintf (stderr, "cannot snd_pcm_poll_descriptors (%s)\n", snd_strerror (err));
        }
    }
    return pfds[0].fd;
*/
return fd;
}

#endif


