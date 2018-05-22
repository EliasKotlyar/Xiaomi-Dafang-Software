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

#include <loguru.hpp>

/**
 * lame_error_callback, lame_message_callback, lame_debug_callback: LAME
 * logging callback functions.
 *
 * [Parameters]
 *     format: Format string.
 *     args: Format arguments.
 */
static void lame_error_callback(const char *format, va_list args)
{

    LOG_F(ERROR,format, args);
}

static void lame_message_callback(const char *format, va_list args)
{
    LOG_F(INFO,format, args);
}





extern "C" {
#include <wave.h>
#include <noise_remover.h>
#include "filt.h"
}
/* Tested configuration
+--------------+---------------+--------------+--------------+--------------+--------------+
|      Audio   |    inSample   |   outSample  |    Filter    |   S/W Vol    |   H/W Vol    |
+--------------+---------------+--------------+--------------+--------------+--------------+
|      MP3     |    8000       |     8000     |     1,2      |    yes       |    yes       |
|              |    8000       |     44100    |     1,2      |    yes       |    yes       |
|              |    44100      |     44100    |     1,2      |    yes       |    yes       |
+--------------+---------------+--------------+--------------+--------------+--------------+
|      OPUS    |    8000       |     48000    |     1,2      |    yes       |    yes       |
|              |    48000      |     48000    |     1,2      |    yes       |    yes       |
+--------------+---------------+--------------+--------------+--------------+--------------+
|      PCM     |    8000       |      8000    |     1,2      |    yes       |    yes       |
+--------------+---------------+--------------+--------------+--------------+--------------+
|      PCMU    |    8000       |      8000    |     1,2      |    yes       |    yes       |
+--------------+---------------+--------------+--------------+--------------+--------------+
*/
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

    LOG_F(INFO, "Open ALSA device: %s", params.m_devName.c_str() );


    if ((fd = ::open(params.m_devName.c_str(), O_RDONLY, 0)) == -1)
    {
        LOG_F(ERROR,"cannot open audio device: %s", params.m_devName.c_str());
    }

    int format= AFMT_S16_LE;
    if (::ioctl(fd, SNDCTL_DSP_SETFMT, &format)==-1)
    { /* Fatal error */
         LOG_F(ERROR,"Cant set format...%s", params.m_devName.c_str());
    }

    int stereo = params.m_channels-1;
    LOG_F(INFO,"Channel Count: %d", params.m_channels);
    if (::ioctl(fd, SNDCTL_DSP_STEREO, &stereo)==-1)
    { /* Fatal error */
       LOG_F(ERROR,"Cant set Mono/Stereo ...%s", params.m_devName.c_str());
    }

    int speed =  params.m_inSampleRate;

    if (ioctl(fd, SNDCTL_DSP_SPEED, &speed)==-1)
    { /* Fatal error */
         LOG_F(ERROR, "Cant set Speed ...%s",params.m_devName.c_str());
    }
   // int vol = params.m_volume;
    if (m_newConfig->hardVolume != -1)
    {
        if (ioctl(fd, SNDCTL_EXT_SET_RECORD_VOLUME, &m_newConfig->hardVolume)==-1)
        { /* Fatal error */
            LOG_F(ERROR, "Cant set vol %d", m_newConfig->hardVolume);
        }
        m_currentConfig.hardVolume = m_newConfig->hardVolume;
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
          LOG_F(ERROR, "failed to create an encoder: %s", opus_strerror(err));
       }

       /* Set the desired bit-rate. You can also set other parameters if needed.
          The Opus library is designed to have good defaults, so only set
          parameters you know you need. Doing otherwise is likely to result
          in worse quality, but better. */

       opus_encoder_ctl(encoder, OPUS_SET_BITRATE(params.m_outSampleRate));
       opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(0));
       break;
      }
    case ENCODE_MP3:
    {
        // Lame Init:
        gfp = lame_init();
        lame_set_errorf(gfp, lame_error_callback);
    lame_set_errorf(gfp, lame_error_callback);

        lame_set_msgf  (gfp, lame_message_callback);
    //lame_set_debugf(lame, lame_debug_callback);
        lame_set_num_channels(gfp,params.m_channels );
    lame_set_msgf  (gfp, lame_message_callback);
    //lame_set_debugf(lame, lame_debug_callback);

        //lame_set_mode(gfp, 3);
        lame_set_in_samplerate(gfp, params.m_inSampleRate);
        lame_set_out_samplerate(gfp, params.m_outSampleRate);
      //  lame_set_scale(gfp, 3.0);

        int ret_code = lame_init_params(gfp);
        if (ret_code < 0)
        { /* Fatal error */
         LOG_F(ERROR,"Cant init Lame");
        }
        lame_print_config(gfp);
        break;
    }
    case ENCODE_PCM:
    default:
        break;
    }



}

void ALSACapture::setSwVolume(short &val, int vol)
{
 val += (short) (val * (vol / 100.0));
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

inline short ALSACapture::filter(short val,bool swap, int num_sample)
{
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

 // TODO: this filter seems not to work, need to check the input values ...
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
    if (m_currentConfig.hardVolume != m_newConfig->hardVolume) {
        if (ioctl(fd, SNDCTL_EXT_SET_RECORD_VOLUME, &m_newConfig->hardVolume)==-1)
        { /* Fatal error */
            LOG_F(ERROR, "Cant set vol %d", m_newConfig->hardVolume);
        }
        LOG_F(INFO, "Set H/Wvol %d", m_newConfig->hardVolume);
        m_currentConfig.hardVolume = m_newConfig->hardVolume;
    }
    m_Filtermethod = m_newConfig->filter;


    switch (m_params.m_encode)
    {
        case ENCODE_OPUS:
            return readOpus(buffer, bufferSize, m_newConfig->softVolume);
            break;
        case ENCODE_MP3:
            return readMP3(buffer, bufferSize, m_newConfig->softVolume);
            break;
        case ENCODE_PCM:
            return readPCM(buffer, bufferSize, m_newConfig->softVolume);
            break;
        case ENCODE_ULAW:
            return readULAW(buffer, bufferSize, m_newConfig->softVolume);
            break;

    }
    return 0;
}

unsigned long ALSACapture::getBufferSize()
{
  switch (m_params.m_encode)
    {
        case ENCODE_OPUS:
            return (m_params.m_inSampleRate*2 /50)*6;
            break;
        case ENCODE_MP3:
            return m_params.m_inSampleRate * sizeof(short) * 1;
            break;
        case ENCODE_PCM:
            return (m_params.m_inSampleRate*0.02)*sizeof(short)*10;
            break;
        case ENCODE_ULAW:
            return (m_params.m_inSampleRate*0.02)*sizeof(short)*10;
            break;
    }
    return 1;
}

size_t ALSACapture::readULAW(char* buffer, size_t bufferSize, int volume)
{
    int num_samples = bufferSize / sizeof(short);
    short localBuffer[ num_samples ];
    // Read 10 packets of 20ms
    int bytesRead = ::read (fd, &localBuffer, bufferSize); //(m_params.m_inSampleRate*0.02)*sizeof(short)*10);
    num_samples = bytesRead / sizeof(short);

    for (int i =0; i<  num_samples ; i++)
    {
        if (volume != -1) setSwVolume(((signed short*)localBuffer)[i], volume);
        buffer[i] = ulaw_encode(filter(((signed short*)localBuffer)[i], false, num_samples));
    }

    return num_samples;
}

size_t ALSACapture::readPCM(char* buffer, size_t bufferSize, int volume)
{
    int num_samples = bufferSize / sizeof(short);
    short localBuffer[ num_samples ];
    // Read 10 packets of 20ms
    int bytesRead = ::read (fd, &localBuffer, bufferSize); //(m_params.m_inSampleRate*0.02)*sizeof(short)*10);
    num_samples = bytesRead / sizeof(short);

    for (int i =0; i<  num_samples ; i++)
    {
        if (volume != -1) setSwVolume(((signed short*)localBuffer)[i], volume);
        ((signed short*)buffer)[i] = filter(((signed short*)localBuffer)[i], true, num_samples);
    }

    return num_samples*2;
}

size_t ALSACapture::readOpus(char* buffer, size_t bufferSize, int volume)
{
    int num_samples = bufferSize / sizeof(short);
    short localBuffer[ num_samples ];
    int bytesRead = ::read (fd, &localBuffer,bufferSize); //(m_params.m_inSampleRate*2 /50)*6);
    num_samples = bytesRead / sizeof(short);


    for (int i =0; i<  num_samples ; i++)
    {
        if (volume != -1) setSwVolume(((signed short*)localBuffer)[i], volume);
        ((signed short*)localBuffer)[i] = filter(((signed short*)localBuffer)[i], false, num_samples);
    }


     /* Encode the frame. */
     bytesRead = opus_encode(encoder, localBuffer, num_samples, (unsigned char*)buffer, bufferSize);

      if (bytesRead<0)
      {
            LOG_F(ERROR, "Error converting to OPUS %d",bytesRead);
            //LOG(ERROR) << "Buffersize " << bufferSize;
            bytesRead = 1;
      }

    return bytesRead;
}

size_t ALSACapture::readMP3(char* buffer, size_t bufferSize, int volume)
{
    int num_samples = bufferSize / sizeof(short);
    short localBuffer[ num_samples ];
    int mp3buf_size = 1.25*num_samples + 7200;

    int bytesRead = ::read (fd, &localBuffer, bufferSize); //m_params.m_inSampleRate * sizeof(short) * 1); //8192*2);
    num_samples = bytesRead / sizeof(short);

    for (int i =0; i<  num_samples ; i++)
    {
        if (volume != -1) setSwVolume(((signed short*)localBuffer)[i], volume);
        ((signed short*)localBuffer)[i] = filter(((signed short*)localBuffer)[i], false, num_samples);
    }

    bytesRead = lame_encode_buffer( gfp, localBuffer, NULL,  num_samples,(unsigned char*)buffer,mp3buf_size);
    //LOG(ERROR) << "Bytes Converted to MP3:" << bytesRead;
    if(bytesRead == 0){
         LOG_F(ERROR,"Error converting to MP3");
        //LOG(ERROR) << "Buffersize " << bufferSize;
        bytesRead = 1;
    }
    return bytesRead;

}

int ALSACapture::getFd()
{
    return fd;
}

/*=================================================================================
**	The following routines came from the sox-12.15 (Sound eXcahcnge) distribution.
**
**	This code is not compiled into libsndfile. It is only used to test the
**	libsndfile lookup tables for correctness.
**
**	I have included the original authors comments.
*/

/*
** This routine converts from linear to ulaw.
**
** Craig Reese: IDA/Supercomputing Research Center
** Joe Campbell: Department of Defense
** 29 September 1989
**
** References:
** 1) CCITT Recommendation G.711  (very difficult to follow)
** 2) "A New Digital Technique for Implementation of Any
**     Continuous PCM Companding Law," Villeret, Michel,
**     et al. 1973 IEEE Int. Conf. on Communications, Vol 1,
**     1973, pg. 11.12-11.17
** 3) MIL-STD-188-113,"Interoperability and Performance Standards
**     for Analog-to_Digital Conversion Techniques,"
**     17 February 1987
**
** Input: Signed 16 bit linear sample
** Output: 8 bit ulaw sample
*/

#define uBIAS 0x84		/* define the add-in bias for 16 bit.frames */

#define uCLIP 32635

unsigned char ALSACapture::ulaw_encode (short sample)
{	static int exp_lut [256] =
	{	0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
		} ;

	int sign, exponent, mantissa ;
	unsigned char ulawbyte ;

	/* Get the sample into sign-magnitude. */
	sign = (sample >> 8) & 0x80 ;					/* set aside the sign */
	if (sign != 0)
		sample = -sample ;							/* get magnitude */
	if (sample > uCLIP)
		sample = uCLIP ;							/* clip the magnitude */

	/* Convert from 16 bit linear to ulaw. */
	sample = sample + uBIAS ;
	exponent = exp_lut [(sample >> 7) & 0xFF] ;
	mantissa = (sample >> (exponent + 3)) & 0x0F ;
	ulawbyte = ~ (sign | (exponent << 4) | mantissa) ;

	return ulawbyte ;
} /* ulaw_encode */

#endif


