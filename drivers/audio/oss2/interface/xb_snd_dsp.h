/**
 * xb_snd_dsp.h
 *
 * jbbi <jbbi@ingenic.cn>
 *
 * 24 APR 2012
 *
 */

#ifndef __XB_SND_DSP_H__
#define __XB_SND_DSP_H__

#include <linux/dmaengine.h>
#include <linux/wait.h>
#include <linux/scatterlist.h>
#include <linux/spinlock.h>
#include <linux/poll.h>
#include <mach/jzdma.h>
#include <mach/jzsnd.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/soundcard.h>

/*####################################################*\
 * sound pipe and command used for dsp device
\*####################################################*/

enum snd_debug_enum{
	SND_DEBUG_LEVEL,
	SND_WARN_LEVEL,
	SND_ERROR_LEVEL,
};

#define snd_print(level,fmt,...)	\
		do{		\
			if(level >= snd_debug_level)	\
				printk("%s[line:%d]"fmt,__func__,__LINE__,##__VA_ARGS__); \
		}while(0)

#define snd_debug_print(fmt, ...)	snd_print(SND_DEBUG_LEVEL, fmt, ##__VA_ARGS__)
#define snd_warn_print(fmt, ...)	snd_print(SND_WARN_LEVEL, fmt, ##__VA_ARGS__)
#define snd_error_print(fmt, ...)	snd_print(SND_ERROR_LEVEL, fmt, ##__VA_ARGS__)


#define DEFAULT_REPLAY_SAMPLERATE	44100
#define DEFAULT_RECORD_SAMPLERATE	44100
#define DEFAULT_REPLAY_CHANNEL		1
#define DEFAULT_RECORD_CHANNEL		1
/**
 * sound device
 **/
enum snd_device_t {
	SND_DEVICE_DEFAULT = 0,

	SND_DEVICE_CURRENT,
	SND_DEVICE_HANDSET,
	SND_DEVICE_HEADSET,	//fix for mid pretest
	SND_DEVICE_SPEAKER,	//fix for mid pretest
	SND_DEVICE_HEADPHONE,

	SND_DEVICE_BT,
	SND_DEVICE_BT_EC_OFF,
	SND_DEVICE_HEADSET_AND_SPEAKER,
	SND_DEVICE_TTY_FULL,
	SND_DEVICE_CARKIT,

	SND_DEVICE_FM_SPEAKER,
	SND_DEVICE_FM_HEADSET,
	SND_DEVICE_BUILDIN_MIC,
	SND_DEVICE_HEADSET_MIC,
	SND_DEVICE_HDMI = 15,	//fix for mid pretest

	SND_DEVICE_LOOP_TEST = 16,

	SND_DEVICE_CALL_START,							//call route start mark
	SND_DEVICE_CALL_HEADPHONE = SND_DEVICE_CALL_START,
	SND_DEVICE_CALL_HEADSET,
	SND_DEVICE_CALL_HANDSET,
	SND_DEVICE_CALL_SPEAKER,
	SND_DEVICE_HEADSET_RECORD_INCALL,
	SND_DEVICE_BUILDIN_RECORD_INCALL,
	SND_DEVICE_CALL_END = SND_DEVICE_BUILDIN_RECORD_INCALL,	//call route end mark

	SND_DEVICE_LINEIN_RECORD,
	SND_DEVICE_LINEIN1_RECORD,
	SND_DEVICE_LINEIN2_RECORD,
	SND_DEVICE_LINEIN3_RECORD,

	SND_DEVICE_COUNT
};
/**
 * extern ioctl command for dsp device
 **/
struct audio_ouput_stream {
	void __user * data;
	unsigned int size;
};

struct audio_input_stream {
	void __user *data;
	void __user *aec;
	unsigned int size;
};

#define SNDCTL_EXT_SYNC_STREAM        		_SIOR ('P', 107, int)
#define SNDCTL_EXT_FLUSH_STREAM        		_SIOR ('P', 106, int)
#define SNDCTL_EXT_SET_AO_STREAM        	_SIOR ('P', 105, struct audio_ouput_stream)
#define SNDCTL_EXT_GET_AI_STREAM        	_SIOR ('P', 104, struct audio_input_stream)
#define SNDCTL_EXT_DISABLE_STREAM        	_SIOR ('P', 103, int)
#define SNDCTL_EXT_ENABLE_STREAM        	_SIOR ('P', 102, int)
#define SNDCTL_EXT_ENABLE_AEC        		_SIOR ('P', 101, int)
#define SNDCTL_EXT_DISABLE_AEC        		_SIOR ('P', 100, int)

#define SNDCTL_EXT_SET_RECORD_VOLUME        _SIOR ('P', 91, int)
#define SNDCTL_EXT_SET_REPLAY_VOLUME        _SIOR ('P', 90, int)
/**
 * dsp device control command
 **/
enum snd_dsp_command {
	/**
	 * the command flowed is used to enable/disable
	 * replay/record.
	 **/
	SND_DSP_ENABLE_REPLAY=0,
	SND_DSP_DISABLE_REPLAY,
	SND_DSP_ENABLE_RECORD,
	SND_DSP_DISABLE_RECORD,
	/**
	 * the command flowed is used to enable/disable the
	 * dma transfer on device.
	 **/
	SND_DSP_ENABLE_DMA_RX,
	SND_DSP_DISABLE_DMA_RX,
	SND_DSP_ENABLE_DMA_TX,
	SND_DSP_DISABLE_DMA_TX,
	/**
	 *@SND_DSP_SET_XXXX_RATE is used to set replay/record rate
	 *@SND_DSP_GET_XXXX_RATE is used to get current samplerate
	 **/
	SND_DSP_SET_REPLAY_RATE,
	SND_DSP_SET_RECORD_RATE,
	SND_DSP_GET_REPLAY_RATE, /*10*/
	SND_DSP_GET_RECORD_RATE,
	/**
	 * @SND_DSP_SET_XXXX_CHANNELS is used to set replay/record
	 * channels, when channels changed, filter maybe also need
	 * changed to a fix value.
	 * @SND_DSP_GET_XXXX_CHANNELS is used to get current channels
	 **/
	SND_DSP_SET_REPLAY_CHANNELS,
	SND_DSP_SET_RECORD_CHANNELS,
	SND_DSP_GET_REPLAY_CHANNELS,
	SND_DSP_GET_RECORD_CHANNELS,
	/**
	 * @SND_DSP_GET_XXXX_FMT_CAP is used to get formats that
	 * replay/record supports.
	 * @SND_DSP_GET_XXXX_FMT used to get current replay/record
	 * format.
	 * @SND_DSP_SET_XXXX_FMT is used to set replay/record format,
	 * if the format changed, trigger,dma max_tsz and filter maybe
	 * also need changed to a fix value. and let them effect.
	 **/
	SND_DSP_GET_REPLAY_FMT_CAP,
	SND_DSP_GET_REPLAY_FMT,
	SND_DSP_SET_REPLAY_FMT,
	SND_DSP_GET_RECORD_FMT_CAP,
	SND_DSP_GET_RECORD_FMT,	/*20*/
	SND_DSP_SET_RECORD_FMT,

	SND_DSP_GET_RECORD_FRAGMENTSIZE,
	SND_DSP_GET_REPLAY_FRAGMENTSIZE,
	/**
	 * @SND_DSP_SET_DEVICE is used to set audio route
	 * @SND_DSP_SET_STANDBY used to set into/release from stanby
	 **/
	SND_DSP_SET_DEVICE,
	SND_DSP_SET_STANDBY,
	/**
	 *	@SND_MIXER_DUMP_REG dump aic register and codec register val
	 *	@SND_MIXER_DUMP_GPIO dump hp mute gpio ...
	 **/
	SND_MIXER_DUMP_REG,
	SND_MIXER_DUMP_GPIO,
	/**
	 *	@SND_DSP_GET_HP_DETECT dump_hp_detect state
	 **/
	SND_DSP_GET_HP_DETECT,
	/**
	 *	@SND_DSP_SET_RECORD_VOL set adc vol
	 *	@SND_DSP_SET_MIC_VOL	set input vol
	 *	@SND_DSP_SET_REPLAY_VOL set dac vol
	 **/
	SND_DSP_SET_RECORD_VOL,
	SND_DSP_SET_MIC_VOL,
	SND_DSP_SET_REPLAY_VOL,
	/**
	 *	@SND_DSP_SET_DMIC_TRIGGER_MODE set dmic trigger
	 **/
	SND_DSP_SET_DMIC_TRIGGER_MODE, /*30*/
	/**
	 * @SND_DSP_CLR_ROUTE  for pretest
	 **/
	SND_DSP_CLR_ROUTE,

	SND_DSP_DEBUG,
	SND_DSP_SET_VOICE_TRIGGER,
	/**
	 *	@SND_DSP_FLUSH_SYNC, wait for ioctl work done.
	 * */
	SND_DSP_FLUSH_SYNC,	/*34*/
	/**
	 * @SND_DSP_RESUME_PROCEDURE, called by driver, resume.
	 */
	SND_DSP_RESUME_PROCEDURE,
};

/**
 *fragsize, must be dived by PAGE_SIZE
 **/
#define FRAGSIZE_S  (PAGE_SIZE >> 1)
#define FRAGSIZE_M  (PAGE_SIZE)
#define FRAGSIZE_L  (PAGE_SIZE << 1)

#define FRAGCNT_S   2
#define FRAGCNT_M   4
#define FRAGCNT_L   8
#define FRAGCNT_B   16

enum task_node_flags {
	SND_TASK_NODE_PREPARED,
	SND_TASK_NODE_FINISHED,
};

enum snd_aec_ops_flags {
	SND_AEC_OFF,
	SND_AEC_ON,
};

#define SND_TASK_NODE_SIZE 4096
struct task_node {
	struct list_head    list;
	struct dsp_route_object *object;
	char 				*data;
	char				*aec;
	size_t				size;
	unsigned char 		flags;
};

/* the size of one data fragment is 10ms */
struct dsp_data_fragment {
	struct list_head	list;
	unsigned int		index;
	unsigned long long	counter;
	/*struct timespec		ts;*/
	/*int					fd;*/
	void 				*vaddr;
	dma_addr_t          paddr;
	unsigned int 		size;
};

typedef enum {
	SND_DSP_STATE_CLOSE,
	SND_DSP_STATE_STANDBY = SND_DSP_STATE_CLOSE,
	SND_DSP_STATE_OPEN,
	SND_DSP_STATE_RUN,
} dsp_state_t;

struct dsp_route_object {
	struct file *fd;
	struct dsp_pipe *dp;
	dsp_state_t	state;
	/* some statistical information */
	unsigned long long ioctrl_counter;
	unsigned long long iodata_counter;
	/* The member only is valid when the pipe record audio. */
	volatile unsigned char aec_enable;
	unsigned int rate;
	unsigned int channel;
	unsigned int format;
	struct task_node datanode;
	char 	*buffer;
	char	*aecbuffer;
};

struct dsp_route_attr {
	unsigned int rate;
	unsigned int channel;
	unsigned int format;
};

#define SND_DSP_DMA_BUFFER_SIZE (96000*2) // The case is that rate is 96K and format is 16bit mono in a second.
#define SND_DSP_DMA_FRAGMENT_MAX_CNT 100	// The max time is 1s.
#define SND_DSP_DMA_FRAGMENT_MIN_CNT 20		// The min time is 200ms.
#define SND_DSP_PIPE_OBJECT_CNT 16
#define SND_DSP_PIPE_DAM2IO_CNT 5
struct dsp_pipe {
	dsp_state_t pipe_state;
	struct dsp_endpoints *dsp;

	/* dma */
	struct dma_chan     *dma_chan;
	struct dma_slave_config dma_config;     /* define by device */
	enum jzdma_type     dma_type;
	struct completion	dma_completion;
	volatile bool       is_trans;				/* the monitor of dma */

	/* driver buffer */
	void				*vaddr;
	dma_addr_t          paddr;
	unsigned int 		reservesize;

	/* buffer manager */
	struct dsp_data_fragment *fragments;
	unsigned int 		fragment_size;
	unsigned int 		fragment_cnt;
	unsigned int 		buffersize;
	/*unsigned int 		fragment_originalsize;*/
	/*unsigned int 		fragment_alignedsize;*/
	struct list_head    fragments_head;
	/*struct dsp_data_fragment    *dma_frag;*/
	/*struct dsp_data_fragment    *io_frag;*/
	unsigned int	dma_tracer;	/* It's offset in buffer */
	unsigned int	io_tracer;	/* It's offset in buffer */

	/* aec buffer */
	struct dsp_data_fragment *aecfragments;
	struct list_head    aecfragments_head;
	void 		*aec_buffer;	/* It is malloced when the pipe record audio */
	unsigned int aec_tracer;	/* It's offset in aec buffer when the pipe record audio */
	unsigned int aec_io;	/* It's offset in aec buffer when the pipe record audio */

	/* tastlist */
	struct list_head	tasklist;

	/* The member only is valid when the pipe record audio. */
	volatile unsigned char aec_enable;

	/**/
	struct dsp_route_object objects[SND_DSP_PIPE_OBJECT_CNT];	// The max times is 16 that the device can be opend.
	char *taskbuffer;
	char *taskaecbuffer;
	unsigned int taskbuffersize;
	struct dsp_route_attr living_attr;
	wait_queue_head_t   wq;

	/* callback funs */
	void (*handle)(struct dsp_pipe *endpoint); /* define by device */
	/* @filter()
	 * buff :		data buffer
	 * cnt :		avialable data size
	 * needed_size:	we need data size
	 *
	 * return covert data size
	 */
	int (*filter)(void *buff, int *cnt, int needed_size);        /* define by device */
	/* lock */
	spinlock_t          pipe_lock;
	struct snd_dev_data *	pddata;
	struct mutex        mutex;

	/* flush stream */
	unsigned char flush_enable;
	struct completion	flush_completion;
};

struct dsp_endpoints {
	dsp_state_t state;
	struct hrtimer hr_timer;
	ktime_t expires;
	atomic_t	timer_stopped;
	struct work_struct workqueue;

	struct mutex        mutex;
    struct dsp_pipe *out_endpoint;
    struct dsp_pipe *in_endpoint;

	/* debug node */
	struct proc_dir_entry *proc;
};

/**
 * filter
 **/
int convert_8bits_signed2unsigned(void *buffer, int *counter,int needed_size);
int convert_8bits_stereo2mono(void *buff, int *data_len,int needed_size);
int convert_8bits_stereo2mono_signed2unsigned(void *buff, int *data_len,int needed_size);
int convert_16bits_stereo2mono(void *buff, int *data_len,int needed_size);
int convert_16bits_stereo2mono_inno(void *buff, int *data_len,int needed_size);
int convert_16bits_stereomix2mono(void *buff, int *data_len,int needed_size);
int convert_32bits_stereo2_16bits_mono(void *buff, int *data_len, int needed_size);
int convert_32bits_2_20bits_tri_mode(void *buff, int *data_len, int needed_size);

/**
 * functions interface
 **/
loff_t xb_snd_dsp_llseek(struct file *file,
						 loff_t offset,
						 int origin,
						 struct snd_dev_data *ddata);

ssize_t xb_snd_dsp_read(struct file *file,
						char __user *buffer,
						size_t count,
						loff_t *ppos,
						struct snd_dev_data *ddata);

ssize_t xb_snd_dsp_write(struct file *file,
						 const char __user *buffer,
						 size_t count,
						 loff_t *ppos,
						 struct snd_dev_data *ddata);

unsigned int xb_snd_dsp_poll(struct file *file,
							   poll_table *wait,
							   struct snd_dev_data *ddata);

long xb_snd_dsp_ioctl(struct file *file,
					  unsigned int cmd,
					  unsigned long arg,
					  struct snd_dev_data *ddata);

int xb_snd_dsp_mmap(struct file *file,
					struct vm_area_struct *vma,
					struct snd_dev_data *ddata);

int xb_snd_dsp_open(struct inode *inode,
					struct file *file,
					struct snd_dev_data *ddata);

int xb_snd_dsp_release(struct inode *inode,
					   struct file *file,
					   struct snd_dev_data *ddata);

int xb_snd_dsp_probe(struct snd_dev_data *ddata);
int xb_snd_dsp_remove(struct snd_dev_data *ddata);

int xb_snd_dsp_suspend(struct snd_dev_data *ddata);
int xb_snd_dsp_resume(struct snd_dev_data *ddata);
#endif //__XB_SND_DSP_H__
