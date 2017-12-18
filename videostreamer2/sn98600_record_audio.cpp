/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** Alsa wrapper
** 
**
** -------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/ioctl.h>

//#include <snx_sdk_conf.h>
#include "sn98600_record_audio.h"
#include "snx_lib.h"

static int thread_enable, thread_exit;
static pthread_mutex_t thread_lock;
static pthread_cond_t thread_cond;
static pthread_t record_thread_id;




static int snx98600_record_audio_open(struct sonix_audio *audio, const char *filename)
{
	int rc = 0;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t buffer_size;
	/* open device */
	if (!(audio->filename = strdup(filename)))
	{
		rc = errno ? errno : -1;
		__ERR_MSG("strdup: %s\n", strerror(rc));
		goto finally1;
	}
	
	/*! open device */
	if(audio->type == AUDIO_IN) { /*! cap device */
		rc = snd_pcm_open(&audio->pcm, audio->filename, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK );
		if(rc < 0) {
			printf("open cap %s failed!!\n", audio->filename);
			goto finally2;
		}
	}
	else { /*! pb device */
		rc = snd_pcm_open(&audio->pcm, audio->filename, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
		if (rc < 0) {  
			printf("open pb %s failed!! %s\n", audio->filename, snd_strerror(rc));
	        goto finally2;
	    }  
	}

	/*! hw param setup */
	snd_pcm_hw_params_alloca(&params);		
	snd_pcm_hw_params_any(audio->pcm, params);		
	snd_pcm_hw_params_set_access(audio->pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED );
	snd_pcm_hw_params_set_format(audio->pcm, params,  SND_PCM_FORMAT_A_LAW);
	snd_pcm_hw_params_set_channels(audio->pcm, params, 1 );		
	snd_pcm_hw_params_set_rate(audio->pcm, params, SAMPLE_RATE, 0 );
	snd_pcm_hw_params_set_period_size(audio->pcm, params, READ_BYTE * 8 / FORMAT_BIT, 0 );
	
	//haoweilo
	buffer_size = MAX_FRAME_NUM;
	if (snd_pcm_hw_params_set_buffer_size_near(audio->pcm, params, &buffer_size) < 0) {
		printf("set pcm hw params buffer size error\n");
		goto finally3;
	}

	rc = snd_pcm_hw_params(audio->pcm, params);
	if (rc < 0) {
		printf("set pcm hw params error !!\n");
		goto finally3;
	}

#if 1	//haowei
	{
		snd_pcm_uframes_t frames_info;
		printf("----------- ALSA HW -----------\n");
		snd_pcm_hw_params_get_period_size(params, &frames_info, 0 );
		printf("period size: %d\n", (int)frames_info);
		snd_pcm_hw_params_get_buffer_size(params, &frames_info );
		printf("buffer size: %d\n", (int)frames_info);
	}

#endif

	audio->codec_pcm_buf_len = READ_BYTE;
	audio->codec_pcm_buf = (char *)memalign(2, audio->codec_pcm_buf_len);
	if(audio->codec_pcm_buf == NULL)
	{
		__ERR_MSG("allocate the buffer of codec for pcm failed\n");
		rc = -2;
		goto finally4;
	}
	audio->codec_pcm_data_len = 0;
	return rc;
	
finally4:
	if(audio->codec_pcm_buf)
		free(audio->codec_pcm_buf);
finally3:
	if (audio->pcm)
		snd_pcm_close(audio->pcm);
finally2:
	if (audio->filename)
		free(audio->filename);
finally1:
	return rc;

}

static int snx98600_record_audio_close(struct sonix_audio *audio)
{
	int rc = 0;
	if(audio->codec_pcm_buf)	{
		free(audio->codec_pcm_buf);
		audio->codec_pcm_buf = NULL;
	}
	if(audio->pcm){
		snd_pcm_close(audio->pcm);
		audio->pcm = NULL;
	}

	if (audio->filename){		
		free(audio->filename);
		audio->filename = NULL;
	}
	return rc;
}

int snx98600_record_audio_start(struct sonix_audio *audio)
{
	int rc = 0;

	if (!audio) {
		rc = EINVAL;
		__ERR_MSG("null argument\n");
		goto finally;
	}

	if (audio->pcm == NULL) {
		rc = EPERM;
		__ERR_MSG("not open\n");
		goto finally;
	}

	/* ISCM will call twice start, add return to avoid exception */
	if (audio->started) {
		rc = EPERM;
		__ERR_MSG("already started\n");
		return 0;
	}
	

	__ERR_MSG("start audio input\n");
	pthread_mutex_lock(&thread_lock);
	thread_enable = 1;
	/* start detection action */
	pthread_cond_signal(&thread_cond);
	pthread_mutex_unlock(&thread_lock);

	audio->started = 1;

finally:
	if (rc != 0) {
		if (audio->pcm) {
			snx98600_record_audio_close(audio);
		}
	}
	return 0;
}

int snx98600_record_audio_stop(struct sonix_audio *audio)
{
	int rc;

	if (!audio) {
		rc = EINVAL;
		__ERR_MSG("null argument\n");
		goto finally;
	}

	if (audio->pcm == NULL) {
		rc = EPERM;
		__ERR_MSG("not open\n");
		goto finally;
	}

	if (!audio->started) {
		rc = EPERM;
		__ERR_MSG("already stopped\n");
		return 0;
	}

	__ERR_MSG("stop audio input\n");
	pthread_mutex_lock(&thread_lock);
	thread_enable = 0;
	pthread_mutex_unlock(&thread_lock);

	audio->started = 0;
	
finally:
	return 0;
}

static int snx98600_record_audio_destory_thread(struct sonix_audio *audio)
{

	/* terminate detection thread */
	thread_exit = 1;

	pthread_mutex_lock(&thread_lock);
	pthread_cond_signal(&thread_cond);
	pthread_mutex_unlock(&thread_lock);
	pthread_join(record_thread_id, NULL );
	pthread_mutex_destroy(&thread_lock);
	pthread_cond_destroy(&thread_cond);

	usleep(300 * 1000);
	return 0;
}

void snx98600_record_audio_free(struct sonix_audio *audio)
{
	if (!audio) 
	{
		__ERR_MSG("audio is empty\n");
		return;
	}
	snx98600_record_audio_destory_thread(audio);

	if (audio->pcm != NULL) 
	{
		snx98600_record_audio_close(audio);
	}

	free(audio);
	audio = NULL;
}

static void *audio_record_thr_func(void *arg)
{
	int rc;
	unsigned int frame_cnt;
	unsigned char *pdata;
	struct sonix_audio *audio = (struct sonix_audio *)arg;
	struct timeval tv;

	while (!thread_exit) 
	{
		pthread_mutex_lock(&thread_lock);
		if (!thread_enable) 
		{
			snd_pcm_drop(audio->pcm);
			__ERR_MSG("audio thr stop\n");
			pthread_cond_wait(&thread_cond, &thread_lock);
			__ERR_MSG("audio thr rec sig\n");
			if (thread_exit)
			{
				pthread_mutex_unlock(&thread_lock);
				break;
			}
			__ERR_MSG("audio thr start\n");
			snd_pcm_prepare(audio->pcm);
		}
		
		pthread_mutex_unlock(&thread_lock);
		
		if ((rc = gettimeofday(&tv, NULL))) 
		{
			rc = errno ? errno : -1;
			__ERR_MSG("failed to create timestamp: %s\n", strerror(rc));
			continue;
		}

		pdata = (unsigned char *)audio->codec_pcm_buf;
		frame_cnt = READ_BYTE * 8 / FORMAT_BIT;

		//fprintf(stderr, "frame_cnt =%d \n", frame_cnt);
		
		while(frame_cnt > 0)
		{
			rc = snd_pcm_readi(audio->pcm, pdata, frame_cnt);
			if(rc <= 0)
			{
				if(rc == -EPIPE)
				{
					__ERR_MSG("Audio record buffer overrun!\n");
					snd_pcm_prepare(audio->pcm);

				}
				else if(rc == -EAGAIN)
				{
					usleep(100);
				}
				else
				{
					__ERR_MSG("Audio record error:%s\n", snd_strerror(rc));
				}
			}
			else
			{
				pdata += rc * FORMAT_BIT / 8;
				frame_cnt -= rc;
			}
		}
		if(audio->cb)
			(*audio->cb)(audio, &tv, (void *)audio->codec_pcm_buf, audio->codec_pcm_buf_len, NULL);

		if(audio->devicesource)
			audio->devicesource->audiocallback(&tv, (void *)audio->codec_pcm_buf, audio->codec_pcm_buf_len, NULL);
	}
	
}


static int snx98600_record_audio_create_thread
(
	pthread_t *thread_id,
	void *(*func)(void *), 
	void *arg
)
{
	int rc = 0;
	pthread_attr_t th_attr;

	thread_exit = 0;

	pthread_mutex_init(&thread_lock, NULL );
	pthread_cond_init(&thread_cond, NULL );
	pthread_attr_init(&th_attr);

	/*! set thread stack size 256 KB*/
	if (pthread_attr_setstacksize(&th_attr, 262144)) {
		__ERR_MSG("set thread stack size fail\n");
		goto finally;
	}

	if ((rc = pthread_create(thread_id, &th_attr, func, arg)) != 0) {
		__ERR_MSG("Can't create thread: %s\n", strerror(rc));
		goto finally;
	}
	
finally:
	if(rc) {
		pthread_mutex_destroy(&thread_lock);
		pthread_cond_destroy(&thread_cond);
	}
    pthread_attr_destroy(&th_attr);
    return rc;

}

struct sonix_audio *snx98600_record_audio_new(char *filename, sonix_audio_cb cb, void *cbarg)
{
	int rc = 0;
	struct sonix_audio *audio;
	
	if (!(audio = (struct sonix_audio *)calloc(1, sizeof(struct sonix_audio)))) {
		rc = errno ? errno : -1;
		fprintf(stderr, "calloc: %s\n", strerror(rc));
		goto finally;
	}

	audio->pcm = NULL;
	audio->cb = cb;
	audio->cbarg = cbarg;
	audio->type = AUDIO_IN;

	rc = snx98600_record_audio_open(audio, filename);
	if(rc < 0)
		goto finally;
	
	/* initial audio capture thread */
	thread_exit = 0;
	rc = snx98600_record_audio_create_thread(&record_thread_id, &audio_record_thr_func, (void *)audio);

finally:
	if (rc != 0)
	{
		if (audio)
		{
			snx98600_record_audio_free(audio);
			audio = NULL;
		}
		errno = rc;
	}
	return audio;
}


