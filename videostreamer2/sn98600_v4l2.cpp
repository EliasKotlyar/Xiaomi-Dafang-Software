/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** SONIX SN986 Series Middleware video Wrapper to Call V4L2 Libraries
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
#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>

#include <linux/videodev2.h>


#include "sn98600_v4l2.h"



void * snx98600_video_read(void *arg)
{
	struct snx_v4l2_video *video = (snx_v4l2_video *)arg;
	struct snx_m2m *m2m = video->m2m;
	struct snx_rc *rate_ctl = video->rate_ctl;
	struct timeval tv;
	int ret;
	int fps_debug= 1;
	int keyFrame = 0;
//	int md_recover;
	if(m2m->m2m) {
		snx_isp_start(m2m);
		// enable drc
		system("echo 0x1 > /proc/isp/drc/enable");
	}

	if (fps_debug != 0) {
		gettimeofday(&tv,NULL);
		video->start_t = tv.tv_sec * 1000000 + tv.tv_usec;
	}
	
	ret = snx_codec_start(m2m);
	if(ret != 0) goto finally;

	video->real_fps=0;
	video->real_bps=0;
	video->started = 1;
	
	printf("H264 thread %dx%d \n", m2m->width, m2m->height);

	while(1) {
		ret = snx_codec_read(m2m);       
		if(m2m->cap_bytesused != 0) {
			gettimeofday(&tv,NULL);
			video->end_t = tv.tv_sec * 1000000 + tv.tv_usec;
			video->real_fps++;
			video->real_bps += m2m->cap_bytesused;


			if(m2m->codec_fmt == V4L2_PIX_FMT_H264) {

				m2m->qp = snx_codec_rc_update(m2m, rate_ctl);
				// index, v, h, count
				snx_md_drop_fps(rate_ctl, &m2m->force_i_frame);
				
				if(video->m2m->flags & 0x8) {
					keyFrame =1;
				}
				else
					keyFrame =0;
				
			}

			if (video->cb) {
				//printf("dispatching to %p: %d bytes"
				//		, video->cb, m2m->cap_bytesused);
				(*video->cb)(video, &(m2m->timestamp), m2m->cap_buffers[m2m->cap_index].start,
						m2m->cap_bytesused, keyFrame);
			}

			// For RTSP device source (H264 / MJPEG)
			if (video->devicesource) {
				//printf("dispatching to %p: %d bytes"
				//		, video->cb, m2m->cap_bytesused);
				video->devicesource->videocallback(&(m2m->timestamp), m2m->cap_buffers[m2m->cap_index].start,
						m2m->cap_bytesused, keyFrame);
			}

		}
		ret = snx_codec_reset(m2m);
//		if(iSignalCount || (video->started == 0)) {
		if(video->started == 0) {
			break;
		}
	} // while(1)

	ret = 0;
finally:
	snx_codec_stop(m2m);
	snx_codec_uninit(m2m);
	if(m2m->m2m) {
		ret = snx_isp_stop(m2m);
		ret = snx_isp_uninit(m2m);

	}
	close(m2m->codec_fd);
	if(m2m->m2m)
		close(m2m->isp_fd);

	pthread_exit((void *)ret);
	
}

int snx98600_video_start(struct snx_v4l2_video *video)
{
	int ret =0;
	ret = pthread_create(&(video->thread_id), NULL, snx98600_video_read, (void *)video);
	if(ret != 0) {
		printf("Can't create thread: %s\n", strerror(ret));
		goto finally;
	}

finally:
	return ret;
}

int snx98600_video_stop(struct snx_v4l2_video *video)
{
	int ret=0;
	struct snx_m2m *m2m = video->m2m;
	void *thread_result;


	if (!video) {
		ret = EINVAL;
		printf("null argument");
		goto finally;
	}

	if (m2m->codec_fd < 0) {
		ret = EPERM;
		printf("not open");
		goto finally;
	}

	if (!video->started) {
		ret = -1;
		printf("not started");
		goto finally;
	}

	printf("stopping: %s \n", video->filename);
	video->started = 0;
	ret = pthread_join(video->thread_id, &thread_result);
	if(ret != 0)
		printf("thread join failed ret==%d\n", ret);

	ret = (int)thread_result;

	ret = 0;

finally:
	return ret;

}

int snx98600_video_open(snx_v4l2_video *video, void *cbarg)
{
	int ret = 0;
	int bitrate;
	struct snx_m2m *m2m = video->m2m;
	struct snx_rc *rate_ctl = video->rate_ctl;


//	m2m->m2m = 1;
	m2m->scale = 1;
//	m2m->isp_fps = 30;
//	m2m->codec_fps = 15;
	if (m2m->isp_fps == 0)
		m2m->isp_fps = 30;

	if (m2m->codec_fps == 0)
		m2m->codec_fps = 15;
	

	m2m->m2m_buffers = 2;

	//format set by main.cpp
	//m2m->codec_fmt = V4L2_PIX_FMT_H264;
	if (m2m->codec_fmt == 0)
		m2m->codec_fmt = V4L2_PIX_FMT_H264;

	m2m->out_mem = V4L2_MEMORY_USERPTR;
	m2m->ds_font_num = 128;
	m2m->dyn_fps_en = 0;			//haoweilo

	m2m->isp_mem = V4L2_MEMORY_MMAP;

	if (m2m->m2m) {

		strcpy(m2m->isp_dev,IMG_DEV_NAME);
		m2m->isp_fmt = V4L2_PIX_FMT_SNX420;
	}

#if 0
	m2m->width = 640;
	m2m->height = 480;

	if (video->resolution_type == RESOLUTION_VGA_264) {
		//strcpy(video->m2m->codec_dev, CAP1_DEV_NAME);
		m2m->width = 640;
		m2m->height = 360;
	}
	else if (video->resolution_type == RESOLUTION_HD) {
		//strcpy(video->m2m->codec_dev, CAP_DEV_NAME);
		m2m->width = 1280;
		m2m->height = 720;
	}
	else if (video->resolution_type == RESOLUTION_HD_MJ) {
		int fps = 5;
		//snx_get_file_value(SNAPSHOT_FPS, &fps, 10);
		//strcpy(video->m2m->codec_dev, CAP_DEV_NAME);
		m2m->width = 1280;
		m2m->height = 720;
		m2m->codec_fmt = V4L2_PIX_FMT_MJPEG;
		m2m->m2m = 0;
		m2m->scale = 1;
		m2m->codec_fps = fps;
	}
#else

	if (m2m->codec_fmt == V4L2_PIX_FMT_MJPEG) {
		//snx_get_file_value(SNAPSHOT_FPS, &fps, 10);
		//strcpy(video->m2m->codec_dev, CAP_DEV_NAME);
		//m2m->m2m = 0;
		m2m->scale = 1;
	}

	// width and height are set in main.cpp
	// if zero back to default width and height
	if ((m2m->width == 0) || (m2m->height == 0)) {

		m2m->width = 1280;
		m2m->height = 720;
	}

#endif

	// if fps_ctrl = 0: isp drop frame
	// if fps_ctrl = 1: use AE exposure time to output average fps
	//system("echo 0 > /proc/isp/ae/fps_ctrl");

	if(m2m->m2m) {
		m2m->isp_fd = snx_open_device(m2m->isp_dev);	// Open output device
		ret = snx_isp_init(m2m);
		if(ret != 0) goto finally;
	}
	// Codec init
	m2m->codec_fd = snx_open_device(m2m->codec_dev);
	ret = snx_codec_init(m2m);
	if(ret != 0) goto finally;

	//if (video->resolution_type == RESOLUTION_HD_MJ) {
	if (m2m->codec_fmt == V4L2_PIX_FMT_MJPEG) {
		if (m2m->qp == 0)
			m2m->qp = 60;
		snx_codec_set_qp(m2m, V4L2_CID_JPEG_COMPRESSION_QUALITY);

	} else {
		rate_ctl->width = m2m->width;
		rate_ctl->height = m2m->height;
		rate_ctl->codec_fd = m2m->codec_fd;
		rate_ctl->Targetbitrate = m2m->bit_rate;
		rate_ctl->framerate = m2m->codec_fps;
		rate_ctl->gop = m2m->gop;
		//rate_ctl->reinit= 1;
		/*Initialize rate control */
		m2m->qp = snx_codec_rc_init(rate_ctl, SNX_RC_INIT);
	}

	ret = 0;

finally:
	if (ret != 0) {
		if (video->filename) {
			free(video->filename);
			video->filename = NULL;
		}

		if (m2m->codec_fd >= 0) {
			close(m2m->codec_fd);
			m2m->codec_fd = -1;
		}
	}
	return ret;
}

static int snx98600_video_close(struct snx_v4l2_video *video)
{
	int ret = 0;
	struct snx_m2m *m2m = video->m2m;
	
	if (m2m->codec_fd < 0) {
		ret = EPERM;
		printf("not open");
		goto finally;
	}

	if (video->started) {
		if ((ret = snx98600_video_stop(video))) {
			printf("failed to stop '%s': %s", video->filename, strerror(ret));
			goto finally;
		}
	}
	
	//printf("closing");
 
	if (video->filename) {
		free(video->filename);
		video->filename = NULL;
	}
	
finally:
	return ret;
}

struct snx_v4l2_video * snx98600_video_new(void)
{
	int ret = 0;
	struct snx_v4l2_video *v4l2_video = NULL;

	if (!(v4l2_video = (struct snx_v4l2_video *)calloc(1, sizeof(struct snx_v4l2_video)))) {
		ret = errno ? errno : -1;
		printf("calloc: %s", strerror(ret));
		goto finally;
	}

	if (!(v4l2_video->m2m = (struct snx_m2m *)calloc(1, sizeof(struct snx_m2m)))) {
		ret = errno ? errno : -1;
		free(v4l2_video);
		printf("calloc: %s", strerror(ret));
		goto finally;
	}

	if (!(v4l2_video->rate_ctl = (struct snx_rc *)calloc(1, sizeof(struct snx_rc)))) {
		ret = errno ? errno : -1;
		free(v4l2_video->m2m);
		free(v4l2_video);
		printf("calloc: %s", strerror(ret));
		goto finally;
	}

	v4l2_video->devicesource = NULL;

finally:
	
	return v4l2_video;
}

int snx98600_video_free(struct snx_v4l2_video *video)
{
	int ret=0;
	struct snx_m2m *m2m = video->m2m;
	struct snx_rc *rate_ctl = video->rate_ctl;

	if(!video) {
		ret = EINVAL;
		goto finally;
	}
	
	ret = snx98600_video_close(video);
	if(ret != 0) {
		goto finally;
	}

	if(rate_ctl) {
		free(rate_ctl);
		rate_ctl = NULL;
	}

	if(m2m) {
		free(m2m);
		m2m = NULL;
	}

	if(video) {
		free(video);
		video = NULL;
	}
		
	ret = 0;
finally:
	return ret;
}
