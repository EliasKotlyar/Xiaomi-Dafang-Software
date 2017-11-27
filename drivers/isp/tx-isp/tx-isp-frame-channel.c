#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/bug.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
//#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/fs.h>

#include "tx-isp-frame-channel.h"
#include "tx-isp-videobuf.h"
#include "tx-isp-debug.h"
//#include "tx-isp-csi.h"
//#include "tx-isp-vic.h"

static int frame_channel_video_irq_notify(frame_chan_vdev_t  *vdev, u32 phyaddr, bool *handled)
{
	unsigned long flags;
	struct frame_channel_video_buffer *buf = NULL;
	struct frame_channel_video_buffer *pos = NULL;

/*	if(atomic_read(&vdev->state) != TX_ISP_STATE_RUN){
		printk("vdev->state is stop \n");
		return 0;
	}
*/
	spin_lock_irqsave(&vdev->slock, flags);
	list_for_each_entry(pos, &vdev->active_list, entry){
		if(pos->buf.addr == phyaddr){
			buf = pos;
			list_del(&buf->entry);
			break;
		}
	}
#if 0
	if (!list_empty(&vdev->active_list)){
		buf = list_entry(vdev->active_list.next,
				struct frame_channel_video_buffer, entry);
		list_del(&buf->entry);
	}
#endif
	spin_unlock_irqrestore(&vdev->slock, flags);
	if(buf && buf->vb.state == VB2_BUF_STATE_ACTIVE){
		struct timespec ts;
		getrawmonotonic(&ts);

		buf->vb.v4l2_buf.timestamp.tv_sec = ts.tv_sec;
		buf->vb.v4l2_buf.timestamp.tv_usec = ts.tv_nsec / 1000;

		buf->vb.v4l2_buf.sequence = vdev->out_frames++;
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);
		complete(&vdev->comp);
	//	printk("bufdone chan%d buf.index = %d\n", vdev->index, buf->vb.v4l2_buf.index);
	}else{
		vdev->lose_frames++;
	//	printk("lose frame  chan%d \n", vdev->index);
	}

	return 0;
}

static int frame_channel_vb2_queue_setup(struct vb2_queue *vq, const struct v4l2_format *fmt,
			unsigned int *nbuffers, unsigned int *nplanes, unsigned int sizes[],
			void *alloc_ctxs[])
{
	frame_chan_vdev_t *vdev = vb2_get_drv_priv(vq);
	struct frame_channel_attribute *attr = &vdev->attr;
	struct v4l2_format *output = &attr->output;
	unsigned int size;
	int ret = ISP_SUCCESS;

	/*
	* 判断nbuffers 是否大于我们的最高限制。
	* 主要负责nplanes 和 相应size[]，alloc_ctxs[] 的设置
	* 目前暂时都是单plane操作。
	*/
	size = output->fmt.pix.sizeimage;

	*nplanes = 1;
	sizes[0] = size;
	alloc_ctxs[0] = vdev->vbm;

//	printk("~~~~~~~~ %s[%d] size = %d ~~~~~~~~~~\n",__func__,__LINE__,size);
	INIT_LIST_HEAD(&vdev->active_list);
	return ret;
}

static int frame_channel_vb2_buffer_init(struct vb2_buffer *vb)
{
	/*
	* 如果MMAP，只会调用一次。
	* 如果USERPTR，driver可以初始化一些私有变量
	*/
	frame_chan_vdev_t *vdev = vb2_get_drv_priv(vb->vb2_queue);
	struct frame_channel_video_buffer *buf =
		container_of(vb, struct frame_channel_video_buffer, vb);
	buf->buf.priv = vdev;

	return 0;
}

static void frame_channel_vb2_buffer_cleanup(struct vb2_buffer *vb)
{
	/*
	* 如果MMAP，只会调用一次。
	* 如果USERPTR，driver可以初始化一些私有变量
	*/
}

#define TX_ISP_VIDEO_BUFFER_CACHE (1<<31)
static int frame_channel_vb2_buffer_prepare(struct vb2_buffer *vb)
{
	frame_chan_vdev_t *vdev = vb2_get_drv_priv(vb->vb2_queue);
	struct frame_channel_video_buffer *buf =
		container_of(vb, struct frame_channel_video_buffer, vb);
	struct v4l2_buffer *v4l2_buf = &vb->v4l2_buf;
	struct frame_channel_attribute *attr = &vdev->attr;
	struct v4l2_format *output = &attr->output;
	unsigned long size = output->fmt.pix.sizeimage;
	void * addr = NULL;
	int ret = ISP_SUCCESS;

	/* 目前是单plane操作 */
	if (vb2_plane_size(vb, 0) < size) {
		ISP_PRINT(ISP_ERROR_LEVEL,"Data will not fit into plane (%lu < %lu)\n",
				vb2_plane_size(vb, 0), size);
		return -EINVAL;
	}
	if(v4l2_buf->memory == V4L2_MEMORY_USERPTR){
		addr = vb2_plane_vaddr(vb, 0);
	}else if(v4l2_buf->memory == V4L2_MEMORY_MMAP){
		addr = vb2_plane_cookie(vb, 0);
	}else{
		ISP_PRINT(ISP_ERROR_LEVEL,"%s[%d] memory is invalid!\n", __func__, __LINE__);
		ret = -EINVAL;
	}

//	if(v4l2_buf->flags & TX_ISP_VIDEO_BUFFER_CACHE)
	{
		if(v4l2_buf->memory == V4L2_MEMORY_USERPTR){
			dma_sync_single_for_device(NULL, (dma_addr_t)addr, size, DMA_FROM_DEVICE);
//			dma_cache_sync(NULL, (void *)addr, size, DMA_FROM_DEVICE);
		}else if(v4l2_buf->memory == V4L2_MEMORY_MMAP){
			dma_sync_single_for_device(NULL, (dma_addr_t)addr, size, DMA_FROM_DEVICE);
		}else{
			ISP_PRINT(ISP_ERROR_LEVEL,"%s[%d] memory is invalid!\n", __func__, __LINE__);
			ret = -EINVAL;
		}
	}
//	printk("~~~~ %s[%d] index = %d addr = 0x%08x ~~~~\n",__func__,__LINE__, v4l2_buf->index, (unsigned int)addr);
	buf->buf.addr = (unsigned int)addr;
	return ret;
}

static int frame_channel_vb2_buffer_finish(struct vb2_buffer *vb)
{
	frame_chan_vdev_t *vdev = vb2_get_drv_priv(vb->vb2_queue);
	struct frame_channel_video_buffer *buf =
		container_of(vb, struct frame_channel_video_buffer, vb);
	struct frame_channel_attribute *attr = &vdev->attr;
	struct v4l2_format *output = &attr->output;
	unsigned long size = output->fmt.pix.sizeimage;

	vb2_set_plane_payload(&buf->vb, 0, size);

	return 0;
}

static void frame_channel_vb2_buffer_queue(struct vb2_buffer *vb)
{
	frame_chan_vdev_t *vdev = vb2_get_drv_priv(vb->vb2_queue);
	struct frame_channel_video_buffer *buf =
		container_of(vb, struct frame_channel_video_buffer, vb);
	struct v4l2_subdev *parent = vdev->parent;
	unsigned long flags;
	struct isp_private_ioctl ioctl;
	int ret = ISP_SUCCESS;

//	ISP_PRINT(ISP_INFO_LEVEL,"%s==========%d\n", __func__, __LINE__);
	spin_lock_irqsave(&vdev->slock, flags);
	list_add_tail(&buf->entry, &vdev->active_list);
	spin_unlock_irqrestore(&vdev->slock, flags);

//	ISP_PRINT(ISP_INFO_LEVEL,"%s[%d] %p\n", __func__, __LINE__, &buf->buf);
	ioctl.dir = TX_ISP_PRIVATE_IOCTL_SET;
	ioctl.cmd = TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_QUEUE_BUFFER;
	ioctl.value = (int)buf;
	ret = v4l2_subdev_call(parent, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);
}

static int frame_channel_vb2_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	frame_chan_vdev_t *vdev = vb2_get_drv_priv(vq);
	struct v4l2_subdev *parent = vdev->parent;
	struct isp_private_ioctl ioctl;
	int ret = ISP_SUCCESS;
	unsigned long flags;

	if(atomic_read(&vdev->state) != TX_ISP_STATE_START){
		return -EBUSY;
	}
	ioctl.dir = TX_ISP_PRIVATE_IOCTL_SET;
	ioctl.cmd = TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_STREAM_ON;
	ioctl.value = (int)vdev;
	ret = v4l2_subdev_call(parent, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);
	if(ret == ISP_SUCCESS){
		spin_lock_irqsave(&vdev->slock, flags);
		atomic_set(&vdev->state, TX_ISP_STATE_RUN);
		spin_unlock_irqrestore(&vdev->slock, flags);
	}
	ISP_PRINT(ISP_INFO_LEVEL,"%s==========%d\n", __func__, __LINE__);
	return ret;
}

static int frame_channel_vb2_stop_streaming(struct vb2_queue *vq)
{
	frame_chan_vdev_t *vdev = vb2_get_drv_priv(vq);
	struct v4l2_subdev *parent = vdev->parent;
	struct isp_private_ioctl ioctl;
	int ret = ISP_SUCCESS;
	unsigned long flags;

	if(atomic_read(&vdev->state) == TX_ISP_STATE_RUN){
		ioctl.dir = TX_ISP_PRIVATE_IOCTL_SET;
		ioctl.cmd = TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_STREAM_OFF;
		ioctl.value = (int)vdev;
		ret = v4l2_subdev_call(parent, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);
		if(ret == ISP_SUCCESS){
			spin_lock_irqsave(&vdev->slock, flags);
			atomic_set(&vdev->state, TX_ISP_STATE_START);
			spin_unlock_irqrestore(&vdev->slock, flags);
		}
		ISP_PRINT(ISP_INFO_LEVEL,"%s==========%d\n", __func__, __LINE__);
	}else if(atomic_read(&vdev->state) == TX_ISP_STATE_START){
		ret = ISP_SUCCESS;
	}else{
		ret = -EINVAL;
	}
	return ret;
}

static void frame_channel_vb2_lock(struct vb2_queue *vq)
{
//	frame_chan_vdev_t *vdev = vb2_get_drv_priv(vq);
//	mutex_lock(&vdev->mlock);
}

static void frame_channel_vb2_unlock(struct vb2_queue *vq)
{
//	frame_chan_vdev_t *vdev = vb2_get_drv_priv(vq);
//	mutex_unlock(&vdev->mlock);
}

static struct vb2_ops frame_channel_vb2_qops = {
	.queue_setup		= frame_channel_vb2_queue_setup,
	.buf_init		= frame_channel_vb2_buffer_init,
	.buf_cleanup		= frame_channel_vb2_buffer_cleanup,
	.buf_prepare		= frame_channel_vb2_buffer_prepare,
	.buf_finish		= frame_channel_vb2_buffer_finish,
	.buf_queue		= frame_channel_vb2_buffer_queue,
	.wait_prepare		= frame_channel_vb2_unlock,
	.wait_finish		= frame_channel_vb2_lock,
	.start_streaming	= frame_channel_vb2_start_streaming,
	.stop_streaming		= frame_channel_vb2_stop_streaming,
};

static int frame_channel_vidioc_querycap(struct file *file, void  *priv,
		struct v4l2_capability *cap)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	ISP_PRINT(ISP_INFO_LEVEL,"%s==========%d\n", __func__, __LINE__);

	strcpy(cap->driver, "frame_channel");
	strcpy(cap->card, "frame_channel");
	strlcpy(cap->bus_info, vdev->parent->v4l2_dev->name, sizeof(cap->bus_info));
	cap->version = 0x00000001;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	return 0;
}
/**
* frame_channel_vidioc_g_priority() - get priority handler
* @file: file ptr
* @priv: file handle
* @prio: ptr to v4l2_priority structure
*/
static int frame_channel_vidioc_g_priority(struct file *file, void *priv,
                          enum v4l2_priority *prio)
{
//	frame_chan_vdev_t *vdev = video_drvdata(file);
//	*prio = v4l2_prio_max(&vdev->prio);
	return 0;
}
/**
* frame_channel_vidioc_s_priority() - set priority handler
* @file: file ptr
* @priv: file handle
* @prio: ptr to v4l2_priority structure
*/
static int frame_channel_vidioc_s_priority(struct file *file, void *priv, enum v4l2_priority p)
{
//	frame_chan_vdev_t *vdev = video_drvdata(file);
//	struct frame_channel_fh *fh = priv;
//	return v4l2_prio_change(&vdev->prio, &fh->prio, p);
	return 0;
}

static int frame_channel_vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
		struct v4l2_fmtdesc *f)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	struct v4l2_subdev *parent = vdev->parent;
	struct isp_private_ioctl ioctl;
	int ret = ISP_SUCCESS;

	ISP_PRINT(ISP_INFO_LEVEL,"%s==========%d\n", __func__, __LINE__);
	vdev->fmtdesc = f;
	ioctl.dir = TX_ISP_PRIVATE_IOCTL_GET;
	ioctl.cmd = TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_ENUM_FMT;
	ioctl.value = (int)vdev;
	ret = v4l2_subdev_call(parent, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);
	if(ret == ISP_SUCCESS)
		f->type = vdev->vbq.type;
	return ret;
}

static int frame_channel_vidioc_g_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *f)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	struct v4l2_format	*out = &vdev->attr.output;
#if 0
	f->fmt.pix.pixelformat = output->fmt.pix.pixelformat;
#else
	memcpy(f, &vdev->attr.output, sizeof(*f));
#endif
	printk("%s[%d] pixformat = 0x%08x width = %d height = %d lineoffset = %d sizeimage = %d\n", __func__, __LINE__,
	out->fmt.pix.pixelformat, out->fmt.pix.width, out->fmt.pix.height, out->fmt.pix.bytesperline, out->fmt.pix.sizeimage);
	printk("%s[%d] pixformat = 0x%08x width = %d height = %d lineoffset = %d sizeimage = %d\n", __func__, __LINE__,
	f->fmt.pix.pixelformat, f->fmt.pix.width, f->fmt.pix.height, f->fmt.pix.bytesperline, f->fmt.pix.sizeimage);
	printk("%s[%d] g_fmt is ok!\n", __func__, __LINE__);
	return ISP_SUCCESS;
}

static int frame_channel_vidioc_try_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *f)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	struct frame_channel_attribute *attr = &vdev->attr;
	struct isp_private_ioctl ioctl;
	int ret = ISP_SUCCESS;

	ISP_PRINT(ISP_INFO_LEVEL,"%s[%d] f->fmt.pix.width = %d, f->fmt.pix.height = %d\n",
			__func__, __LINE__, f->fmt.pix.width, f->fmt.pix.height);

	if (f->fmt.pix.field == V4L2_FIELD_ANY)
		f->fmt.pix.field = V4L2_FIELD_INTERLACED;
	else if (V4L2_FIELD_INTERLACED != f->fmt.pix.field)
		return -EINVAL;
	v4l_bound_align_image(&f->fmt.pix.width,
			attr->min_width, attr->max_width, 3,
			&f->fmt.pix.height,
			attr->min_width, attr->max_height, 1, 0);
	if (f->fmt.pix.width  < attr->min_width || f->fmt.pix.width  > attr->max_width ||
			f->fmt.pix.height < attr->min_height || f->fmt.pix.height > attr->max_height) {
		ISP_PRINT(ISP_ERROR_LEVEL,"Invalid format (%dx%d)\n",
				f->fmt.pix.width, f->fmt.pix.height);
		return -EINVAL;
	}
	vdev->fmt = f;
	ioctl.dir = TX_ISP_PRIVATE_IOCTL_GET;
	ioctl.cmd = TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_TRY_FMT;
	ioctl.value = (int)vdev;
	ISP_PRINT(ISP_INFO_LEVEL,"%s[%d] f->fmt.pix.width = %d, f->fmt.pix.height = %d\n",
			__func__, __LINE__, f->fmt.pix.width, f->fmt.pix.height);
	ret = v4l2_subdev_call(vdev->parent, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);
	if(ret != ISP_SUCCESS) {
		ISP_PRINT(ISP_ERROR_LEVEL,"Format(0x%x) size(%dx%d) is unsupported\n",
				f->fmt.pix.pixelformat,
				f->fmt.pix.width,
				f->fmt.pix.height);
	}

	return ret;
}

static int frame_channel_vidioc_s_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *f)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	struct isp_private_ioctl ioctl;
//	struct frame_channel_fih *fh = priv;
	struct vb2_queue *q = &vdev->vbq;
	int ret = ISP_SUCCESS;

	ISP_PRINT(ISP_INFO_LEVEL,"%s==========%d\n", __func__, __LINE__);
	/* check priority */
//	ret = v4l2_prio_check(&camdev->prio, fh->prio);
//	if(0 != ret)
//		return ret;
	if (vb2_is_streaming(q))
		return -EBUSY;

	vdev->fmt = f;
	ret = frame_channel_vidioc_try_fmt_vid_cap(file, priv, f);
	if (ret != ISP_SUCCESS)
		return ret;
	/*set isp format */
	ioctl.dir = TX_ISP_PRIVATE_IOCTL_GET;
	ioctl.cmd = TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SET_FMT;
	ioctl.value = (int)vdev;
	ret = v4l2_subdev_call(vdev->parent, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);
	if(ret != ISP_SUCCESS) {
		ISP_PRINT(ISP_ERROR_LEVEL,"Format(0x%x) size(%dx%d) is unsupported\n",
				f->fmt.pix.pixelformat,
				f->fmt.pix.width,
				f->fmt.pix.height);
	}
	return ret;
}

static int frame_channel_vidioc_cropcap(struct file *file, void *priv,
		struct v4l2_cropcap *a)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	struct frame_channel_attribute *attr = &vdev->attr;
	struct isp_private_ioctl ioctl;
	int ret = ISP_SUCCESS;

	ioctl.dir = TX_ISP_PRIVATE_IOCTL_GET;
	ioctl.cmd = TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_CROP_CAP;
	ioctl.value = (int)vdev;
	ret = v4l2_subdev_call(vdev->parent, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);
	if(ret != ISP_SUCCESS) {
		ISP_PRINT(ISP_INFO_LEVEL,"Don't support to Crop\n");
	}else{
		a->bounds = attr->bounds;
		a->defrect = attr->bounds;
		a->type				= vdev->vbq.type;
		a->pixelaspect.numerator	= 1;
		a->pixelaspect.denominator	= 1;
		ISP_PRINT(ISP_INFO_LEVEL,"CropCap: default rect is %dx%d\n",
				attr->bounds.width, attr->bounds.height);
	}
	return ret;
}

static int frame_channel_vidioc_g_crop(struct file *file, void *priv,
		struct v4l2_crop *a)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	struct frame_channel_attribute *attr = &vdev->attr;

	a->c = attr->crop;
	a->type	= vdev->vbq.type;

	return ISP_SUCCESS;
}

static int frame_channel_vidioc_s_crop(struct file *file, void *fh,
		const struct v4l2_crop *a)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	struct frame_channel_attribute *attr = &vdev->attr;
	struct v4l2_rect *bounds = &(attr->bounds);
	struct isp_private_ioctl ioctl;
	struct vb2_queue *q = &vdev->vbq;
	int ret = ISP_SUCCESS;

	if (vb2_is_streaming(q))
		return -EBUSY;

	if(a->type != vdev->vbq.type)
		return -EINVAL;

	if(a->c.top < 0 || a->c.left < 0 || a->c.width < 0 || a->c.height < 0
		|| a->c.left + a->c.width > bounds->width ||
		a->c.top + a->c.height > bounds->height){
		printk("%s[%d] the parameter is invalid!\n", __func__,__LINE__);
		return -EINVAL;
	}

//	memcpy(&attr->crop, a->c,  sizeof(struct v4l2_rect));
	if(a->c.width && a->c.height)
		attr->crop_enable = 1;
	else
		attr->crop_enable = 0;
	attr->crop = a->c;
	ioctl.dir = TX_ISP_PRIVATE_IOCTL_SET;
	ioctl.cmd = TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SET_CROP;
	ioctl.value = (int)vdev;
	ret = v4l2_subdev_call(vdev->parent, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);
	return ret;
}

static int frame_channel_vidioc_reqbufs(struct file *file, void *priv,
		struct v4l2_requestbuffers *p)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	struct vb2_queue *q = &vdev->vbq;
	int ret = 0;
	ISP_PRINT(ISP_INFO_LEVEL,"%s[%d]\n", __func__, __LINE__);
	if(p->type != q->type){
		ISP_PRINT(ISP_ERROR_LEVEL,"%s[%d] req->type = %d, queue->type = %d\n", __func__, __LINE__,
					p->type, q->type);
		return -EINVAL;
	}

	ret = vb2_reqbufs(&vdev->vbq, p);
	if(!ret){
		vdev->reqbufs = p->count;
	}
	return ret;
}

static int frame_channel_vidioc_querybuf(struct file *file, void *priv,
		struct v4l2_buffer *p)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	ISP_PRINT(ISP_INFO_LEVEL,"%s==========%d\n", __func__, __LINE__);
	return vb2_querybuf(&vdev->vbq, p);
}

static int frame_channel_vidioc_qbuf(struct file *file, void *priv,
		struct v4l2_buffer *p)
{
	int ret;
	frame_chan_vdev_t *vdev = video_drvdata(file);
	ISP_PRINT(ISP_INFO_LEVEL,"%s==========%d\n", __func__, __LINE__);
	ret = vb2_qbuf(&vdev->vbq, p);
//	printk("+++ Qbuf: chan%d buf.index = %d\n", vdev->index, p->index);
	return ret;
}

static int frame_channel_vidioc_dqbuf(struct file *file, void *priv,
		struct v4l2_buffer *p)
{
	int ret;
	frame_chan_vdev_t *vdev = video_drvdata(file);
	ISP_PRINT(ISP_INFO_LEVEL,"%s==========%d\n", __func__, __LINE__);
	ret = vb2_dqbuf(&vdev->vbq, p, file->f_flags & O_NONBLOCK);
//	printk("--- DQbuf: chan%d buf.index = %d\n", vdev->index, p->index);
	return ret;
}
/* static unsigned int irq_read(unsigned int offset) */
/* { */
/* 	return *(volatile unsigned int *)(0xb3320000 + 0xc); */
/* } */
static int frame_channel_vidioc_streamon(struct file *file, void *priv,
		enum v4l2_buf_type i)
{
	int ret = 0;
	frame_chan_vdev_t *vdev = video_drvdata(file);
	ISP_PRINT(ISP_INFO_LEVEL,"%s==========%d\n", __func__, __LINE__);

	if(atomic_read(&vdev->state) != TX_ISP_STATE_START){
		return -EBUSY;
	}
	vdev->out_frames = 0;
	vdev->lose_frames = 0;
	INIT_COMPLETION(vdev->comp);
	ret = vb2_streamon(&vdev->vbq, i);
//	check_csi_error();
//	check_vic_error();
/* 			for(i = 0;i<10;i++){ */
/* 				while(((irq_read(0xc)&0x10000)!=0x10000)){ */
/* 				printk("1111111111111111111111111111\n"); */
/* 				} */
/* 				printk("2222222222222  0x%08x  2222222222222222\n", */
/* 				       (irq_read(0xc))); */
/* 				*(volatile unsigned int *)(0xb3320000 + 4) = 0x10001; */

/* } */

/* //	irq_write(IRQ_CLR_1, 0x10001); */
/* 		printk("vic------------start\n"); */


	return ret;
}

static int frame_channel_vidioc_streamoff(struct file *file, void *priv,
		enum v4l2_buf_type i)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	struct frame_channel_attribute *attr = &vdev->attr;
	int ret = 0;
	ISP_PRINT(ISP_INFO_LEVEL,"%s==========%d\n", __func__, __LINE__);
	ret = vb2_streamoff(&vdev->vbq, i);
	/* reset some public parameters */
	memset(&attr->crop, 0, sizeof(attr->crop));
	memset(&attr->scaler, 0, sizeof(attr->scaler));
//	memset(&attr->output, 0, sizeof(attr->output));
	attr->crop_enable = 0;
	attr->scaler_enable = 0;
	attr->frame_rate = 0;
	return ret;
}

static int frame_channel_vidioc_s_ctrl(struct file *file, void *priv,
		struct v4l2_control *ctrl)
{
	int ret = 0;
	return ret;
}

static int frame_channel_vidioc_g_ctrl(struct file *file, void *priv,
		struct v4l2_control *ctrl)
{
	int ret = 0;
	return ret;
}
static int frame_channel_vidioc_s_parm(struct file *file, void *priv,
		struct v4l2_streamparm *parm)
{
	int ret = 0;
	return ret;

}

static int frame_channel_vidioc_g_parm(struct file *file, void *priv,
		struct v4l2_streamparm *parm)
{
	int ret = 0;
	return ret;
}

static inline long frame_channel_scaler_capture(frame_chan_vdev_t *vdev, void *arg)
{
	struct frame_channel_attribute *attr = &vdev->attr;
	struct frame_image_scalercap *scalercap = &attr->scalercap;
	struct isp_private_ioctl ioctl;
	long ret = ISP_SUCCESS;


	ioctl.dir = TX_ISP_PRIVATE_IOCTL_GET;
	ioctl.cmd = TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SCALER_CAP;
	ioctl.value = (int)vdev;
	ret = v4l2_subdev_call(vdev->parent, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);
	if(ret == ISP_SUCCESS) {
	//	printk("&&&&&&& %s[%d] arg = %p *arg = 0x%08x  &&&&&&\n", __func__,__LINE__,arg, *(unsigned int*)arg);
		memcpy(arg, scalercap, sizeof(*scalercap));
	}
	return ret;
}

static inline long frame_channel_set_scaler_info(frame_chan_vdev_t *vdev, void *arg)
{
	struct frame_channel_attribute *attr = &vdev->attr;
	struct frame_image_scaler *scaler = &attr->scaler;
	struct frame_image_scalercap *scalercap = &attr->scalercap;
	struct isp_private_ioctl ioctl;
	struct vb2_queue *q = &vdev->vbq;
	long ret = ISP_SUCCESS;

	if (vb2_is_streaming(q))
		return -EBUSY;

	memcpy(scaler, arg, sizeof(*scaler));

//	attr->scaler = scaler;
	if(scaler->out_width && scaler->out_height)
		attr->scaler_enable = 1;
	else
		attr->scaler_enable = 0;

	if (attr->scaler_enable) {
		if(scaler->out_width > scalercap->max_width || scaler->out_width < scalercap->min_width
				|| scaler->out_height > scalercap->max_height || scaler->out_height < scalercap->min_height){
			printk("%s[%d] the parameter is invalid!\n", __func__, __LINE__);
			return -EINVAL;
		}
	}

	ioctl.dir = TX_ISP_PRIVATE_IOCTL_GET;
	ioctl.cmd = TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SET_SCALER;
	ioctl.value = (int)vdev;
	ret = v4l2_subdev_call(vdev->parent, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);
	return ret;
}

static inline long frame_channel_listen_buffer(frame_chan_vdev_t *vdev, void *arg)
{
//	unsigned int timeout = 0;
	long ret = ISP_SUCCESS;
#if 1
	if(atomic_read(&vdev->state) != TX_ISP_STATE_RUN){
		*(int *)arg = -1;
		return -EPERM;
	}
#endif
	ret = wait_for_completion_interruptible(&vdev->comp);
	if (ret < 0)
		*(int *)arg = -1;
	else
		*(int *)arg = vdev->comp.done + 1;
	return ret;
}

static long frame_channel_vidioc_default(struct file *file, void *fh, bool valid_prio, unsigned int cmd, void *arg)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	long ret = ISP_SUCCESS;

	switch(cmd){
		case VIDIOC_DEFAULT_CMD_SCALER_CAP:
			ret = frame_channel_scaler_capture(vdev, arg);
			break;
		case VIDIOC_DEFAULT_CMD_SET_SCALER:
			ret = frame_channel_set_scaler_info(vdev, arg);
			break;
		case VIDIOC_DEFAULT_CMD_LISTEN_BUF:
			ret = frame_channel_listen_buffer(vdev, arg);
			break;
		default:
			ret = -ISP_ERROR;
			break;
	}
	return ret;
}

static const struct v4l2_ioctl_ops frame_channel_v4l2_ioctl_ops = {

	/* VIDIOC_QUERYCAP handler */
	.vidioc_querycap		= frame_channel_vidioc_querycap,
	/* Priority handling */
	.vidioc_s_priority		= frame_channel_vidioc_s_priority,
	.vidioc_g_priority		= frame_channel_vidioc_g_priority,

	.vidioc_enum_fmt_vid_cap	= frame_channel_vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= frame_channel_vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap		= frame_channel_vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= frame_channel_vidioc_s_fmt_vid_cap,

	/*frame management*/
	.vidioc_reqbufs			= frame_channel_vidioc_reqbufs,
	.vidioc_querybuf		= frame_channel_vidioc_querybuf,
	.vidioc_qbuf			= frame_channel_vidioc_qbuf,
	.vidioc_dqbuf			= frame_channel_vidioc_dqbuf,

	/*isp function, modified according to spec*/
	.vidioc_g_ctrl			= frame_channel_vidioc_g_ctrl,
	.vidioc_s_ctrl			= frame_channel_vidioc_s_ctrl,
	.vidioc_cropcap			= frame_channel_vidioc_cropcap,
	.vidioc_g_crop			= frame_channel_vidioc_g_crop,
	.vidioc_s_crop			= frame_channel_vidioc_s_crop,
	.vidioc_s_parm			= frame_channel_vidioc_s_parm,
	.vidioc_g_parm			= frame_channel_vidioc_g_parm,

	.vidioc_streamon		= frame_channel_vidioc_streamon,
	.vidioc_streamoff		= frame_channel_vidioc_streamoff,

	/* For other private ioctls */
	.vidioc_default			= frame_channel_vidioc_default,
};

static int frame_channel_v4l2_open(struct file *file)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	struct frame_channel_attribute *attr = &vdev->attr;
	struct frame_channel_fh *fh;
	struct isp_private_ioctl ioctl;
	unsigned long flags;

	if(atomic_read(&vdev->state) != TX_ISP_STATE_STOP){
		return -EBUSY;
	}

	fh = (struct frame_channel_fh *)kzalloc(sizeof(*fh), GFP_KERNEL);
	if(NULL == fh)
		goto fh_alloc_fail;
	file->private_data = fh;
	/* Initialize priority of this instance to default priority */
	fh->prio = V4L2_PRIORITY_DEFAULT;
//	v4l2_prio_open(&camdev->prio, &fh->prio);

	/* reset some public parameters */
	memset(&attr->crop, 0, sizeof(attr->crop));
	memset(&attr->scaler, 0, sizeof(attr->scaler));
//	memset(&attr->output, 0, sizeof(attr->output));
	attr->crop_enable = 0;
	attr->scaler_enable = 0;
	attr->frame_rate = 0;

	ioctl.dir = TX_ISP_PRIVATE_IOCTL_GET;
	ioctl.cmd = TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SET_SCALER;
	ioctl.value = (int)vdev;
	v4l2_subdev_call(vdev->parent, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);

	ioctl.dir = TX_ISP_PRIVATE_IOCTL_SET;
	ioctl.cmd = TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SET_CROP;
	ioctl.value = (int)vdev;
	v4l2_subdev_call(vdev->parent, core, ioctl, VIDIOC_ISP_PRIVATE_IOCTL, &ioctl);

	spin_lock_irqsave(&vdev->slock, flags);
	atomic_set(&vdev->state, TX_ISP_STATE_START);
	spin_unlock_irqrestore(&vdev->slock, flags);
	return ISP_SUCCESS;
fh_alloc_fail:
	return -ENOMEM;
}

static int frame_channel_v4l2_close(struct file *file)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
//	struct frame_channel_fh *fh = file->private_data;
	unsigned long flags;
	if(atomic_read(&vdev->state) == TX_ISP_STATE_RUN){
		frame_channel_vidioc_streamoff(file, NULL, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	}

	if(atomic_read(&vdev->state) == TX_ISP_STATE_START){
		spin_lock_irqsave(&vdev->slock, flags);
		atomic_set(&vdev->state, TX_ISP_STATE_STOP);
		spin_unlock_irqrestore(&vdev->slock, flags);
	}
	/* Close the priority */
//	v4l2_prio_close(&camdev->prio, fh->prio);
//	printk("~~~ %s[%d]  ~~~~\n",__func__,__LINE__);
	return ISP_SUCCESS;
}

static unsigned int frame_channel_v4l2_poll(struct file *file,
		struct poll_table_struct *wait)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	return vb2_poll(&vdev->vbq, file, wait);
}

static int frame_channel_v4l2_mmap(struct file *file, struct vm_area_struct *vma)
{
	frame_chan_vdev_t *vdev = video_drvdata(file);
	return vb2_mmap(&vdev->vbq, vma);
}


static struct v4l2_file_operations frame_channel_v4l2_fops = {
	.owner 		= THIS_MODULE,
	.open 		= frame_channel_v4l2_open,
	.release 	= frame_channel_v4l2_close,
	.poll		= frame_channel_v4l2_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap 		= frame_channel_v4l2_mmap,
};

static struct video_device frame_channel_video = {
	.name = "isp-frame-channel",
	.minor = -1,
	.release = video_device_release,
	.fops = &frame_channel_v4l2_fops,
	.ioctl_ops = &frame_channel_v4l2_ioctl_ops,
};

static unsigned int bitmap = 0;
static void inline set_bitmap(int index)
{
	bitmap |= (1 << index);
}
static void inline clear_bitmap(int index)
{
	bitmap &= ~(1 << index);
}
static bool inline check_bitmap(int index)
{
	return bitmap & (1 << index) ? true : false;
}

int tx_isp_frame_channel_device_register(frame_chan_vdev_t *vdev)
{
	struct v4l2_device *v4l2_dev;
	struct video_device *vfd;
	struct vb2_queue *q;
	int ret = ISP_SUCCESS;

	if(!vdev)
		return -EINVAL;

	if(!vdev->parent || !vdev->vbm || check_bitmap(vdev->index)){
		return -EINVAL;
	}

	spin_lock_init(&vdev->slock);
	mutex_init(&vdev->mlock);
	init_completion(&vdev->comp);
	INIT_LIST_HEAD(&vdev->active_list);
	v4l2_dev = vdev->parent->v4l2_dev;
//	printk("&&&&&&&&&&& %s frame channel%d :slock = %p, mlock = %p &&&&&&&&&&\n",__func__,vdev->index,
//					&vdev->slock, &vdev->mlock);
	/* Initialize queue. */
	q = &vdev->vbq;
	memset(q, 0, sizeof(*q));
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_USERPTR;
	q->drv_priv = vdev;
	q->buf_struct_size = sizeof(struct frame_channel_video_buffer);
	q->ops = &frame_channel_vb2_qops;
	q->mem_ops = &frame_channel_vb2_memops;
	q->timestamp_type = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;

	ret = vb2_queue_init(q);
	if(ret){
		v4l2_err(v4l2_dev, "Failed to init vb2 queue\n");
		goto exit;
	}

	/* Initialize video */
	vfd = video_device_alloc();
	if (!vfd) {
		v4l2_err(v4l2_dev, "Failed to allocate frame channel video\n");
		ret = -ENOMEM;
		goto err_alloc_video;
	}

	sprintf(frame_channel_video.name, "isp-frame-channel%d",vdev->index);
	memcpy(vfd, &frame_channel_video, sizeof(frame_channel_video));
	vfd->lock = &vdev->mlock;
	vfd->v4l2_dev = v4l2_dev;
//	vfd->debug = V4L2_DEBUG_IOCTL | V4L2_DEBUG_IOCTL_ARG;
	vfd->debug =0;

	set_bit(V4L2_FL_USE_FH_PRIO, &(vfd->flags)); // add lately

	vdev->video = vfd;

	v4l2_disable_ioctl_locking(vfd, VIDIOC_DQBUF);
	v4l2_disable_ioctl_locking(vfd, VIDIOC_QBUF);
	v4l2_disable_ioctl_locking(vfd, VIDIOC_DEFAULT_CMD_LISTEN_BUF);

	ret = video_register_device(vfd, VFL_TYPE_GRABBER, -1);
	if (ret < 0) {
		ISP_PRINT(ISP_ERROR,"Failed to register video device\n");
		goto free_video_device;
	}

	vdev->interrupt_service_routine = frame_channel_video_irq_notify;

	video_set_drvdata(vfd, vdev);

	set_bitmap(vdev->index);

	atomic_set(&vdev->state, TX_ISP_STATE_STOP);
#if 0
	/* init v4l2_priority */
	v4l2_prio_init(&camdev->prio);
#endif
	return 0;
free_video_device:
	video_device_release(vfd);
err_alloc_video:
	vb2_queue_release(q);
exit:
	return ret;
}

void tx_isp_frame_channel_device_unregister(frame_chan_vdev_t *vdev)
{
	video_unregister_device(vdev->video);
	vb2_queue_release(&vdev->vbq);
	clear_bitmap(vdev->index);
}

#if 0
static int frame_channel_camera_suspend(struct device *dev)
{
	struct frame_channel_camera_dev *camdev = dev_get_drvdata(dev);

	isp_dev_call(camdev->isp, suspend, NULL);

	return 0;
}

static int frame_channel_camera_resume(struct device *dev)
{
	struct frame_channel_camera_dev *camdev = dev_get_drvdata(dev);

	isp_dev_call(camdev->isp, resume, NULL);

	return 0;
}
#endif
