#ifndef SN98600_AUDIO_H_
#define SN98600_AUDIO_H_

#include "AlsaDeviceSource.h"

#ifdef __cplusplus
extern "C" {
#endif


#include <alsa/asoundlib.h>



struct sonix_audio;

typedef void (*sonix_audio_cb)(struct sonix_audio *audio,
		const struct timeval *tstamp, void *data, size_t len,
		void *cbarg);

enum sonix_audio_type { AUDIO_IN, AUDIO_OUT};

struct sonix_audio {
	enum sonix_audio_type type;		//>> audio direction
	char *filename;					//>> audio device name

	sonix_audio_cb cb;				//>> audio record callback
	void *cbarg;

	/*! PCM related */
	snd_pcm_t *pcm;		
	char *codec_pcm_buf;			//>> pcm data address
	int codec_pcm_buf_len;			//>> pcm data len
	int codec_pcm_data_len;			//>> require pcm data len

	/*! reserved */
	int started;

	AlsaDeviceSource * devicesource;
};

#define OATD_BUF_NUM 25
#define __ERR_MSG(fmt, args...) fprintf(stderr, "sonix audio --- %s:" fmt, __FUNCTION__, ## args)

#ifdef __cplusplus
}
#endif

#endif /* SN98600_AUDIO_H_ */
