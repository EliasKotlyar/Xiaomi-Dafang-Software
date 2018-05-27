/**
 * xb_snd_dsp.c
 *
 * xhshen <xianghui.shen@ingenic.cn>
 *
 */
#define DEBUG
#include <linux/module.h>
#include <linux/soundcard.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <jz_proc.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include "xb_snd_dsp.h"
#include <asm/mipsregs.h>
#include <asm/io.h>
#include <linux/fs.h>

extern unsigned int snd_debug_level;
#define AUDIO_DRIVER_VERSION "V20-20170615a"

static int cached_fragments = SND_DSP_DMA_FRAGMENT_MAX_CNT;
module_param(cached_fragments, int, S_IRUGO);
MODULE_PARM_DESC(cached_fragments, "AUDIO cached datasize,its unit is 10ms");

/*###########################################################*\
 * sub functions
 \*###########################################################*/
static struct dsp_endpoints * xb_dsp_get_endpoints(struct snd_dev_data *ddata)
{
	return ddata->get_endpoints(ddata);
}

/********************************************************\
 * dma
\********************************************************/

static void snd_reconfig_dma(struct dsp_pipe *dp)
{
	if (dp->dma_chan == NULL)
		return;

	dmaengine_slave_config(dp->dma_chan,&dp->dma_config);
}
static bool dma_chan_filter(struct dma_chan *chan, void *filter_param)
{
	struct dsp_pipe *dp = filter_param;
	return (void*)dp->dma_type == chan->private;
}

static int snd_reuqest_dma(struct dsp_pipe *dp, struct device *dev)
{
	int ret = 0;
	dma_cap_mask_t mask;
	unsigned long lock_flags;
	/* alloc memory */
	dp->reservesize = PAGE_ALIGN(SND_DSP_DMA_BUFFER_SIZE);

	dp->vaddr = dma_alloc_coherent(dev,
							  dp->reservesize,
							  &dp->paddr,
							  GFP_KERNEL | GFP_DMA);

	if (dp->vaddr == NULL){
		snd_error_print("failed to alloc dma buffer\n");
		return -ENOMEM;
	}
	/*memset(dp->vaddr, 0x55, dp->reservesize);*/
	/*dma_sync_single_for_device(NULL, dp->paddr, dp->reservesize, DMA_TO_DEVICE);*/

	/* Try to grab a DMA channel */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	spin_lock_irqsave(&dp->pipe_lock, lock_flags);
	dp->is_trans = false;
	spin_unlock_irqrestore(&dp->pipe_lock, lock_flags);
	dp->dma_chan = dma_request_channel(mask, dma_chan_filter, (void*)dp);

	if (dp->dma_chan == NULL) {
		ret = -ENXIO;
		goto out;
	}
	snd_reconfig_dma(dp);
	snd_debug_print(" init dev = %p size = %d dp->vaddr = %p dp->paddr = 0x%08x\n", dev, dp->reservesize, dp->vaddr, dp->paddr);
	return 0;
out:
	/* free memory */
	if(dp->vaddr){
		dmam_free_noncoherent(dev,
				dp->reservesize,
				dp->vaddr,
				dp->paddr);
	}
	dp->vaddr = NULL;
	return ret;
}

static void snd_release_dma(struct dsp_pipe *dp, struct device *dev)
{
	unsigned long lock_flags;
	if (dp) {
		if (dp->dma_chan) {
			spin_lock_irqsave(&dp->pipe_lock, lock_flags);
			if(dp->is_trans != false){
				dmaengine_terminate_all(dp->dma_chan);
				/*wait_for_completion_interruptible(&dp->dma_completion);*/
				dp->is_trans = false;
			}
			spin_unlock_irqrestore(&dp->pipe_lock, lock_flags);
			/* free memory */
			if(dp->vaddr){
			snd_debug_print(" deinit dev = %p size = %d dp->vaddr = %p dp->paddr = 0x%08x\n", dev, dp->reservesize, dp->vaddr, dp->paddr);
				dma_free_coherent(dev,
						dp->reservesize,
						dp->vaddr,
						dp->paddr);
			}
			dp->vaddr = NULL;
			dma_release_channel(dp->dma_chan);
			dp->dma_chan = NULL;
		}
	}
}

#if 0
static void snd_dma_callback(void *arg)
{
	struct dsp_pipe *dp = (struct dsp_pipe *)arg;
	complete(&dp->dma_completion);
	return;
}
#endif
/********************************************************\
 * filter
\********************************************************/
/*
 * Convert signed byte to unsiged byte
 *
 * Mapping:
 * 	signed		unsigned
 *	0x00 (0)	0x80 (128)
 *	0x01 (1)	0x81 (129)
 *	......		......
 *	0x7f (127)	0xff (255)
 *	0x80 (-128)	0x00 (0)
 *	0x81 (-127)	0x01 (1)
 *	......		......
 *	0xff (-1)	0x7f (127)
 */
int convert_8bits_signed2unsigned(void *buffer, int *counter,int needed_size)
{
	int i;
	int counter_8align = 0;
	unsigned char *ucsrc	= buffer;
	unsigned char *ucdst	= buffer;

	if (needed_size < (*counter)) {
		*counter = needed_size;
	}
	counter_8align = (*counter) & ~0x7;

	for (i = 0; i < counter_8align; i+=8) {
		*(ucdst + i + 0) = *(ucsrc + i + 0) + 0x80;
		*(ucdst + i + 1) = *(ucsrc + i + 1) + 0x80;
		*(ucdst + i + 2) = *(ucsrc + i + 2) + 0x80;
		*(ucdst + i + 3) = *(ucsrc + i + 3) + 0x80;
		*(ucdst + i + 4) = *(ucsrc + i + 4) + 0x80;
		*(ucdst + i + 5) = *(ucsrc + i + 5) + 0x80;
		*(ucdst + i + 6) = *(ucsrc + i + 6) + 0x80;
		*(ucdst + i + 7) = *(ucsrc + i + 7) + 0x80;
	}

	BUG_ON(i != counter_8align);

	for (i = counter_8align; i < *counter; i++) {
		*(ucdst + i) = *(ucsrc + i) + 0x80;
	}

	return *counter;
}

/*
 * Convert stereo data to mono data, data width: 8 bits/channel
 *
 * buff:	buffer address
 * data_len:	data length in kernel space, the length of stereo data
 *		calculated by "node->end - node->start"
 */
int convert_8bits_stereo2mono(void *buff, int *data_len,int needed_size)
{
	/* stride = 16 bytes = 2 channels * 1 byte * 8 pipelines */
	int data_len_16aligned = 0;
	int mono_cur, stereo_cur;
	unsigned char *uc_buff = buff;

	if ((*data_len) > needed_size*2)
		*data_len = needed_size*2;

	*data_len = (*data_len) & (~0x1);

	data_len_16aligned = (*data_len)& ~0xf;

	/* copy 8 times each loop */
	for (stereo_cur = mono_cur = 0;
	     stereo_cur < data_len_16aligned;
	     stereo_cur += 16, mono_cur += 8) {

		uc_buff[mono_cur + 0] = uc_buff[stereo_cur + 0];
		uc_buff[mono_cur + 1] = uc_buff[stereo_cur + 2];
		uc_buff[mono_cur + 2] = uc_buff[stereo_cur + 4];
		uc_buff[mono_cur + 3] = uc_buff[stereo_cur + 6];
		uc_buff[mono_cur + 4] = uc_buff[stereo_cur + 8];
		uc_buff[mono_cur + 5] = uc_buff[stereo_cur + 10];
		uc_buff[mono_cur + 6] = uc_buff[stereo_cur + 12];
		uc_buff[mono_cur + 7] = uc_buff[stereo_cur + 14];
	}

	BUG_ON(stereo_cur != data_len_16aligned);

	/* remaining data */
	for (; stereo_cur < (*data_len); stereo_cur += 2, mono_cur++) {
		uc_buff[mono_cur] = uc_buff[stereo_cur];
	}

	return ((*data_len) >> 1);
}

/*
 * Convert stereo data to mono data, and convert signed byte to unsigned byte.
 *
 * data width: 8 bits/channel
 *
 * buff:	buffer address
 * data_len:	data length in kernel space, the length of stereo data
 *		calculated by "node->end - node->start"
 */
int convert_8bits_stereo2mono_signed2unsigned(void *buff, int *data_len,int needed_size)
{
	/* stride = 16 bytes = 2 channels * 1 byte * 8 pipelines */
	int data_len_16aligned = 0;
	int mono_cur, stereo_cur;
	unsigned char *uc_buff = buff;

	if ((*data_len) > needed_size*2)
		*data_len = needed_size*2;

	*data_len = (*data_len) & (~0x1);

	data_len_16aligned = (*data_len) & ~0xf;

	/* copy 8 times each loop */
	for (stereo_cur = mono_cur = 0;
	     stereo_cur < data_len_16aligned;
	     stereo_cur += 16, mono_cur += 8) {

		uc_buff[mono_cur + 0] = uc_buff[stereo_cur + 0] + 0x80;
		uc_buff[mono_cur + 1] = uc_buff[stereo_cur + 2] + 0x80;
		uc_buff[mono_cur + 2] = uc_buff[stereo_cur + 4] + 0x80;
		uc_buff[mono_cur + 3] = uc_buff[stereo_cur + 6] + 0x80;
		uc_buff[mono_cur + 4] = uc_buff[stereo_cur + 8] + 0x80;
		uc_buff[mono_cur + 5] = uc_buff[stereo_cur + 10] + 0x80;
		uc_buff[mono_cur + 6] = uc_buff[stereo_cur + 12] + 0x80;
		uc_buff[mono_cur + 7] = uc_buff[stereo_cur + 14] + 0x80;
	}

	BUG_ON(stereo_cur != data_len_16aligned);

	/* remaining data */
	for (; stereo_cur < (*data_len); stereo_cur += 2, mono_cur++) {
		uc_buff[mono_cur] = uc_buff[stereo_cur] + 0x80;
	}

	return ((*data_len) >> 1);
}

/*
 * Convert stereo data to mono data, data width: 16 bits/channel
 *
 * buff:	buffer address
 * data_len:	data length in kernel space, the length of stereo data
 *		calculated by "node->end - node->start"
 */
int convert_16bits_stereo2mono(void *buff, int *data_len, int needed_size)
{
	/* stride = 32 bytes = 2 channels * 2 byte * 8 pipelines */
	int data_len_32aligned = 0;
	int data_cnt_ushort = 0;
	int mono_cur, stereo_cur;
	unsigned short *ushort_buff = (unsigned short *)buff;

	if ((*data_len) > needed_size*2)
		*data_len = needed_size*2;

	/*when 16bit format one sample has four bytes
	 *so we can not operat the singular byte*/
	*data_len = (*data_len) & (~0x3);

	data_len_32aligned = (*data_len) & ~0x1f;
	data_cnt_ushort = data_len_32aligned >> 1;

	/* copy 8 times each loop */
	for (stereo_cur = mono_cur = 0;
	     stereo_cur < data_cnt_ushort;
	     stereo_cur += 16, mono_cur += 8) {

		ushort_buff[mono_cur + 0] = ushort_buff[stereo_cur + 0];
		ushort_buff[mono_cur + 1] = ushort_buff[stereo_cur + 2];
		ushort_buff[mono_cur + 2] = ushort_buff[stereo_cur + 4];
		ushort_buff[mono_cur + 3] = ushort_buff[stereo_cur + 6];
		ushort_buff[mono_cur + 4] = ushort_buff[stereo_cur + 8];
		ushort_buff[mono_cur + 5] = ushort_buff[stereo_cur + 10];
		ushort_buff[mono_cur + 6] = ushort_buff[stereo_cur + 12];
		ushort_buff[mono_cur + 7] = ushort_buff[stereo_cur + 14];
	}

	BUG_ON(stereo_cur != data_cnt_ushort);

	/* remaining data */
	for (; stereo_cur < ((*data_len) >> 1); stereo_cur += 2, mono_cur++) {
		ushort_buff[mono_cur] = ushort_buff[stereo_cur];
	}

	return ((*data_len) >> 1);
}

int convert_16bits_stereo2mono_inno(void *buff, int *data_len, int needed_size)
{
	/* stride = 32 bytes = 2 channels * 2 byte * 8 pipelines */
	int data_len_32aligned = 0;
	int data_cnt_ushort = 0;
	int mono_cur, stereo_cur;
	unsigned short *ushort_buff = (unsigned short *)buff;

	if ((*data_len) > needed_size*2)
		*data_len = needed_size*2;

	/*when 16bit format one sample has four bytes
	 *so we can not operat the singular byte*/
	*data_len = (*data_len) & (~0x3);

	data_len_32aligned = (*data_len) & ~0x1f;
	data_cnt_ushort = data_len_32aligned >> 1;

	/* copy 8 times each loop */
	for (stereo_cur = mono_cur = 0;
	     stereo_cur < data_cnt_ushort;
	     stereo_cur += 16, mono_cur += 8) {

		ushort_buff[mono_cur + 0] = ushort_buff[stereo_cur + 0] + ushort_buff[stereo_cur + 1];
		ushort_buff[mono_cur + 1] = ushort_buff[stereo_cur + 2] + ushort_buff[stereo_cur + 3];
		ushort_buff[mono_cur + 2] = ushort_buff[stereo_cur + 4] + ushort_buff[stereo_cur + 5];
		ushort_buff[mono_cur + 3] = ushort_buff[stereo_cur + 6] + ushort_buff[stereo_cur + 7];
		ushort_buff[mono_cur + 4] = ushort_buff[stereo_cur + 8] + ushort_buff[stereo_cur + 9];
		ushort_buff[mono_cur + 5] = ushort_buff[stereo_cur + 10] + ushort_buff[stereo_cur + 11];
		ushort_buff[mono_cur + 6] = ushort_buff[stereo_cur + 12] + ushort_buff[stereo_cur + 13];
		ushort_buff[mono_cur + 7] = ushort_buff[stereo_cur + 14] + ushort_buff[stereo_cur + 15];
	}

	BUG_ON(stereo_cur != data_cnt_ushort);

	/* remaining data */
	for (; stereo_cur < ((*data_len) >> 1); stereo_cur += 2, mono_cur++) {
		ushort_buff[mono_cur] = ushort_buff[stereo_cur] + ushort_buff[stereo_cur +1];
	}

	return ((*data_len) >> 1);
}

/*
 * convert normal 16bit stereo data to mono data
 *
 * buff:	buffer address
 * data_len:	data length in kernel space, the length of stereo data
 *
 */
int convert_16bits_stereomix2mono(void *buff, int *data_len,int needed_size)
{
	/* stride = 32 bytes = 2 channels * 2 byte * 8 pipelines */
	int data_len_32aligned = 0;
	int data_cnt_ushort = 0;
	int left_cur, right_cur, mono_cur;
	short *ushort_buff = (short *)buff;
	/*init*/
	left_cur = 0;
	right_cur = left_cur + 1;
	mono_cur = 0;


	if ( (*data_len) > needed_size*2)
		*data_len = needed_size*2;

	/*when 16bit format one sample has four bytes
	 *so we can not operat the singular byte*/
	*data_len = (*data_len) & (~0x3);

	data_len_32aligned = (*data_len) & (~0x1f);
	data_cnt_ushort = data_len_32aligned >> 1;

	/*because the buff's size is always 4096 bytes,so it will not lost data*/
	while (left_cur < data_cnt_ushort)
	{
		ushort_buff[mono_cur + 0] = ((ushort_buff[left_cur + 0]) + (ushort_buff[right_cur + 0]));
		ushort_buff[mono_cur + 1] = ((ushort_buff[left_cur + 2]) + (ushort_buff[right_cur + 2]));
		ushort_buff[mono_cur + 2] = ((ushort_buff[left_cur + 4]) + (ushort_buff[right_cur + 4]));
		ushort_buff[mono_cur + 3] = ((ushort_buff[left_cur + 6]) + (ushort_buff[right_cur + 6]));
		ushort_buff[mono_cur + 4] = ((ushort_buff[left_cur + 8]) + (ushort_buff[right_cur + 8]));
		ushort_buff[mono_cur + 5] = ((ushort_buff[left_cur + 10]) + (ushort_buff[right_cur + 10]));
		ushort_buff[mono_cur + 6] = ((ushort_buff[left_cur + 12]) + (ushort_buff[right_cur + 12]));
		ushort_buff[mono_cur + 7] = ((ushort_buff[left_cur + 14]) + (ushort_buff[right_cur + 14]));

		left_cur += 16;
		right_cur += 16;
		mono_cur += 8;
	}

	BUG_ON(left_cur != data_cnt_ushort);

	/* remaining data */
	for (;right_cur < ((*data_len) >> 1); left_cur += 2, right_cur += 2)
		ushort_buff[mono_cur++] = ushort_buff[left_cur] + ushort_buff[right_cur];

	return ((*data_len) >> 1);
}

/********************************************************\
 * others
\********************************************************/
static int init_pipe(struct dsp_pipe *dp,struct device *dev,enum dma_data_direction direction)
{
	int ret = 0;

	if (dp == NULL)
		return -ENODEV;

	/* init dsp fragments */
	dp->fragment_cnt = 0;
	dp->buffersize = 0;

	dp->fragments = (struct dsp_data_fragment *)kzalloc(sizeof(struct dsp_data_fragment)*cached_fragments, GFP_KERNEL);
	if(dp->fragments == NULL){
		snd_error_print("failed to alloc dma fragments\n");
		ret = -ENOMEM;
		goto failed_alloc_fragments;
	}
	memset(dp->fragments, 0, sizeof(struct dsp_data_fragment)*cached_fragments);
	ret = snd_reuqest_dma(dp, dev);
	if (ret) {
		snd_error_print("failed to alloc dma channel\n");
		goto failed_request_dma;
	}
	dp->taskbuffersize = SND_DSP_PIPE_OBJECT_CNT*SND_TASK_NODE_SIZE;
	dp->taskbuffer = kmalloc(dp->taskbuffersize, GFP_KERNEL);
	if(!dp->taskbuffer){
		snd_error_print("Failed to alloc task buffer!\n");
		dp->taskbuffersize = 0;
		ret = -ENOMEM;
		goto failed_alloc_taskbuf;
	}
	if (direction == DMA_TO_DEVICE) {
		dp->dma_config.direction = DMA_MEM_TO_DEV;
		dp->aec_buffer = NULL;
		dp->taskaecbuffer = NULL;
	}else if(direction == DMA_FROM_DEVICE){
		dp->dma_config.direction = DMA_DEV_TO_MEM;
		dp->aec_buffer = kzalloc(dp->reservesize, GFP_KERNEL);
		if(dp->aec_buffer == NULL){
			snd_error_print("Failed to alloc aec buffer!\n");
			ret = -ENOMEM;
			goto failed_alloc_aec;
		}
		dp->aecfragments = (struct dsp_data_fragment *)kzalloc(sizeof(struct dsp_data_fragment)*cached_fragments, GFP_KERNEL);
		if(dp->aecfragments == NULL){
			snd_error_print("failed to alloc dma fragments\n");
			ret = -ENOMEM;
			goto failed_alloc_aecfragments;
		}
		memset(dp->aec_buffer, 0 , dp->reservesize);
		dp->taskaecbuffer = kzalloc(dp->taskbuffersize, GFP_KERNEL);
		if(!dp->taskaecbuffer){
			snd_error_print("Failed to alloc task buffer!\n");
			dp->taskbuffersize = 0;
			ret = -ENOMEM;
			goto failed_alloc_taskaecbuf;
		}
	}else{
		snd_error_print("The dma direction is wrong!\n");
	}
	INIT_LIST_HEAD(&dp->fragments_head);
	INIT_LIST_HEAD(&dp->aecfragments_head);
	INIT_LIST_HEAD(&dp->tasklist);
	init_completion(&dp->flush_completion);

	init_waitqueue_head(&dp->wq);
	spin_lock_init(&dp->pipe_lock);
	mutex_init(&dp->mutex);
	dp->pipe_state = SND_DSP_STATE_CLOSE;
	return 0;
failed_alloc_taskaecbuf:
	if(dp->aecfragments)
		kfree(dp->aecfragments);
	dp->aecfragments = NULL;
failed_alloc_aecfragments:
	kfree(dp->aec_buffer);
	dp->aec_buffer = NULL;
failed_alloc_aec:
	kfree(dp->taskbuffer);
	dp->taskbuffer = NULL;
failed_alloc_taskbuf:
	snd_release_dma(dp, dev);
failed_request_dma:
	kfree(dp->fragments);
	dp->fragments = NULL;
failed_alloc_fragments:
	return ret;
}

static void deinit_pipe(struct dsp_pipe *dp,struct device *dev)
{

	snd_release_dma(dp, dev);
	if(dp->fragments){
		kfree(dp->fragments);
		dp->fragments = NULL;
	}
	if(dp->taskbuffer){
		kfree(dp->taskbuffer);
		dp->taskbuffer = NULL;
	}
	mutex_destroy(&dp->mutex);
#if 0
	/* free memory */
	dmam_free_noncoherent(dev,
			      dp->reservesize,
			      dp->vaddr,
			      dp->paddr);
	dp->vaddr = NULL;
#endif
	if(dp->aec_buffer){
		kfree(dp->aec_buffer);
		dp->aec_buffer = NULL;
	}
	if(dp->aecfragments){
		kfree(dp->aecfragments);
		dp->aecfragments = NULL;
	}
	if(dp->taskaecbuffer){
		kfree(dp->taskaecbuffer);
		dp->taskaecbuffer = NULL;
	}
}

/*###########################################################*\
 * interfacees
 \*###########################################################*/
/********************************************************\
 * llseek
 \********************************************************/
loff_t xb_snd_dsp_llseek(struct file *file,
			 loff_t offset,
			 int origin,
			 struct snd_dev_data *ddata)
{
	return 0;
}

/********************************************************\
 * read
 \********************************************************/
ssize_t xb_snd_dsp_read(struct file *file,
			char __user *buffer,
			size_t count,
			loff_t *ppos,
			struct snd_dev_data *ddata)
{
	return 0;
}

/********************************************************\
 * write
 \********************************************************/
ssize_t xb_snd_dsp_write(struct file *file,
			 const char __user *buffer,
			 size_t count,
			 loff_t *ppos,
			 struct snd_dev_data *ddata)
{
	return 0;
}

/********************************************************\
 * ioctl
 \********************************************************/
unsigned int xb_snd_dsp_poll(struct file *file,
			     poll_table *wait,
			     struct snd_dev_data *ddata)
{
	return -EINVAL;
}

/********************************************************\
 * ioctl
\********************************************************/

static inline long dsp_ioctl_set_channel(struct dsp_route_object *object, unsigned int channel)
{
	long ret = 0;
	if(object){
		object->channel = channel;
	}else{
		ret = -EBADF;
	}
	return ret;
}

static inline long dsp_ioctl_set_fmt(struct dsp_route_object *object, unsigned int fmt)
{
	long ret = 0;

	if(object){
		object->format = fmt;
	}else{
		ret = -EBADF;
	}
	return ret;
}

static inline long dsp_ioctl_set_rate(struct dsp_route_object *object, unsigned int rate)
{
	long ret = 0;

	if(object){
		object->rate = rate;
	}else{
		ret = -EBADF;
	}
	return ret;
}


static int jz_asoc_dma_stop_and_reset(struct dsp_pipe *dp)
{
	/*struct dsp_data_fragment *node = NULL;*/
	unsigned long lock_flags;
	if(dp == NULL)
		return -EINVAL;

	spin_lock_irqsave(&dp->pipe_lock, lock_flags);
	if (dp->dma_chan) {
		if(dp->is_trans != false){
			dmaengine_terminate_all(dp->dma_chan);
			/*wait_for_completion_interruptible(&dp->dma_completion);*/
			dp->is_trans = false;
		}
	}
	dp->pipe_state = SND_DSP_STATE_OPEN;
	spin_unlock_irqrestore(&dp->pipe_lock, lock_flags);

	/*mutex_lock(&dp->mutex);*/
#if 0
	/* delete the data list */
	while(!list_empty(&dp->fragments_head)){
		node = list_first_entry(&dp->fragments_head, typeof(*node), list);
		list_del(&node->list);
	}
	while(!list_empty(&dp->aecfragments_head)){
		node = list_first_entry(&dp->aecfragments_head, typeof(*node), list);
		list_del(&node->list);
	}
#endif
	if(dp->fragments)
		memset(dp->fragments, 0, sizeof(struct dsp_data_fragment)*dp->fragment_cnt);
	if(dp->aec_buffer)
		memset(dp->aec_buffer, 0, dp->reservesize);

	INIT_LIST_HEAD(&dp->fragments_head);
	INIT_LIST_HEAD(&dp->aecfragments_head);
	dp->fragment_cnt = 0;

	/*mutex_unlock(&dp->mutex);*/
	return 0;
}

static int jz_asoc_dma_prepare_and_submit(struct dsp_pipe *dp)
{
	struct dma_async_tx_descriptor *desc;
	unsigned long flags = DMA_CTRL_ACK;
	int index = 0;

	if(dp->fragment_size){
		dp->fragment_cnt = dp->reservesize / dp->fragment_size;
	}

	dp->fragment_cnt = dp->fragment_cnt > cached_fragments ? cached_fragments : dp->fragment_cnt;

	if(dp->fragment_cnt == 0)
		return -1;

	/* Now, it's active buffer size  */
	dp->buffersize = dp->fragment_cnt * dp->fragment_size;

	for(index = 0; index < dp->fragment_cnt; index++){
		dp->fragments[index].vaddr = (dp->vaddr + index*dp->fragment_size);
		dp->fragments[index].paddr = (dp->paddr + index*dp->fragment_size);
		dp->fragments[index].size = dp->fragment_size;
		dp->fragments[index].index = index;
		dp->fragments[index].counter = 0;
		list_add_tail(&dp->fragments[index].list, &dp->fragments_head);
	}

	memset(dp->vaddr, 0, dp->buffersize);
	dma_sync_single_for_device(NULL, dp->paddr, dp->buffersize, DMA_TO_DEVICE);
	dmaengine_slave_config(dp->dma_chan,&dp->dma_config);

	desc = dp->dma_chan->device->device_prep_dma_cyclic(dp->dma_chan,
			dp->paddr,
			dp->buffersize,
			dp->buffersize,
			dp->dma_config.direction,
			flags,
			NULL);
	if (!desc) {
		dev_err(NULL, "cannot prepare slave dma\n");
		return -EINVAL;
	}
#if 0
	/* set desc callback */
	desc->callback = snd_dma_callback;
	desc->callback_param = (void *)dp;
#endif
	dmaengine_submit(desc);
	return 0;
}

static inline long dsp_ioctl_enable_stream(struct file *file)
{
	long ret = 0;
	struct dsp_route_object *object = NULL;
	struct dsp_route_attr *attr = NULL;
	struct dsp_pipe *dp = NULL;
	struct snd_dev_data *ddata = NULL;
	enum snd_device_t arg = 0;
	unsigned long lock_flags;

	object = file->private_data;
	if(object == NULL || object->fd != file){
		snd_error_print("The fd is wrong!\n");
		return -EBADF;
	}

	dp = object->dp;
	ddata = dp ? dp->pddata: NULL;

	if(dp == NULL || ddata == NULL)
		return -EINVAL;
	attr = &dp->living_attr;
	mutex_lock(&dp->mutex);
	/* the route hasn't been opened */
	if(dp->pipe_state == SND_DSP_STATE_CLOSE){
		snd_error_print("Please opened it firstly!\n");
		mutex_unlock(&dp->mutex);
		return -EPERM;
	}

	if(dp->pipe_state == SND_DSP_STATE_RUN){
		if(object->rate != attr->rate || object->format != attr->format
					|| object->channel != attr->channel){
			snd_debug_print("The attr of the file donesn't match with living attr.\n");
			ret = -EPERM;
		}
	}else{
		/* */
		if(file->f_mode & FMODE_WRITE){
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_SET_REPLAY_CHANNELS, (unsigned long)&object->channel);
			if(ret){
				snd_error_print("Failed to set replay channel\n");
				goto exit;
			}
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_SET_REPLAY_RATE, (unsigned long)&object->rate);
			if(ret){
				snd_error_print("Failed to set replay rate\n");
				goto exit;
			}
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_SET_REPLAY_FMT, (unsigned long)&object->format);
			if(ret){
				snd_error_print("Failed to set replay format\n");
				goto exit;
			}
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_GET_REPLAY_FRAGMENTSIZE, (unsigned long)&dp->fragment_size);
			if(ret){
				snd_error_print("Failed to get replay fragment size\n");
				goto exit;
			}
		}else if(file->f_mode & FMODE_READ){
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_SET_RECORD_CHANNELS, (unsigned long)&object->channel);
			if(ret){
				snd_error_print("Failed to set record format\n");
				goto exit;
			}
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_SET_RECORD_RATE, (unsigned long)&object->rate);
			if(ret){
				snd_error_print("Failed to set record format\n");
				goto exit;
			}
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_SET_RECORD_FMT, (unsigned long)&object->format);
			if(ret){
				snd_error_print("Failed to set record format\n");
				goto exit;
			}
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_GET_RECORD_FRAGMENTSIZE, (unsigned long)&dp->fragment_size);
			if(ret){
				snd_error_print("Failed to get record fragment size\n");
				goto exit;
			}
		}else{
			snd_error_print("Don't support the fmode(%d)!\n",file->f_mode);
			ret = -EPERM;
			goto exit;
		}

		attr->rate = object->rate;
		attr->format = object->format;
		attr->channel = object->channel;

		/* */
		ret = jz_asoc_dma_prepare_and_submit(dp);
		snd_debug_print("fragemen_size = %d\n",dp->fragment_size);

		if(file->f_mode & FMODE_WRITE){
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_ENABLE_REPLAY, 0);
			if(ret){
				snd_error_print("Failed to enable replay!\n");
				goto exit;
			}
			arg = SND_DEVICE_SPEAKER;
			arg = (int)ddata->dev_ioctl_2(ddata, SND_DSP_SET_DEVICE, (unsigned long)&arg);
			if (arg < 0) {
				snd_error_print("Failed to set replay device!\n");
				ret = -EIO;
				goto exit;
			}
		}else{
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_ENABLE_RECORD, 0);
			if(ret){
				snd_error_print("Failed to enable record!\n");
				goto exit;
			}
			arg = SND_DEVICE_BUILDIN_MIC;
			arg = (int)ddata->dev_ioctl_2(ddata, SND_DSP_SET_DEVICE, (unsigned long)&arg);
			if (arg < 0) {
				snd_error_print("Failed to set record device!\n");
				ret = -EIO;
				goto exit;
			}
		}

		spin_lock_irqsave(&dp->pipe_lock, lock_flags);
		dp->pipe_state = SND_DSP_STATE_RUN;
		spin_unlock_irqrestore(&dp->pipe_lock, lock_flags);
	}
	if(!ret)
		object->state = SND_DSP_STATE_RUN;
exit:
	mutex_unlock(&dp->mutex);
	return ret;
}

static inline long dsp_ioctl_disable_stream(struct file *file)
{
	long ret = 0;
	int i = 0;
	struct dsp_route_object *object = NULL;
	struct dsp_pipe *dp = NULL;
	struct snd_dev_data *ddata = NULL;
	struct task_node *node = NULL;
	/*unsigned long lock_flags;*/

	object = file->private_data;
	if(object == NULL || object->fd != file){
		snd_error_print("The fd is wrong!\n");
		return -EBADF;
	}

	dp = object->dp;
	ddata = dp ? dp->pddata: NULL;

	if(dp == NULL || ddata == NULL)
		return -EINVAL;

	mutex_lock(&dp->mutex);
	/* the route hasn't been opened */
	if(dp->pipe_state == SND_DSP_STATE_CLOSE){
		snd_error_print("Please opened it firstly!\n");
		mutex_unlock(&dp->mutex);
		return -EPERM;
	}

	object->state = SND_DSP_STATE_OPEN;

	for(i = 0; i < SND_DSP_PIPE_OBJECT_CNT; i++){
			if(dp->objects[i].fd && dp->objects[i].state == SND_DSP_STATE_RUN){
				break;
			}
	}

	/* all objects must have been disable stream */
	if(i >= SND_DSP_PIPE_OBJECT_CNT){
		/*mutex_unlock(&dp->mutex);*/
		jz_asoc_dma_stop_and_reset(dp);

		/*mutex_lock(&dp->mutex);*/
		if(file->f_mode & FMODE_WRITE){
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_DISABLE_DMA_TX, 0);
			ret |= (int)ddata->dev_ioctl_2(ddata, SND_DSP_DISABLE_REPLAY, 0);
		}else{
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_DISABLE_DMA_RX, 0);
			ret |= (int)ddata->dev_ioctl_2(ddata, SND_DSP_DISABLE_RECORD, 0);
		}
		if (ret)
			ret = -EFAULT;

		/*spin_lock_irqsave(&dp->pipe_lock, lock_flags);*/
		while(!list_empty(&dp->tasklist)){
			node = list_first_entry(&dp->tasklist, typeof(*node), list);
			list_del(&node->list);
			if(node->flags != SND_TASK_NODE_FINISHED){
				node->flags = SND_TASK_NODE_FINISHED;
				wake_up_interruptible(&dp->wq);
			}
		}
		/*spin_unlock_irqrestore(&dp->pipe_lock, lock_flags);*/

	}

	mutex_unlock(&dp->mutex);

	return ret;
}

static inline long dsp_ioctl_ao_stream(struct file *file, struct audio_ouput_stream *stream)
{
	long ret = 0;
	struct dsp_route_object *object = NULL;
	struct dsp_pipe *dp = NULL;
	unsigned int totalsize = 0;
	unsigned int maxsize = 0;
	unsigned int size = 0;
	void __user *buff = 0;

	if(stream->size == 0){
		snd_debug_print("The ao stream size is 0!\n");
		return 0;
	}
	object = file->private_data;
	if(object == NULL || object->fd != file){
		snd_error_print("The fd is wrong!\n");
		return -EBADF;
	}

	if(object->state != SND_DSP_STATE_RUN){
		snd_error_print("Please enable the stream firstly!\n");
		return -EPERM;
	}

	dp = object->dp;

	buff = stream->data;
	totalsize = stream->size;
	maxsize = SND_TASK_NODE_SIZE / dp->fragment_size * dp->fragment_size;

	while(totalsize){
		size = totalsize > maxsize ? maxsize : totalsize;

		object->datanode.data = object->buffer;
		object->datanode.size = size;
		object->datanode.flags = SND_TASK_NODE_PREPARED;
		ret = copy_from_user(object->datanode.data, buff, size);
		if(ret){
			snd_error_print("failed to copy_from_user; size = %d\n",size);
			ret = -EIO;
			break;
		}

		mutex_lock(&dp->mutex);
		list_add_tail(&object->datanode.list, &dp->tasklist);
		mutex_unlock(&dp->mutex);
		do{
			ret = wait_event_interruptible(dp->wq, object->datanode.flags == SND_TASK_NODE_FINISHED);
		}while(ret == -ERESTARTSYS);

		if(ret < 0){
			ret = -EIO;
			break;
		}
		totalsize -= size;
		buff += size;
	}

	stream->size -= totalsize;
	object->ioctrl_counter++;
	object->iodata_counter += stream->size;
	return ret;
}

static inline long dsp_ioctl_ai_stream(struct file *file, struct audio_input_stream *stream)
{
	long ret = 0;
	struct dsp_route_object *object = NULL;
	struct dsp_pipe *dp = NULL;
	unsigned int totalsize = 0;
	unsigned int maxsize = 0;
	unsigned int size = 0;
	void __user *buff = 0;
	void __user *aecbuff = 0;
	/*unsigned long lock_flags;*/

	if(stream->size == 0){
		snd_debug_print("The ai stream size is 0!\n");
		return 0;
	}
	object = file->private_data;
	if(object == NULL || object->fd != file){
		snd_error_print("The fd is wrong!\n");
		return -EBADF;
	}

	if(object->state != SND_DSP_STATE_RUN){
		snd_error_print("Please enable the stream firstly!\n");
		return -EPERM;
	}

	dp = object->dp;

	buff = stream->data;
	aecbuff = stream->aec;
	totalsize = stream->size;
	maxsize = SND_TASK_NODE_SIZE / dp->fragment_size * dp->fragment_size;
	while(totalsize){
		size = totalsize > maxsize ? maxsize : totalsize;

		object->datanode.data = object->buffer;
		object->datanode.size = size;
		object->datanode.flags = SND_TASK_NODE_PREPARED;

		if((object->aec_enable == SND_AEC_ON) && stream->aec){
			object->datanode.aec = object->aecbuffer;
		}else{
			object->datanode.aec = NULL;
		}
		mutex_lock(&dp->mutex);
		list_add_tail(&object->datanode.list, &dp->tasklist);
		mutex_unlock(&dp->mutex);
		do{
			ret = wait_event_interruptible(dp->wq, object->datanode.flags == SND_TASK_NODE_FINISHED);
		}while(ret == -ERESTARTSYS);

		if(ret < 0){
			ret = -EIO;
			break;
		}
		ret = copy_to_user(buff, object->buffer, size);
		if(ret){
			snd_error_print("failed to copy_to_user\n");
			ret = -EIO;
			break;
		}

		if(object->datanode.aec){
			ret = copy_to_user(aecbuff, object->aecbuffer, size);
			if(ret){
				snd_error_print("failed to copy_to_user\n");
				ret = -EIO;
				break;
			}
		}
		totalsize -= size;
		buff += size;
		aecbuff += size;
	}

	stream->size -= totalsize;
	object->ioctrl_counter++;
	object->iodata_counter += stream->size;
	return ret;
}

static inline long dsp_ioctl_aec_ops(struct file *file, char onoff)
{
	long ret = 0;
	struct dsp_route_object *object = NULL;
	struct dsp_pipe *dp = NULL;
	int index = 0;
	int fragsize = 0;

	object = file->private_data;
	if(object == NULL || object->fd != file){
		snd_error_print("The fd is wrong!\n");
		return -EBADF;
	}

	if(object->state != SND_DSP_STATE_RUN){
		snd_error_print("Please enable the stream firstly!\n");
		return -EPERM;
	}

	dp = object->dp;
#if (defined(CONFIG_SOC_T10) || defined(CONFIG_SOC_T20)  || defined(CONFIG_SOC_T30))
	fragsize = dp->fragment_size >> 1; /* this is specifical t10 and t20 */
#else
	fragsize = dp->fragment_size;
#endif
	mutex_lock(&dp->mutex);
	if(onoff && dp->aec_enable == 0){
		for(index = 0; index < dp->fragment_cnt; index++){
			dp->aecfragments[index].vaddr = (dp->aec_buffer + index*fragsize);
			dp->aecfragments[index].size = fragsize;
			dp->aecfragments[index].index = index;
			list_add_tail(&dp->aecfragments[index].list, &dp->aecfragments_head);
		}
	}

	if(onoff){
		dp->aec_enable++;
	}else{
		dp->aec_enable--;
	}
	object->aec_enable = onoff;
	mutex_unlock(&dp->mutex);

	return ret;
}

static inline long dsp_ioctl_sync_ao_stream(struct file *file)
{
	struct dsp_route_object *object = NULL;
	struct dsp_pipe *dp = NULL;
	unsigned int io_tracer = 0;
	unsigned int dma_tracer = 0;
	unsigned int time = 0;

	object = file->private_data;
	if(object == NULL || object->fd != file){
		snd_error_print("The fd is wrong!\n");
		return -EBADF;
	}

	if(object->state != SND_DSP_STATE_RUN){
		snd_error_print("Please enable the stream firstly!\n");
		return -EPERM;
	}

	dp = object->dp;

	mutex_lock(&dp->mutex);
	io_tracer = dp->io_tracer;
	dma_tracer = dp->dma_tracer;
	mutex_unlock(&dp->mutex);

	time = io_tracer < dma_tracer ? io_tracer + dp->fragment_cnt - dma_tracer: io_tracer - dma_tracer;
	msleep((time + 1)*10);
	return 0;
}

static inline long dsp_ioctl_flush_ao_stream(struct file *file)
{
	long ret = 0;
	struct dsp_route_object *object = NULL;
	struct dsp_pipe *dp = NULL;
	unsigned long time = 0;

	object = file->private_data;
	if(object == NULL || object->fd != file){
		snd_error_print("The fd is wrong!\n");
		return -EBADF;
	}

	if(object->state != SND_DSP_STATE_RUN){
		snd_error_print("Please enable the stream firstly!\n");
		return -EPERM;
	}

	dp = object->dp;

	if(dp->flush_enable){
		msleep(20);
	}else{
		mutex_lock(&dp->mutex);
		dp->flush_enable = 1;
		mutex_unlock(&dp->mutex);

		time = wait_for_completion_timeout(&dp->flush_completion, msecs_to_jiffies(500));
		if(!time && dp->flush_completion.done){
			snd_error_print("Flush AO timeout!\n");
			ret = -ETIMEDOUT;
		}
	}
	return ret;
}

/**
 * only the dsp device opend as O_RDONLY or O_WRONLY, ioctl
 * works, if a dsp device opend as O_RDWR, it will return -1
 **/
long xb_snd_dsp_ioctl(struct file *file,
		      unsigned int cmd,
		      unsigned long arg,
		      struct snd_dev_data *ddata)
{
	long ret = -EINVAL;
	struct dsp_pipe *dp = NULL;
	struct dsp_endpoints *endpoints = NULL;

	if (ddata == NULL)
		return -ENODEV;

	if(ddata->priv_data) {
		endpoints = xb_dsp_get_endpoints(ddata);
	} else {
		endpoints = (struct dsp_endpoints *)ddata->ext_data;
	}
	if (endpoints == NULL)
		return -ENODEV;

	/* O_RDWR mode operation, do not allowed */
	if ((file->f_mode & FMODE_READ) && (file->f_mode & FMODE_WRITE))
		return -EPERM;

	switch (cmd) {
		//case SNDCTL_DSP_BIND_CHANNEL:
		/* OSS 4.x: Route setero output to the specified channels(obsolete) */
		/* we do't support here */
		//break;
	case SNDCTL_DSP_STEREO:
	case SNDCTL_DSP_CHANNELS: {
		/* OSS 4.x: set the number of audio channels */
		int channels = -1;

		if (get_user(channels, (int *)arg)) {
			ret = -EFAULT;
			goto EXIT_IOCTRL;
		}

		if (cmd == SNDCTL_DSP_STEREO) {
			if (channels > 1){
				ret = -EINVAL;
				goto EXIT_IOCTRL;
			}
			channels = channels ? 2 : 1;
		}

		/* fatal: this command can be well used in O_RDONLY and O_WRONLY mode,
		   if opend as O_RDWR, only replay channels will be set, if record
		   channels also want to be set, use cmd SOUND_PCM_READ_CHANNELS instead*/
		ret = dsp_ioctl_set_channel(file->private_data, channels);

		if(ret)
			goto EXIT_IOCTRL;

		if (cmd == SNDCTL_DSP_STEREO) {
			if (channels > 2) {
				ret = -EFAULT;
				goto EXIT_IOCTRL;
			}
			channels = channels == 2 ? 1 : 0;
		}

		ret = put_user(channels, (int *)arg);
		break;
	}
	case SNDCTL_DSP_SETFMT: {
		/* OSS 4.x: Select the sample format */
		int fmt = -1;

		if (get_user(fmt, (int *)arg)) {
		       ret = -EFAULT;
		       goto EXIT_IOCTRL;
		}

		/* fatal: this command can be well used in O_RDONLY and O_WRONLY mode,
		   if opend as O_RDWR, only replay format will be set */
		if (file->f_mode & FMODE_WRITE) {
			dp = endpoints->out_endpoint;
			if(fmt == AFMT_QUERY){
				/* fatal: this command can be well used in O_RDONLY and O_WRONLY mode,
			   	if opend as O_RDWR, only replay current format will be return */
				fmt = dp->living_attr.format;
				ret = 0;
			}else{
				/* set format */
				ret = dsp_ioctl_set_fmt(file->private_data, fmt);
			}
		} else if (file->f_mode & FMODE_READ) {
			dp = endpoints->in_endpoint;
			if(fmt == AFMT_QUERY){
				/* query format */
				fmt = dp->living_attr.format;
				ret = 0;
			}else{
				/* set format */
				ret = dsp_ioctl_set_fmt(file->private_data, fmt);
			}
		} else {
			ret = -EPERM;
			goto EXIT_IOCTRL;
		}
		if(ret)
			goto EXIT_IOCTRL;
		ret = put_user(fmt, (int *)arg);
		break;
	}

	case SNDCTL_DSP_SPEED: {
		/* OSS 4.x: Set the sampling rate */
		int rate = -1;

		if (get_user(rate, (int *)arg)) {
			ret = -EFAULT;
			goto EXIT_IOCTRL;
		}

		/* fatal: this command can be well used in O_RDONLY and O_WRONLY mode,
		   if opend as O_RDWR, only replay rate will be set, if record rate
		   also need to be set, use cmd SOUND_PCM_READ_RATE instead */
		if (file->f_mode & FMODE_WRITE) {
			dp = endpoints->out_endpoint;
			ret = dsp_ioctl_set_rate(file->private_data, rate);
		} else if (file->f_mode & FMODE_READ) {
			dp = endpoints->in_endpoint;
			ret = dsp_ioctl_set_rate(file->private_data, rate);
		} else {
			ret = -EPERM;
			goto EXIT_IOCTRL;
		}
		ret = put_user(rate, (int *)arg);

		break;
	}

	case SNDCTL_DSP_GETFMTS: {
		/* OSS 4.x: Returns a list of natively supported sample formats */
		int mask = -1;

		/* fatal: this command can be well used in O_RDONLY and O_WRONLY mode,
		   if opend as O_RDWR, only replay supported formats will be return */
		if (file->f_mode & FMODE_WRITE) {
			dp = endpoints->out_endpoint;
			mask = dp->living_attr.format;
		} else if (file->f_mode & FMODE_READ) {
			dp = endpoints->in_endpoint;
			mask = dp->living_attr.format;
		} else {
			ret = -EPERM;
			goto EXIT_IOCTRL;
		}
		ret = put_user(mask, (int *)arg);
		break;
	}

	case SNDCTL_EXT_SYNC_STREAM:
		/* OSS 4.x: Suspend the application until all samples have been played */
		//TODO: check it will wait until replay complete
		if (file->f_mode & FMODE_WRITE) {
			ret = dsp_ioctl_sync_ao_stream(file);
		} else
			ret = -ENOSYS;
		break;
	case SNDCTL_EXT_FLUSH_STREAM:
		/* OSS 4.x: Suspend the application until all samples have been played */
		//TODO: check it will wait until replay complete
		if (file->f_mode & FMODE_WRITE) {
			ret = dsp_ioctl_flush_ao_stream(file);
		} else
			ret = -ENOSYS;
		break;
	case SNDCTL_EXT_SET_REPLAY_VOLUME: {
		int vol;
		if (get_user(vol, (int*)arg)){
			ret = -EFAULT;
			goto EXIT_IOCTRL;
		}
		if (file->f_mode & FMODE_READ)
			dp = endpoints->in_endpoint;
		if (file->f_mode & FMODE_WRITE)
			dp = endpoints->out_endpoint;
		if (ddata->dev_ioctl) {
			mutex_lock(&dp->mutex);
			ret = (int)ddata->dev_ioctl(SND_DSP_SET_REPLAY_VOL, (unsigned long)&vol);
			mutex_unlock(&dp->mutex);
		} else if (ddata->dev_ioctl_2) {
			mutex_lock(&dp->mutex);
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_SET_REPLAY_VOL, (unsigned long)&vol);
			mutex_unlock(&dp->mutex);
		}
			if (!ret)
				break;
		ret = put_user(vol, (int *)arg);
		break;
	}

	case SNDCTL_EXT_SET_RECORD_VOLUME: {
		int vol;
		if (get_user(vol, (int*)arg)){
			ret = -EFAULT;
			goto EXIT_IOCTRL;
		}
		if (file->f_mode & FMODE_READ)
			dp = endpoints->in_endpoint;
		if (file->f_mode & FMODE_WRITE)
			dp = endpoints->out_endpoint;
		if (ddata->dev_ioctl) {
			mutex_lock(&dp->mutex);
			ret = (int)ddata->dev_ioctl(SND_DSP_SET_RECORD_VOL, (unsigned long)&vol);
			mutex_unlock(&dp->mutex);
		} else if (ddata->dev_ioctl_2) {
			mutex_lock(&dp->mutex);
			ret = (int)ddata->dev_ioctl_2(ddata, SND_DSP_SET_RECORD_VOL, (unsigned long)&vol);
			mutex_unlock(&dp->mutex);
		}
			if (!ret)
				break;
		ret = put_user(vol, (int *)arg);
		break;
	}

	case SNDCTL_EXT_ENABLE_STREAM:
		ret = dsp_ioctl_enable_stream(file);
		break;
	case SNDCTL_EXT_DISABLE_STREAM:
		snd_debug_print(" disable stream #########\n");
		ret = dsp_ioctl_disable_stream(file);
		break;
	case SNDCTL_EXT_SET_AO_STREAM:
		if(file->f_mode & FMODE_WRITE){
			struct audio_ouput_stream stream;
			copy_from_user(&stream, (__user void*)arg, sizeof(stream));
			ret = dsp_ioctl_ao_stream(file, &stream);
			copy_to_user((__user void*)arg, &stream, sizeof(stream));
		}else{
			snd_error_print("The fd can't replay audio, please open it with O_WRONLY\n");
			ret = -EPERM;
		}
		break;
	case SNDCTL_EXT_GET_AI_STREAM:
		if(file->f_mode & FMODE_READ){
			struct audio_input_stream stream;
			copy_from_user(&stream, (__user void*)arg, sizeof(stream));
			ret = dsp_ioctl_ai_stream(file, &stream);
			copy_to_user((__user void*)arg, &stream, sizeof(stream));
		}else{
			snd_error_print("The fd can't record audio, please open it with O_RDONLY\n");
			ret = -EPERM;
		}
		break;
	case SNDCTL_EXT_ENABLE_AEC:
		if(file->f_mode & FMODE_READ)
			ret = dsp_ioctl_aec_ops(file, SND_AEC_ON);
		break;
	case SNDCTL_EXT_DISABLE_AEC:
		if(file->f_mode & FMODE_READ)
			ret = dsp_ioctl_aec_ops(file, SND_AEC_OFF);
		break;
	default:
		snd_warn_print("SOUDND ERROR:ioctl command %d is not supported\n", cmd);
		ret = -1;
		goto EXIT_IOCTRL;
	}
EXIT_IOCTRL:
	return ret;
}

/********************************************************\
 * open
\********************************************************/
int xb_snd_dsp_open(struct inode *inode,
		    struct file *file,
		    struct snd_dev_data *ddata)
{
	int ret = 0;
	int i = 0;
	struct dsp_pipe *dpi = NULL;
	struct dsp_pipe *dpo = NULL;
	struct dsp_endpoints *endpoints = NULL;
	struct dsp_route_object *object = NULL;

	/* O_RDWR mode operation, do not allowed */
	if ((file->f_mode & FMODE_READ) && (file->f_mode & FMODE_WRITE))
		return -EPERM;

	if (ddata == NULL) {
		return -ENODEV;
	}

	if(ddata->priv_data) {
		endpoints = xb_dsp_get_endpoints(ddata);
	} else {
		endpoints = (struct dsp_endpoints *)ddata->ext_data;
	}
	if (endpoints == NULL) {
		return -ENODEV;
	}

	mutex_lock(&endpoints->mutex);
	if(file->f_mode & FMODE_READ){
		dpi = endpoints->in_endpoint;
		dpi->pddata = ddata;	//used for callback

		for(i = 0; i < SND_DSP_PIPE_OBJECT_CNT; i++){
			if(dpi->objects[i].fd == file){
				snd_error_print("The filenode has been opend!\n");
				mutex_unlock(&endpoints->mutex);
				return -EBADF;
			}
			if(dpi->objects[i].fd == 0){
				object = &dpi->objects[i];
				break;
			}
		}

		if(object == NULL){
			snd_error_print("The audio driver is busy!\n");
			mutex_unlock(&endpoints->mutex);
			return -EBUSY;
		}
		object->buffer = dpi->taskbuffer + i*SND_TASK_NODE_SIZE;
		object->aecbuffer = dpi->taskaecbuffer + i*SND_TASK_NODE_SIZE;
		object->fd = file;
		object->dp = dpi;
		object->state = SND_DSP_STATE_OPEN;
		object->ioctrl_counter = 0;
		object->iodata_counter = 0;
		file->private_data = object;

		if(dpi->pipe_state == SND_DSP_STATE_CLOSE){
			dpi->pipe_state = SND_DSP_STATE_OPEN;
			dpi->aec_enable = 0;
			if(endpoints->state == SND_DSP_STATE_CLOSE)
				endpoints->state = SND_DSP_STATE_OPEN;
		}
	}

	object = NULL;
	if(file->f_mode & FMODE_WRITE){
		dpo = endpoints->out_endpoint;
		dpo->pddata = ddata; //used for callback

		for(i = 0; i < SND_DSP_PIPE_OBJECT_CNT; i++){
			if(dpo->objects[i].fd == file){
				snd_error_print("The filenode has been opend!\n");
				mutex_unlock(&endpoints->mutex);
				return -EBADF;
			}
			if(dpo->objects[i].fd == 0){
				object = &dpo->objects[i];
				break;
			}
		}

		if(object == NULL){
			snd_error_print("The audio driver is busy!\n");
			mutex_unlock(&endpoints->mutex);
			return -EBUSY;
		}

		object->buffer = dpo->taskbuffer + i*SND_TASK_NODE_SIZE;
		object->aecbuffer = NULL;
		object->fd = file;
		object->dp = dpo;
		object->state = SND_DSP_STATE_OPEN;
		object->ioctrl_counter = 0;
		object->iodata_counter = 0;
		file->private_data = object;

		if(dpo->pipe_state == SND_DSP_STATE_CLOSE){
			dpo->pipe_state = SND_DSP_STATE_OPEN;
			dpo->flush_enable = 0;
			if(endpoints->state == SND_DSP_STATE_CLOSE)
				endpoints->state = SND_DSP_STATE_OPEN;
		}
	}

	/* enable hrtimer */
	if(endpoints->state != SND_DSP_STATE_CLOSE && atomic_read(&endpoints->timer_stopped)){
		atomic_set(&endpoints->timer_stopped, 0);
		hrtimer_start(&endpoints->hr_timer, endpoints->expires , HRTIMER_MODE_REL);
	}
	mutex_unlock(&endpoints->mutex);
	return ret;
}

/********************************************************\
 * release
 \********************************************************/
int xb_snd_dsp_release(struct inode *inode,
		       struct file *file,
		       struct snd_dev_data *ddata)
{
	int i = 0;
	int ret = 0;
	int refcnt = 0;
	struct dsp_pipe *dpi = NULL;
	struct dsp_pipe *dpo = NULL;
	struct dsp_endpoints *endpoints = NULL;
	struct dsp_route_object *object = NULL;

	if (ddata == NULL)
		return -1;

	if(ddata->priv_data) {
		endpoints = xb_dsp_get_endpoints(ddata);
	} else {
		endpoints = (struct dsp_endpoints *)ddata->ext_data;
	}
	if (endpoints == NULL)
		return -1;

	dpi = endpoints->in_endpoint;
	if (file->f_mode & FMODE_READ) {

		object = file->private_data;
		if(object->aec_enable == SND_AEC_ON)
			xb_snd_dsp_ioctl(file, SNDCTL_EXT_ENABLE_AEC, 0, ddata);
		if(object->state == SND_DSP_STATE_RUN)
			xb_snd_dsp_ioctl(file, SNDCTL_EXT_DISABLE_STREAM, 0, ddata);

		object->datanode.data = NULL;
		object->datanode.aec = NULL;
		object->fd = NULL;
		object->state = SND_DSP_STATE_CLOSE;
		snd_debug_print("\n");
		mutex_lock(&dpi->mutex);
		for(i = 0; i < SND_DSP_PIPE_OBJECT_CNT; i++){
			if(dpi->objects[i].fd){
				mutex_unlock(&dpi->mutex);
				goto next;
			}
		}

		snd_debug_print("\n");
		dpi->pipe_state = SND_DSP_STATE_CLOSE;
		mutex_unlock(&dpi->mutex);
	}
next:
	refcnt = 0;
	dpo = endpoints->out_endpoint;
	if (file->f_mode & FMODE_WRITE) {

		object = file->private_data;

		if(object->state == SND_DSP_STATE_RUN)
			xb_snd_dsp_ioctl(file, SNDCTL_EXT_DISABLE_STREAM, 0, ddata);

		object->fd = NULL;
		object->state = SND_DSP_STATE_CLOSE;

		mutex_lock(&dpo->mutex);
		for(i = 0; i < SND_DSP_PIPE_OBJECT_CNT; i++){
			if(dpo->objects[i].fd){
				snd_debug_print("i=%d fd=%p\n",i,dpo->objects[i].fd);
				mutex_unlock(&dpo->mutex);
				goto out;
			}
		}
		dpo->pipe_state = SND_DSP_STATE_CLOSE;
		mutex_unlock(&dpo->mutex);
	}

out:
	mutex_lock(&endpoints->mutex);
	if((dpi->pipe_state == SND_DSP_STATE_CLOSE) && (dpo->pipe_state == SND_DSP_STATE_CLOSE)){
		atomic_set(&endpoints->timer_stopped, 1);
		endpoints->state = SND_DSP_STATE_CLOSE;
	}
	mutex_unlock(&endpoints->mutex);
	return ret;
}

/********************************************************\
 * xb_snd_probe
\********************************************************/
static int ao_sync_loop = 0;
static inline void ao_dmaaddr_sync_ioaddr(struct dsp_pipe *dp, unsigned int dma_current)
{
	struct dsp_data_fragment *io_frag = &dp->fragments[dp->io_tracer];
	struct dsp_data_fragment *tmp_frag = NULL;
	struct list_head *list = NULL;
	unsigned int tmp_io = dp->io_tracer;
	if(dp->dma_tracer > tmp_io)
		tmp_io += dp->fragment_cnt;
	if(dp->dma_tracer > dma_current)
		dma_current += dp->fragment_cnt;
	if(tmp_io < dma_current + SND_DSP_PIPE_DAM2IO_CNT)
		tmp_io = (dma_current + SND_DSP_PIPE_DAM2IO_CNT);

	tmp_io = tmp_io % dp->fragment_cnt;
	tmp_frag = &dp->fragments[tmp_io];
	ao_sync_loop = 0;
	while(io_frag != tmp_frag){
		ao_sync_loop++;
		if(ao_sync_loop > 1000)
			snd_error_print("LOOPS are too much\n");
		io_frag->counter = 0;
		list = &io_frag->list;
		if(list->next == &dp->fragments_head)
			list = list->next;
		io_frag = list_entry(list->next, struct dsp_data_fragment, list);
	}

	dp->io_tracer = tmp_io;
}

static inline void ai_dmaaddr_sync_ioaddr(struct dsp_pipe *dp, unsigned int dma_current)
{
	unsigned int tmp_io = dp->io_tracer;
	if(dp->dma_tracer > tmp_io)
		tmp_io += dp->fragment_cnt;
	if(dp->dma_tracer > dma_current)
		dma_current += dp->fragment_cnt;
	if(tmp_io < dma_current + SND_DSP_PIPE_DAM2IO_CNT)
		tmp_io = dma_current + SND_DSP_PIPE_DAM2IO_CNT;
	dp->io_tracer = tmp_io % dp->fragment_cnt;
}

static int ao_copy_loop = 0;
static inline int ao_copy_from_user(struct dsp_pipe *dp, struct task_node *node)
{
	int i = 0;
	int size = dp->fragment_size;
	int cnt = node->size / size;
	struct dsp_data_fragment *io_frag = &dp->fragments[dp->io_tracer];
	struct dsp_data_fragment *dma_frag = &dp->fragments[dp->dma_tracer];
	struct list_head *list = NULL;

	for(i = 0; i < cnt; i++){
		if(io_frag == dma_frag)
			break;
		memcpy(io_frag->vaddr, (const void*)node->data, size);
		dma_sync_single_for_device(NULL, (dma_addr_t)(io_frag->paddr), io_frag->size, DMA_TO_DEVICE);
		io_frag->counter = 0;
		node->data += size;
		ao_copy_loop++;
		/*dma_cache_sync(NULL, (void *)io_frag->vaddr, io_frag->size, DMA_MEM_TO_DEV);*/
		list = &io_frag->list;
		if(list->next == &dp->fragments_head)
			list = list->next;
		io_frag = list_entry(list->next, struct dsp_data_fragment, list);
	}
	dp->io_tracer = io_frag->index;
	node->size -= i*size;
	/*snd_debug_print("node->size=%d\n",node->size);*/
	return node->size;
}

static inline int ai_copy_to_user(struct dsp_pipe *dp, struct task_node *node)
{
	int i = 0;
	int size = dp->fragment_size;
	int size_inno = size >> 1;
	int cnt = node->size / size_inno;
	struct dsp_data_fragment *io_frag = &dp->fragments[dp->io_tracer];
	struct dsp_data_fragment *aec_frag = &dp->aecfragments[dp->io_tracer];
	struct dsp_data_fragment *dma_frag = &dp->fragments[dp->dma_tracer];
	struct list_head *list = NULL;

	for(i = 0; i < cnt; i++){
		if(io_frag == dma_frag)
			break;
		dma_sync_single_for_device(NULL, (dma_addr_t)(io_frag->paddr), io_frag->size, DMA_FROM_DEVICE);
		dp->filter(io_frag->vaddr, &size, size_inno);
		memcpy(node->data, io_frag->vaddr, size_inno);
		node->data += size_inno;
		memset(io_frag->vaddr, 0, io_frag->size);
		dma_sync_single_for_device(NULL, (dma_addr_t)(io_frag->paddr), io_frag->size, DMA_TO_DEVICE);
		if(node->aec){
			aec_frag = &dp->aecfragments[io_frag->index];
			copy_to_user(node->aec, aec_frag->vaddr, size_inno);
			memset(aec_frag->vaddr, 0, size_inno);
			node->aec += size_inno;
		}
		list = &io_frag->list;
		if(list->next == &dp->fragments_head)
			list = list->next;
		io_frag = list_entry(list->next, struct dsp_data_fragment, list);
	}
	/*snd_debug_print("cnt = %d io_frag->paddr = 0x%08x, i = %d\n", cnt, io_frag->paddr, i);*/
	dp->io_tracer = io_frag->index;
	node->size -= i*size_inno;
	return node->size;
}

static void dsp_workqueue_handle(struct work_struct *work)
{
	int ret = 0;
	struct dsp_endpoints *endpoints = container_of(work, struct dsp_endpoints, workqueue);
	struct dsp_pipe *dpi = endpoints->in_endpoint;
	struct dsp_pipe *dpo = endpoints->out_endpoint;
	struct dma_chan *dma_chan = NULL;
	dma_addr_t dpi_currentaddr = 0;
	dma_addr_t dpo_currentaddr = 1;
	unsigned long lock_flags;
	unsigned int ao_current_index = 0;
	unsigned int ai_current_index = 0;
	struct task_node *node = NULL;
	struct list_head *list = NULL;

	int aec_start = 0;
	int aec_end = 0;
	int ao_start = 0;
	int ao_end = 0;
	struct dsp_data_fragment *aec_sfrag = NULL;
	struct dsp_data_fragment *aec_efrag = NULL;
	struct dsp_data_fragment *ao_sfrag = NULL;
	struct dsp_data_fragment *ao_efrag = NULL;
	int first_scan = 1;
	int loop_cnt0 = 0;
	int loop_cnt1 = 0;

	/*snd_debug_print("workqueue\n");*/
	spin_lock_irqsave(&dpi->pipe_lock, lock_flags);
	/* get living dma address */
	if(dpi && dpi->is_trans){
		dma_chan = dpi->dma_chan;
		dpi_currentaddr = dma_chan->device->get_current_trans_addr(dma_chan, NULL, NULL,
				dpi->dma_config.direction);
		ai_current_index = ((dpi_currentaddr - dpi->paddr) / dpi->fragment_size) % dpi->fragment_cnt;
	}

	if(dpo && dpo->is_trans){
		dma_chan = dpo->dma_chan;
		dpo_currentaddr = dma_chan->device->get_current_trans_addr(dma_chan, NULL, NULL,
				dpo->dma_config.direction);
		ao_current_index = ((dpo_currentaddr - dpo->paddr) / dpo->fragment_size) % dpo->fragment_cnt;
	}
	spin_unlock_irqrestore(&dpi->pipe_lock, lock_flags);

	/* update ao data */
	if(dpo && dpo->is_trans){
		mutex_lock(&dpo->mutex);
		ao_copy_loop = 0;
		ao_dmaaddr_sync_ioaddr(dpo, ao_current_index);
		while(!list_empty(&dpo->tasklist)){
			loop_cnt0++;
			if(loop_cnt0 > 1000)
				snd_error_print("LOOPS are too much\n");
			node = list_first_entry(&dpo->tasklist, typeof(*node), list);
			ret = ao_copy_from_user(dpo, node);
			if(ret == 0){
				list_del(&node->list);
				node->flags = SND_TASK_NODE_FINISHED;
				wake_up_interruptible(&dpo->wq);
			}else
				break;
		}
		mutex_unlock(&dpo->mutex);
	}

	loop_cnt0 = 0;
	/* copy ai and aec to user */
	if(dpi->is_trans && dpi->pipe_state == SND_DSP_STATE_RUN){
		mutex_lock(&dpi->mutex);
		/* sync aec data */
		if(dpi->aec_enable && dpo->is_trans){
				/* ao enable */
				ao_start = dpo->dma_tracer;
				ao_sfrag = &dpo->fragments[ao_start];
				aec_start = dpi->dma_tracer;
				aec_end = ai_current_index;
				aec_sfrag = &dpi->aecfragments[aec_start];
				aec_efrag = &dpi->aecfragments[aec_end];
				while(aec_sfrag->index != aec_efrag->index){
					loop_cnt0++;
					if(loop_cnt0 > 1000)
						snd_error_print("LOOPS are too much\n");
					if(first_scan){
						while(ao_sfrag->counter){
							loop_cnt1++;
							if(loop_cnt1 > dpo->fragment_cnt){
								printk(KERN_ERR"#### Warnning ####\n");
								printk(KERN_ERR"LOOPS are too much: copy %d, sync %d; [%d, %d][%d, %d, %d]\n", ao_copy_loop, ao_sync_loop, 
											aec_sfrag->index, ai_current_index, dpo->dma_tracer, ao_current_index, dpo->io_tracer);
								break;
							}
							/* move ao */
							list = &ao_sfrag->list;
							if(list->next == &dpo->fragments_head)
								list = list->next;
							ao_sfrag = list_entry(list->next, struct dsp_data_fragment, list);
						}
					}
					first_scan = 0;
					/* ao data has been finished to copy */
					if(ao_sfrag->counter)
						break;
					ao_sfrag->counter = 1;
					memcpy(aec_sfrag->vaddr, ao_sfrag->vaddr, aec_sfrag->size);
					aec_sfrag->counter = ao_sfrag->counter;
					/* move aec */
					list = &aec_sfrag->list;
					if(list->next == &dpi->aecfragments_head)
						list = list->next;
					aec_sfrag = list_entry(list->next, struct dsp_data_fragment, list);
					/* move ao */
					list = &ao_sfrag->list;
					if(list->next == &dpo->fragments_head)
						list = list->next;
					ao_sfrag = list_entry(list->next, struct dsp_data_fragment, list);
				}
				/*dpi->dma_tracer = aec_sfrag->index;*/
				if(aec_sfrag->index != ai_current_index)
					printk(KERN_ERR"error:aec %d, ai %d sd %d\n", aec_sfrag->index, ai_current_index, aec_efrag->index);
		}

		dpi->dma_tracer = dpi->fragments[ai_current_index].index;

		ai_dmaaddr_sync_ioaddr(dpi, ai_current_index);
		loop_cnt0 = 0;
		/* copy data */
		while(!list_empty(&dpi->tasklist)){
			loop_cnt0++;
			if(loop_cnt0 > 1000)
				snd_error_print("LOOPS are too much\n");
			node = list_first_entry(&dpi->tasklist, typeof(*node), list);
			ret = ai_copy_to_user(dpi, node);
			if(ret == 0){
				list_del(&node->list);
				node->flags = SND_TASK_NODE_FINISHED;
				wake_up_interruptible(&dpi->wq);
			}else
				break;
		}
		mutex_unlock(&dpi->mutex);
	}

	loop_cnt0 = 0;
	/* sync ao dma_tracer and clear ao data */
	if(dpo->is_trans && dpo->pipe_state == SND_DSP_STATE_RUN){
		mutex_lock(&dpo->mutex);
		if(dpo->flush_enable){
			memset(dpo->vaddr, 0, dpo->buffersize);
			dma_sync_single_for_device(NULL, dpo->paddr, dpo->buffersize, DMA_TO_DEVICE);
			dpo->io_tracer = (ao_current_index + SND_DSP_PIPE_DAM2IO_CNT) % dpo->fragment_cnt;
			dpo->flush_enable = 0;
			complete(&dpo->flush_completion);
		}else{
			ao_start = dpo->dma_tracer;
			ao_end = ao_current_index;
			ao_sfrag = &dpo->fragments[ao_start];
			ao_efrag = &dpo->fragments[ao_end];
			while(ao_sfrag != ao_efrag){
				loop_cnt0++;
				if(loop_cnt0 > 1000)
					printk(KERN_ERR"LOOPS are too much\n");
				memset(ao_sfrag->vaddr, 0, ao_sfrag->size);
				dma_sync_single_for_device(NULL, (dma_addr_t)(ao_sfrag->paddr), ao_sfrag->size, DMA_TO_DEVICE);
				list = &ao_sfrag->list;
				if(list->next == &dpo->fragments_head)
					list = list->next;
				ao_sfrag = list_entry(list->next, struct dsp_data_fragment, list);
			}
		}
		dpo->dma_tracer = dpo->fragments[ao_current_index].index;
		mutex_unlock(&dpo->mutex);
	}
	return;
}

static enum hrtimer_restart jz_asoc_hrtimer_callback(struct hrtimer *hr_timer) {
	struct dsp_endpoints *endpoints = container_of(hr_timer,
			struct dsp_endpoints, hr_timer);
	struct dsp_pipe *dpi = endpoints->in_endpoint;
	struct dsp_pipe *dpo = endpoints->out_endpoint;
	struct dma_chan *dma_chan = NULL;
	struct snd_dev_data *ddata = NULL;

	if (atomic_read(&endpoints->timer_stopped))
		goto out;
	hrtimer_start(&endpoints->hr_timer, endpoints->expires , HRTIMER_MODE_REL);
	if(dpi && dpi->pipe_state == SND_DSP_STATE_RUN){
		dma_chan = dpi->dma_chan;
		ddata = dpi->pddata;
		if(dpi->is_trans == false){
			/* start to dma */
			dpi->dma_tracer = 0;
			dpi->io_tracer = (dpi->fragment_cnt - SND_DSP_PIPE_DAM2IO_CNT);
			dma_async_issue_pending(dma_chan);
			ddata->dev_ioctl_2(ddata, SND_DSP_ENABLE_DMA_RX, 0);
			dpi->is_trans = true;
		}
	}

	if(dpo && dpo->pipe_state == SND_DSP_STATE_RUN){
		dma_chan = dpo->dma_chan;
		ddata = dpo->pddata;
		if(dpo->is_trans == false){
			/* start to dma */
			dpo->dma_tracer = 0;
			dpo->io_tracer = SND_DSP_PIPE_DAM2IO_CNT;
			dma_async_issue_pending(dma_chan);
			ddata->dev_ioctl_2(ddata, SND_DSP_ENABLE_DMA_TX, 0);
			dpo->is_trans = true;
		}
	}
	schedule_work(&endpoints->workqueue);
out:
	return HRTIMER_NORESTART;
}

static int audio_info_show(struct seq_file *m, void *v)
{
	int len = 0;
	struct dsp_endpoints *endpoints = (struct dsp_endpoints*)(m->private);
    struct dsp_pipe *ao;
    struct dsp_pipe *ai;
	int open_cnt = 0, run_cnt = 0, free_cnt = 0;
	int index = 0;
	if(endpoints == NULL)
		return len;
	ao = endpoints->out_endpoint;
	if(ao == NULL)
		return len;

	len += seq_printf(m ,"The version of audio driver is %s\n", AUDIO_DRIVER_VERSION);
	/*len += seq_printf(m ,"=== The info of audio replay ===\n");*/
	switch(ao->pipe_state){
		case SND_DSP_STATE_CLOSE:
			len += seq_printf(m ,"audio replay status is %s\n", "closed");
			break;
		case SND_DSP_STATE_OPEN:
			len += seq_printf(m ,"audio replay status is %s\n", "opened");
			break;
		default:
			len += seq_printf(m ,"audio replay status is %s\n", "running");
			break;
	}
	if(ao->pipe_state == SND_DSP_STATE_RUN){
		for(index = 0; index < SND_DSP_PIPE_OBJECT_CNT; index++){
			if(ao->objects[index].state == SND_DSP_STATE_OPEN)
				open_cnt++;
			else if(ao->objects[index].state == SND_DSP_STATE_RUN){
				open_cnt++;
				run_cnt++;
				len += seq_printf(m ,"replay fd[%p]: ioctrl(%lld), datasize(%lld)\n",
							ao->objects[index].fd, ao->objects[index].ioctrl_counter, ao->objects[index].iodata_counter);
			}else{
				free_cnt++;
			}
		}
		len += seq_printf(m ,"The objects state of replay: [using %d, run %d] [free %d]\n", open_cnt, run_cnt, free_cnt);
		len += seq_printf(m ,"The living rate of replay %d\n", ao->living_attr.rate);
		len += seq_printf(m ,"The living channel of replay %d\n", ao->living_attr.channel);
		len += seq_printf(m ,"The living format of replay %d\n", ao->living_attr.format);
		len += seq_printf(m ,"The dma paddr of replay 0x%08x\n", ao->paddr);
		len += seq_printf(m ,"The dma tracer of replay %d\n", ao->dma_tracer);
		len += seq_printf(m ,"The io tracer of replay %d\n", ao->io_tracer);
	}
	ai = endpoints->in_endpoint;
	if(ai == NULL)
		return len;
	open_cnt = 0;
	run_cnt = 0;
	free_cnt = 0;
	/*len += seq_printf(m ,"=== The info of audio record ===\n");*/
	switch(ai->pipe_state){
		case SND_DSP_STATE_CLOSE:
			len += seq_printf(m ,"audio record status is %s\n", "closed");
			break;
		case SND_DSP_STATE_OPEN:
			len += seq_printf(m ,"audio record status is %s\n", "opened");
			break;
		default:
			len += seq_printf(m ,"audio record status is %s\n", "running");
			break;
	}
	if(ai->pipe_state == SND_DSP_STATE_RUN){
		for(index = 0; index < SND_DSP_PIPE_OBJECT_CNT; index++){
			if(ai->objects[index].state == SND_DSP_STATE_OPEN)
				open_cnt++;
			else if(ai->objects[index].state == SND_DSP_STATE_RUN){
				open_cnt++;
				run_cnt++;
				len += seq_printf(m ,"record fd[%p]: ioctrl(%lld), datasize(%lld)\n",
							ai->objects[index].fd, ai->objects[index].ioctrl_counter, ai->objects[index].iodata_counter);
			}else{
				free_cnt++;
			}
		}
		len += seq_printf(m ,"The objects state of record: [using %d, run %d] [free %d]\n", open_cnt, run_cnt, free_cnt);
		len += seq_printf(m ,"The living rate of record %d\n", ai->living_attr.rate);
		len += seq_printf(m ,"The living channel of record %d\n", ai->living_attr.channel);
		len += seq_printf(m ,"The living format of record %d\n", ai->living_attr.format);
		len += seq_printf(m ,"The dma paddr of record 0x%08x\n", ai->paddr);
		len += seq_printf(m ,"The dma tracer of record %d\n", ai->dma_tracer);
		len += seq_printf(m ,"The io tracer of record %d\n", ai->io_tracer);
		len += seq_printf(m ,"The aec state is %s\n", ai->aec_enable ? "enable" : "disable");
	}

	return len;
}
static int audio_info_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, audio_info_show, PDE_DATA(inode),2048);
}
static const struct file_operations audio_info_fops ={
	.read = seq_read,
	.open = audio_info_open,
	.llseek = seq_lseek,
	.release = single_release,
};

int xb_snd_dsp_probe(struct snd_dev_data *ddata)
{
	int ret = -1;
	struct dsp_pipe *dp = NULL;
	struct dsp_endpoints *endpoints = NULL;
	struct proc_dir_entry *proc;

#ifndef CONFIG_HIGH_RES_TIMERS
	snd_error_print("NOTE: The HRTIMER had been selected when audio driver was enabled!\n");
	return -1;
#endif
	if (ddata == NULL)
		return -1;

	if(ddata->priv_data) {
		endpoints = xb_dsp_get_endpoints(ddata);
	} else {
		endpoints = (struct dsp_endpoints *)ddata->ext_data;
	}

	if (endpoints == NULL)
		return -1;

	if(cached_fragments > SND_DSP_DMA_FRAGMENT_MAX_CNT || cached_fragments < SND_DSP_DMA_FRAGMENT_MIN_CNT){
		snd_error_print("There is reasonable cached fragments! Its range should be 20 to 100.\n");
		return -1;
	}

	mutex_init(&endpoints->mutex);

	/* out_endpoint init */
	if ((dp = endpoints->out_endpoint) != NULL) {
		ret = init_pipe(dp , ddata->dev,DMA_TO_DEVICE);
		if (ret)
			goto error1;
		dp->dsp = endpoints;
	}
	if ((dp = endpoints->in_endpoint) != NULL) {
		ret = init_pipe(dp , ddata->dev,DMA_FROM_DEVICE);
		if (ret)
			goto error2;
		dp->dsp = endpoints;
	}

	atomic_set(&endpoints->timer_stopped, 1);
	hrtimer_init(&endpoints->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	endpoints->hr_timer.function = jz_asoc_hrtimer_callback;
	endpoints->expires = ns_to_ktime(1000*1000*20);	// the time section is 20ms.
	INIT_WORK(&endpoints->workqueue, dsp_workqueue_handle);

	/* debug info */
	proc = jz_proc_mkdir("audio");
	if (!proc) {
		endpoints->proc = NULL;
		snd_debug_print("create dev_attr_isp_info failed!\n");
	} else {
		endpoints->proc = proc;
	}
	proc_create_data("audio_info", S_IRUGO, proc, &audio_info_fops, (void *)endpoints);

	endpoints->state = SND_DSP_STATE_CLOSE;
	snd_debug_print("audio dsp driver probe ok!\n");
	return 0;

error2:
	deinit_pipe(endpoints->out_endpoint, ddata->dev);
error1:
	return ret;
}

int xb_snd_dsp_remove(struct snd_dev_data *ddata)
{
	struct dsp_endpoints *endpoints = NULL;

	if (ddata == NULL)
		return -1;

	if(ddata->priv_data) {
		endpoints = xb_dsp_get_endpoints(ddata);
	} else {
		endpoints = (struct dsp_endpoints *)ddata->ext_data;
	}

	if (endpoints == NULL)
		return -1;
	if(endpoints->state != SND_DSP_STATE_CLOSE){
		snd_error_print("The device is running, Can't remove it!\n");
		return -1;
	}

	hrtimer_cancel(&endpoints->hr_timer);
	cancel_work_sync(&endpoints->workqueue);
	deinit_pipe(endpoints->in_endpoint, ddata->dev);
	deinit_pipe(endpoints->out_endpoint, ddata->dev);
	mutex_destroy(&endpoints->mutex);

	if (endpoints->proc)
		proc_remove(endpoints->proc);

	return 0;
}

int xb_snd_dsp_suspend(struct snd_dev_data *ddata)
{
	if (ddata){
		struct dsp_endpoints * endpoints = NULL;
		bool out_trans = false,in_trans = false;

		if(ddata->priv_data) {
			endpoints = xb_dsp_get_endpoints(ddata);
		} else {
			endpoints = (struct dsp_endpoints *)ddata->ext_data;
		}
		if(endpoints->out_endpoint) {
			mutex_lock(&endpoints->out_endpoint->mutex);
			out_trans = endpoints->out_endpoint->is_trans;
			mutex_unlock(&endpoints->out_endpoint->mutex);
		}
		if(endpoints->in_endpoint){
			mutex_lock(&endpoints->in_endpoint->mutex);
			in_trans = endpoints->in_endpoint->is_trans;
			mutex_unlock(&endpoints->in_endpoint->mutex);
		}
		if(out_trans || in_trans)
			return -1;
	}
	return 0;
}
int xb_snd_dsp_resume(struct snd_dev_data *ddata)
{
	return 0;
}
