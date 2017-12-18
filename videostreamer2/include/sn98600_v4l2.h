#ifndef V4L2_H_
#define V4L2_H_

#include "snx_lib.h"
#include "isp_lib_api.h"
#include "snx_vc_lib.h"
#include "snx_rc_lib.h"

#include "V4l2DeviceSource.h"

typedef void (*sn98600_cb)(struct snx_v4l2_video *video,
		const struct timeval *tstamp, void *data, size_t len,
		int keyframe);
//		void *cbarg);

struct snx_v4l2_video { 
	char *filename;
	struct snx_m2m *m2m;
	struct snx_rc *rate_ctl;

	//enum RESOLUTION_TYPE resolution_type;

	unsigned long long start_t;
	unsigned long long end_t;
	int real_bps;	// calculator bps
	int real_fps; // calculator fps

	int started;
	pthread_t thread_id;

	sn98600_cb cb;
	void *cbarg;

	V4L2DeviceSource * devicesource;

};

void * snx98600_video_read(void *arg);
int snx98600_video_start(struct snx_v4l2_video *video);
int snx98600_video_stop(struct snx_v4l2_video *video);
int snx98600_video_open(snx_v4l2_video *video, void *cbarg);
struct snx_v4l2_video * snx98600_video_new(void);
int snx98600_video_free(struct snx_v4l2_video *video);



#endif /* V4L2_H_ */
