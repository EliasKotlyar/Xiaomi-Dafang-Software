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


extern "C" {
#include <wave.h>
#include <noise_remover.h>
#include "filt.h"
}


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
    m_sharedMem = SharedMem::instance();
    m_sharedMem.readConfig();
    m_newConfig = m_sharedMem.getConfig();

    memcpy(&m_currentConfig, m_newConfig, sizeof(shared_conf));
    // Taken from : http://www.4front-tech.com/pguide/audio.html#channels

    LOG(NOTICE) << "Open ALSA device: \"" << params.m_devName << "\"";


    if ((fd = ::open(params.m_devName.c_str(), O_RDONLY, 0)) == -1)
    {
        LOG(ERROR) << "cannot open audio device: " << params.m_devName;
    }

    int format= AFMT_S16_LE;
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

    int speed =  params.m_inSampleRate;

    if (ioctl(fd, SNDCTL_DSP_SPEED, &speed)==-1)
    { /* Fatal error */
        LOG(ERROR) << "Cant set Speed ..." << params.m_devName;
    }
   // int vol = params.m_volume;
    if (m_newConfig->volume != -1)
    {
        if (ioctl(fd, SNDCTL_EXT_SET_RECORD_VOLUME, &m_newConfig->volume)==-1)
        { /* Fatal error */
            LOG(ERROR) << "Cant set vol" << m_newConfig->volume;
        }
    }

    switch (params.m_encode)
    {
    case ENCODE_OPUS:
    {
#define APPLICATION OPUS_APPLICATION_VOIP
       int err =0;
        /*Create a new encoder state */
       encoder = opus_encoder_create(params.m_inSampleRate, params.m_channels, APPLICATION, &err);

       if (err<0)
       {
          fprintf(stderr, "failed to create an encoder: %s\n", opus_strerror(err));
          exit(0);
       }

       /* Set the desired bit-rate. You can also set other parameters if needed.
          The Opus library is designed to have good defaults, so only set
          parameters you know you need. Doing otherwise is likely to result
          in worse quality, but better. */

       err = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(params.m_outSampleRate));
       err = opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));

       if (err<0)
       {
          fprintf(stderr, "failed to set bitrate: %s\n", opus_strerror(err));
          exit(0);
       }
       opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(0));
       break;
      }
    case ENCODE_MP3:
    {
        // Lame Init:
        gfp = lame_init();
        lame_set_num_channels(gfp,params.m_channels );
        //lame_set_mode(gfp, 3);
        lame_set_in_samplerate(gfp, params.m_inSampleRate);
        lame_set_out_samplerate(gfp, params.m_outSampleRate);

        int ret_code = lame_init_params(gfp);
        if (ret_code < 0)
        { /* Fatal error */
            LOG(ERROR) << "Cant init Lame";
        }
        lame_print_config(gfp);
        break;
    }
    case ENCODE_PCM:
    default:
        break;
    }



}

#define _SWAP(val) (val << 8) | ((val >> 8) & 0xFF);
inline signed short lowpass(signed short input, bool swap)
{
   static signed short last_sample=0;
   signed short retvalue=(input + (last_sample * 7)) >> 3;
   last_sample=retvalue;
   if (swap)
    return _SWAP(retvalue);
   return retvalue;
}
static float clampf(float v, float min, float max){
    return v < min ? min : (v > max ? max : v);
}

inline short ALSACapture::filter(short val,bool swap, int num_sample)
{
 //   int method = 2;
    if (m_Filtermethod == 1)
    {
         static struct noise_remover_s nrm;
         static bool isInit = false;
         if (isInit == false) {
            noise_remover_init( &nrm );
            isInit = true;
         }

        short y;
        /* process audio */
        val = ASHIFT16(val,-2);

        y = noise_remover ( &nrm, val, 1 );  /* training=1 */

        if( y>8192 ) // 8192 = ASHIFT16( 32768, -2 )
            y = 32767;
        else if( y<-8192 )
            y = -32768;
        else
            y = ASHIFT16(y,+2);

        return lowpass(y, swap);
    }

    if (m_Filtermethod == 2)
    {
        return lowpass(val, swap);
    }

    if (m_Filtermethod==3)
    {
       static bool isInit = false;
       static Filter * my_filter = NULL;
       if (isInit == false) {
            my_filter = new Filter(LPF, num_sample,((float)m_params.m_inSampleRate)/1000.0 , 1.0);
//            my_filter = new Filter(LPF, num_sample,((float)m_params.m_inSampleRate)/1000.0 , 3.0, 6.0);

         //   my_filter = new Filter(HPF, num_sample,44.1, 3.0);//((float)m_params.m_inSampleRate)/1000.0 , 3.0);
            isInit = true;
        }

        double res = my_filter->do_sample( (double) val );
       // printf("in=%f out %f\n", (double)val, res);
        //return (unsigned short) my_filter->do_sample( (double) val );
        return (short)res;
    }
    if (swap == true)
        return _SWAP(val);
    return val;
}

size_t ALSACapture::read(char* buffer, size_t bufferSize)
{
    m_sharedMem.readConfig();
    if (m_currentConfig.volume != m_newConfig->volume) {
        if (ioctl(fd, SNDCTL_EXT_SET_RECORD_VOLUME, &m_newConfig->volume)==-1)
        { /* Fatal error */
            LOG(ERROR) << "Cant set vol" << m_newConfig->volume;
        }
        m_currentConfig.volume = m_newConfig->volume;
    }
    m_Filtermethod = m_newConfig->filter;
    switch (m_params.m_encode)
    {
        case ENCODE_OPUS:
            return readOpus(buffer, bufferSize);
            break;
        case ENCODE_MP3:
            return readMP3(buffer, bufferSize);
            break;
        case ENCODE_PCM:
            return readPCM(buffer, bufferSize);
            break;

    }
    return 0;
}

size_t ALSACapture::readPCM(char* buffer, size_t bufferSize)
{
    int num_samples = bufferSize / sizeof(short);
    short localBuffer[ num_samples ];
    // Read 10 packets of 20ms
    int bytesRead = ::read (fd, &localBuffer, (m_params.m_inSampleRate*0.02)*sizeof(short)*10);
    num_samples = bytesRead / sizeof(short);

    for (int i =0; i<  num_samples ; i++)
    {
        ((signed short*)buffer)[i] = filter(((signed short*)localBuffer)[i], true, num_samples);
    }

    return num_samples*2;
}

size_t ALSACapture::readOpus(char* buffer, size_t bufferSize)
{
    int num_samples = bufferSize / sizeof(short);
    short localBuffer[ num_samples ];
    int bytesRead = ::read (fd, &localBuffer,(m_params.m_inSampleRate*2 /50)*6);
    num_samples = bytesRead / sizeof(short);


    for (int i =0; i<  num_samples ; i++)
    {
        ((signed short*)localBuffer)[i] = filter(((signed short*)localBuffer)[i], false, num_samples);
    }


     /* Encode the frame. */
     bytesRead = opus_encode(encoder, localBuffer, num_samples, (unsigned char*)buffer, bufferSize);

      if (bytesRead<0)
      {
            LOG(ERROR) << "Error converting to OPUS " << bytesRead;
            //LOG(ERROR) << "Buffersize " << bufferSize;
            bytesRead = 1;
      }

    return bytesRead;
}

size_t ALSACapture::readMP3(char* buffer, size_t bufferSize)
{
    int num_samples = bufferSize / sizeof(short);
    short localBuffer[ num_samples ];
    int mp3buf_size = 1.25*num_samples + 7200;
    int bytesRead = ::read (fd, &localBuffer, bufferSize);
    num_samples = bytesRead / sizeof(short);

    for (int i =0; i<  num_samples ; i++)
    {
        ((signed short*)localBuffer)[i] = filter(((signed short*)localBuffer)[i], false, num_samples);
    }

    bytesRead = lame_encode_buffer( gfp, localBuffer, NULL,  num_samples,(unsigned char*)buffer,mp3buf_size);
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
    return fd;
}


#endif


