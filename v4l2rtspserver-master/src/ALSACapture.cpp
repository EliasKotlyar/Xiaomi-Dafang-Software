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
    if (::ioctl(fd, SNDCTL_DSP_STEREO, &stereo)==-1)
    { /* Fatal error */
       LOG(ERROR) << "Cant set Mono ..." << params.m_devName;
    }


    int speed = params.m_sampleRate;




    if (ioctl(fd, SNDCTL_DSP_SPEED, &speed)==-1)
    { /* Fatal error */
        LOG(ERROR) << "Cant set Speed ..." << params.m_devName;
    }


    // Lame Init:
    gfp = lame_init();
    int ret_code = lame_init_params(gfp);
    if (ret_code < 0)
    { /* Fatal error */
        LOG(ERROR) << "Cant init Lame";
    }






    /*
    snd_pcm_hw_params_t *hw_params = NULL;
    int err = 0;

    // open PCM device
    if ((err = snd_pcm_open (&m_pcm, m_params.m_devName.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        LOG(ERROR) << "cannot open audio device: " << m_params.m_devName << " error:" <<  snd_strerror (err);
    }

    // configure hw_params
    else if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        LOG(ERROR) << "cannot allocate hardware parameter structure device: " << m_params.m_devName << " error:" <<  snd_strerror (err);
        this->close();
    }
    else if ((err = snd_pcm_hw_params_any (m_pcm, hw_params)) < 0) {
        LOG(ERROR) << "cannot initialize hardware parameter structure device: " << m_params.m_devName << " error:" <<  snd_strerror (err);
        this->close();
    }
    else if ((err = snd_pcm_hw_params_set_access (m_pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        LOG(ERROR) << "cannot set access type device: " << m_params.m_devName << " error:" <<  snd_strerror (err);
        this->close();
    }
    else if ((err = snd_pcm_hw_params_set_format (m_pcm, hw_params, m_params.m_fmt)) < 0) {
        LOG(ERROR) << "cannot set sample format device: " << m_params.m_devName << " error:" <<  snd_strerror (err);
        this->close();
    }
    else if ((err = snd_pcm_hw_params_set_rate_near (m_pcm, hw_params, &m_params.m_sampleRate, 0)) < 0) {
        LOG(ERROR) << "cannot set sample rate device: " << m_params.m_devName << " error:" <<  snd_strerror (err);
        this->close();
    }
    else if ((err = snd_pcm_hw_params_set_channels (m_pcm, hw_params, m_params.m_channels)) < 0) {
        LOG(ERROR) << "cannot set channel count device: " << m_params.m_devName << " error:" <<  snd_strerror (err);
        this->close();
    }
    else if ((err = snd_pcm_hw_params (m_pcm, hw_params)) < 0) {
        LOG(ERROR) << "cannot set parameters device: " << m_params.m_devName << " error:" <<  snd_strerror (err);
        this->close();
    }

    // get buffer size
    else if ((err = snd_pcm_get_params(m_pcm, &m_bufferSize, &m_periodSize)) < 0) {
        LOG(ERROR) << "cannot get parameters device: " << m_params.m_devName << " error:" <<  snd_strerror (err);
        this->close();
    }

    // start capture
    else if ((err = snd_pcm_prepare (m_pcm)) < 0) {
        LOG(ERROR) << "cannot prepare audio interface for use device: " << m_params.m_devName << " error:" <<  snd_strerror (err);
        this->close();
    }
    else if ((err = snd_pcm_start (m_pcm)) < 0) {
        LOG(ERROR) << "cannot start audio interface for use device: " << m_params.m_devName << " error:" <<  snd_strerror (err);
        this->close();
    }

    LOG(NOTICE) << "ALSA device: \"" << m_params.m_devName << "\" buffer_size:" << m_bufferSize << " period_size:" << m_periodSize << " rate:" << m_params.m_sampleRate;
    */
}

size_t ALSACapture::read(char* buffer, size_t bufferSize)
{
    int num_samples = bufferSize / sizeof(short);
    short localBuffer[ num_samples ];
    int bytesRead = ::read (fd, &localBuffer, bufferSize);
    bytesRead = lame_encode_buffer( gfp,         localBuffer, localBuffer,  num_samples,(unsigned char*)buffer,bufferSize);
    return bytesRead;



/*
    size_t size = 0;
    int fmt_phys_width_bytes = 0;
    if (m_pcm != 0)
    {
        int fmt_phys_width_bits = snd_pcm_format_physical_width(m_params.m_fmt);
        fmt_phys_width_bytes = fmt_phys_width_bits / 8;

        snd_pcm_sframes_t ret = snd_pcm_readi (m_pcm, buffer, m_periodSize*fmt_phys_width_bytes);
        LOG(DEBUG) << "ALSA buffer in_size:" << m_periodSize*fmt_phys_width_bytes << " read_size:" << ret;
        if (ret > 0) {
            size = ret;

            // swap if capture in not in network order
            if (!snd_pcm_format_big_endian(m_params.m_fmt)) {
                for(unsigned int i = 0; i < size; i++){
                    char * ptr = &buffer[i * fmt_phys_width_bytes * m_params.m_channels];

                    for(unsigned int j = 0; j < m_params.m_channels; j++){
                        ptr += j * fmt_phys_width_bytes;
                        for (int k = 0; k < fmt_phys_width_bytes/2; k++) {
                            char byte = ptr[k];
                            ptr[k] = ptr[fmt_phys_width_bytes - 1 - k];
                            ptr[fmt_phys_width_bytes - 1 - k] = byte;
                        }
                    }
                }
            }
        }
    }
    return size * m_params.m_channels * fmt_phys_width_bytes;
    */
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


