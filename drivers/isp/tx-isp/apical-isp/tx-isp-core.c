#include <linux/module.h>
#include <linux/delay.h>
#include <media/v4l2-common.h>
#include <linux/v4l2-mediabus.h>
#include <asm/mipsregs.h>
#include <linux/mm.h>
#include <linux/clk.h>

#include "tx-isp-core.h"
#include "../tx-isp-video-in.h"

#include <apical-isp/apical_math.h>
#include "system_i2c.h"
#include <apical-isp/apical_isp_io.h>
#include <apical-isp/system_io.h>
#include <apical-isp/apical_configuration.h>
#include <apical-isp/system_interrupts.h>
#include <apical-isp/apical_isp_config.h>
#include "system_semaphore.h"
#include "apical_command_api.h"
#include <apical-isp/apical_isp_core_nomem_settings.h>
#include "apical_scaler_lut.h"
#include "sensor_drv.h"
#include <apical-isp/apical_firmware_config.h>
#include "tx-isp-core-tuning.h"

#if ISP_HAS_CONNECTION_DEBUG
#include "apical_cmd_interface.h"
#endif

system_tab stab ;
#define SOFT_VERSION "H201611150900"
#define FIRMWARE_VERSION "H01-380"

#if defined(CONFIG_SOC_T10)
static int isp_clk = ISP_CLK_960P_MODE;
#elif defined(CONFIG_SOC_T20)
static int isp_clk = ISP_CLK_1080P_MODE;
#endif
module_param(isp_clk, int, S_IRUGO);
MODULE_PARM_DESC(isp_clk, "isp core clock");

/*
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   manager the buffer of frame channels
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 */
static inline void init_buffer_fifo(struct list_head *fifo)
{
	INIT_LIST_HEAD(fifo);
}

static struct frame_channel_buffer *pop_buffer_fifo(struct list_head *fifo)
{
	struct frame_channel_buffer *buf;
	if(!list_empty(fifo)){
		buf = list_first_entry(fifo, struct frame_channel_buffer, entry);
		//		printk("^-^ %s %d %p %p^-^\n",__func__,__LINE__, fifo, buf);
		list_del(&(buf->entry));
	}else
		buf = NULL;

	return buf;
}

static void inline push_buffer_fifo(struct list_head *fifo, struct frame_channel_buffer *buf)
{
	//	printk("^@^ %s %d %p %p^-^\n",__func__,__LINE__, fifo, buf);
	list_add_tail(&(buf->entry), fifo);
}

static void inline cleanup_buffer_fifo(struct list_head *fifo)
{
	struct frame_channel_buffer *buf;
	while(!list_empty(fifo)){
		buf = list_first_entry(fifo, struct frame_channel_buffer, entry);
		list_del(&(buf->entry));
	}
}

/*
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   interrupt handler
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 */
static system_interrupt_handler_t isr_func[APICAL_IRQ_COUNT] = {NULL};
static void* isr_param[APICAL_IRQ_COUNT] = {NULL};
//static int interrupt_line[APICAL_IRQ_COUNT] = {0};
static struct v4l2_subdev *use_to_intc_sd = NULL;

static void inline isp_set_interrupt_ops(struct v4l2_subdev *sd)
{
	use_to_intc_sd = sd;
}
static inline unsigned short isp_intc_state(void)
{
	return apical_isp_interrupts_interrupt_status_read();
}

void system_program_interrupt_event(uint8_t event, uint8_t id)
{
	switch(event)
	{
		case 0: apical_isp_interrupts_interrupt0_source_write(id); break;
		case 1: apical_isp_interrupts_interrupt1_source_write(id); break;
		case 2: apical_isp_interrupts_interrupt2_source_write(id); break;
		case 3: apical_isp_interrupts_interrupt3_source_write(id); break;
		case 4: apical_isp_interrupts_interrupt4_source_write(id); break;
		case 5: apical_isp_interrupts_interrupt5_source_write(id); break;
		case 6: apical_isp_interrupts_interrupt6_source_write(id); break;
		case 7: apical_isp_interrupts_interrupt7_source_write(id); break;
		case 8: apical_isp_interrupts_interrupt8_source_write(id); break;
		case 9: apical_isp_interrupts_interrupt9_source_write(id); break;
		case 10: apical_isp_interrupts_interrupt10_source_write(id); break;
		case 11: apical_isp_interrupts_interrupt11_source_write(id); break;
		case 12: apical_isp_interrupts_interrupt12_source_write(id); break;
		case 13: apical_isp_interrupts_interrupt13_source_write(id); break;
		case 14: apical_isp_interrupts_interrupt14_source_write(id); break;
		case 15: apical_isp_interrupts_interrupt15_source_write(id); break;
	}
}
static inline void isp_clear_irq_source(void)
{
	int event = 0;
	for(event = 0; event < APICAL_IRQ_COUNT; event++){
		system_program_interrupt_event(event, 0);
	}
}

void system_set_interrupt_handler(uint8_t source,
		system_interrupt_handler_t handler, void* param)
{
	isr_func[source] = handler;
	isr_param[source] = param;
}
void system_init_interrupt(void)
{
	int i;
	for (i = 0; i < APICAL_IRQ_COUNT; i++)
	{
		isr_func[i] = NULL;
		isr_param[i] = NULL;
	}

	isp_clear_irq_source();
}

void system_hw_interrupts_enable(void)
{
	struct tx_isp_notify_argument arg;
	arg.value = TX_ISP_TOP_IRQ_ISP;
	use_to_intc_sd->v4l2_dev->notify(use_to_intc_sd, TX_ISP_NOTIFY_ENABLE_IRQ, &arg);
}

void system_hw_interrupts_disable(void)
{
	struct tx_isp_notify_argument arg;
	arg.value = TX_ISP_TOP_IRQ_ISP;
	use_to_intc_sd->v4l2_dev->notify(use_to_intc_sd, TX_ISP_NOTIFY_DISABLE_IRQ, &arg);
}

/*
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   v4l2_subdev_ops will be defined as follows
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 */
static int isp_set_buffer_lineoffset_vflip_disable(struct tx_isp_frame_channel *chan)
{
	frame_chan_vdev_t *video = &chan->video;
	struct v4l2_format *output = &(video->attr.output);
	unsigned int regw = 0xb00;

	switch(output->fmt.pix.pixelformat){
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV21:
			APICAL_WRITE_32(regw + 0xa0 + 0x100 * (video->index), video->attr.lineoffset);//lineoffset
			//	printk("(offset = 0x%08x)\n",offset);
		default:
			APICAL_WRITE_32(regw + 0x20 + 0x100 * (video->index), video->attr.lineoffset);//lineoffset
			break;
	}
	return 0;
}
static int isp_set_buffer_lineoffset_vflip_enable(struct tx_isp_frame_channel *chan)
{
	frame_chan_vdev_t *video = &chan->video;
	struct v4l2_format *output = &(video->attr.output);
	unsigned int regw = 0xb00;
	switch(output->fmt.pix.pixelformat){
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV21:
			/* UV */
			APICAL_WRITE_32(regw + 0xa0 + 0x100 * (video->index), -video->attr.lineoffset);//lineoffset
			/* Y */
			APICAL_WRITE_32(regw + 0x20 + 0x100 * (video->index), -video->attr.lineoffset);//lineoffset
			break;
		default:
			APICAL_WRITE_32(regw + 0x20 + 0x100 * (video->index), -video->attr.lineoffset);//lineoffset
			break;
	}
	return 0;
}

static int isp_set_buffer_address_vflip_disable(struct tx_isp_frame_channel *chan, struct frame_channel_buffer *buf, unsigned char bank_id)
{
	frame_chan_vdev_t *video = &chan->video;
	struct v4l2_format *output = &(video->attr.output);
	unsigned int regw = 0xb00;
	unsigned int offset;

	switch(output->fmt.pix.pixelformat){
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV21:
			offset = output->fmt.pix.width * ((output->fmt.pix.height + 0xf) & ~0xf);
			APICAL_WRITE_32((regw + 0x88 + 0x100 * (video->index) + 0x04 * bank_id), buf->addr + offset);
			//	printk("(offset = 0x%08x)\n",offset);
		default:
			APICAL_WRITE_32((regw + 0x08 + 0x100 * (video->index) + 0x04 * bank_id), buf->addr);
			break;
	}
	return 0;
}
static int isp_set_buffer_address_vflip_enable(struct tx_isp_frame_channel *chan, struct frame_channel_buffer *buf, unsigned char bank_id)
{
	frame_chan_vdev_t *video = &chan->video;
	struct v4l2_format *output = &(video->attr.output);
	unsigned int regw = 0xb00;
	unsigned int offset;
	unsigned int offset_uv;
	switch(output->fmt.pix.pixelformat){
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV21:
			/* UV */
			offset = output->fmt.pix.width * output->fmt.pix.height;
			offset_uv = (output->fmt.pix.height & 0xf) ? output->fmt.pix.width * (0x10 - (output->fmt.pix.height & 0xf)) :0; /* align at 16 lines */
			APICAL_WRITE_32((regw + 0x88 + 0x100 * (video->index) + 0x04 * bank_id),
											buf->addr + output->fmt.pix.sizeimage + offset_uv - video->attr.lineoffset);
			/* Y */
			APICAL_WRITE_32((regw + 0x08 + 0x100 * (video->index) + 0x04 * bank_id), buf->addr + offset - video->attr.lineoffset);
			break;
		default:
			APICAL_WRITE_32((regw + 0x08 + 0x100 * (video->index) + 0x04 * bank_id), buf->addr + output->fmt.pix.sizeimage - video->attr.lineoffset);
			break;
	}
	return 0;
}

static int isp_enable_dma_transfer(struct tx_isp_frame_channel *chan, int onoff)
{
	frame_chan_vdev_t *video = &chan->video;
	struct v4l2_format *output = &(video->attr.output);
	unsigned int regw = 0xb00;
	//	unsigned int primary;
	//	printk("^~^ %s[%d] index = %d onoff = %d ^~^\n",__func__, __LINE__, video->index, onoff);
	switch(output->fmt.pix.pixelformat){
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV21:
			if(onoff)
				APICAL_WRITE_32(regw + 0xa4 + 0x100*(video->index), 0x02);// axi_port_enable set 1, frame_write_cancel set 0.
			else
				APICAL_WRITE_32(regw + 0xa4 + 0x100*(video->index), 0x03);// axi_port_enable set 1, frame_write_cancel set 0.

		default:
			if(onoff)
				APICAL_WRITE_32(regw + 0x24 + 0x100*(video->index), 0x02);// axi_port_enable set 1, frame_write_cancel set 0.
			else
				APICAL_WRITE_32(regw + 0x24 + 0x100*(video->index), 0x03);// axi_port_enable set 1, frame_write_cancel set 0.
			//	printk("[%s]dma enable(0x%08x is wrote 0x%08x)\n",onoff ? "on":"off",regw + 0x100*(video->index), primary);
			break;
	}
	return 0;
}

static int isp_configure_base_addr(struct tx_isp_core_device *core)
{
	struct tx_isp_frame_channel *chan;
	struct frame_channel_buffer *buf;
	unsigned int hw_dma = 0;
	unsigned char current_bank = 0;
	unsigned char bank_id = 0;
	unsigned char i = 0;
	int index = 0;
	for(index = 0; index < ISP_MAX_OUTPUT_VIDEOS; index++){
		chan = &(core->chans[index]);
		if(chan->state == TX_ISP_STATE_RUN){
			hw_dma = APICAL_READ_32(0xb24 + 0x100*index);
			current_bank = (hw_dma >> 8) & 0x7;
			/* The begin pointer is next bank. */
			for(i = 0, bank_id = current_bank; i < chan->usingbanks; i++, bank_id++){
				bank_id = bank_id % chan->usingbanks;
				if(chan->bank_flag[bank_id] == 0){
					buf = pop_buffer_fifo(&chan->fifo);
					if(buf != NULL){
						if(core->vflip_state){
							isp_set_buffer_address_vflip_enable(chan, buf, bank_id);
						}else{
							isp_set_buffer_address_vflip_disable(chan, buf, bank_id);
						}
						chan->vflip_flag[bank_id] = core->vflip_state;
						chan->bank_flag[bank_id] = 1;
						chan->banks_addr[bank_id] = buf->addr;
					}else
						break;
				}
			}
		}
	}
	return 0;
}

static inline int isp_enable_channel(struct tx_isp_frame_channel *chan)
{
	unsigned int hw_dma = 0;
	/*unsigned char current_bank = 0;*/
	unsigned char next_bank = 0;
	hw_dma = APICAL_READ_32(0xb24 + 0x100 * (chan->video.index));
	next_bank = (((hw_dma >> 8) & 0x7) + 1) % chan->usingbanks;
	/*current_bank = (hw_dma >> 8) & 0x7;*/
	if(chan->bank_flag[next_bank] ^ chan->dma_state){
		chan->dma_state = chan->bank_flag[next_bank];
		isp_enable_dma_transfer(chan, chan->dma_state);
	}
	return 0;
}

static int isp_modify_dma_direction(struct tx_isp_frame_channel *chan)
{
	unsigned int hw_dma = 0;
	unsigned char next_bank = 0;
	if(chan->state == TX_ISP_STATE_RUN){
		hw_dma = APICAL_READ_32(0xb24 + 0x100 * (chan->video.index));
		next_bank = (((hw_dma >> 8) & 0x7) + 1) % chan->usingbanks;
		if(chan->vflip_flag[next_bank] ^ chan->vflip_state){
			chan->vflip_state = chan->vflip_flag[next_bank];
			if(chan->vflip_state){
				isp_set_buffer_lineoffset_vflip_enable(chan);
			}else{
				isp_set_buffer_lineoffset_vflip_disable(chan);
			}
		}
	}
	return 0;
}

static inline void cleanup_chan_banks(struct tx_isp_frame_channel *chan)
{
	int bank_id = 0;
	while(bank_id < chan->usingbanks){
		if(chan->bank_flag[bank_id]){
			chan->video.interrupt_service_routine(&chan->video, chan->banks_addr[bank_id], NULL);
			chan->bank_flag[bank_id] = 0;
		}
		bank_id++;
	}
}

#define ISP_CHAN_DMA_STAT (1<<16)
#define ISP_CHAN_DMA_ACTIVE (1<<16)
static inline void isp_core_update_addr(struct tx_isp_frame_channel *chan)
{
	frame_chan_vdev_t *video = &chan->video;
	struct v4l2_format *output = &(video->attr.output);
	unsigned int y_hw_dma = 0;
	unsigned int uv_hw_dma = 0;
	unsigned char current_bank = 0;
	unsigned char uv_bank = 0;
	unsigned char last_bank = 0;
	unsigned char next_bank = 0;
	unsigned char bank_id = 0;
	unsigned int current_active = 0;
	unsigned int value = 0;

	switch(output->fmt.pix.pixelformat){
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV21:
			uv_hw_dma = APICAL_READ_32(0xba4 + 0x100 * video->index);
			current_active |= uv_hw_dma;
		default:
			y_hw_dma = APICAL_READ_32(0xb24 + 0x100 * video->index);
			current_active |= y_hw_dma;
			break;
	}

	current_bank = (y_hw_dma >> 8) & 0x7;
	uv_bank = (uv_hw_dma >> 8) & 0x7;

	if(uv_bank != current_bank){
		/*printk("##### y_bank = %d, nv_bank = %d\n",current_bank,uv_bank);*/
		value = (0x1<<3) | (chan->usingbanks - 1);
		switch(output->fmt.pix.pixelformat){
			case V4L2_PIX_FMT_NV12:
			case V4L2_PIX_FMT_NV21:
				APICAL_WRITE_32(0xb9c + 0x100*(video->index), value);
			default:
				APICAL_WRITE_32(0xb1c + 0x100*(video->index), value);
				break;
		}
		chan->reset_dma_flag = 1;
		return;
	}
	if(chan->reset_dma_flag){
		/*printk("##### y_bank = %d, nv_bank = %d\n",current_bank,uv_bank);*/
		value = (0x0<<3) | (chan->usingbanks - 1);
		switch(output->fmt.pix.pixelformat){
			case V4L2_PIX_FMT_NV12:
			case V4L2_PIX_FMT_NV21:
				APICAL_WRITE_32(0xb9c + 0x100*(video->index), value);
			default:
				APICAL_WRITE_32(0xb1c + 0x100*(video->index), value);
				break;
		}
		chan->reset_dma_flag = 0;
	}

	if((current_active & ISP_CHAN_DMA_STAT) == ISP_CHAN_DMA_ACTIVE){
		last_bank = (y_hw_dma >> 11) & 0x7;
		bank_id = last_bank;
	}else{
		bank_id = current_bank;
	}
	if(chan->bank_flag[bank_id]){
		chan->video.interrupt_service_routine(&chan->video, chan->banks_addr[bank_id], NULL);
		chan->bank_flag[bank_id] = 0;
	}

	next_bank = (current_bank + 1) % chan->usingbanks;
	if(chan->bank_flag[next_bank] != 1){
		isp_enable_channel(chan);
	}
	return;
}

extern int apical_isp_day_or_night_s_ctrl_internal(struct tx_isp_core_device *core);

static int isp_core_interrupt_service_routine(struct v4l2_subdev *sd, u32 status, bool *handled)
{
	struct tx_isp_core_device *core = sd_to_tx_isp_core_device(sd);
	struct apical_isp_contrl *contrl = &core->contrl;
	struct tx_isp_frame_channel *chan;
	unsigned short isp_irq_status = 0;
	unsigned char color = apical_isp_top_rggb_start_read();
	unsigned int i = 0;
	int ret = IRQ_HANDLED;
	if((isp_irq_status = isp_intc_state()) != 0)
	{
		apical_isp_interrupts_interrupt_clear_write(0);
		apical_isp_interrupts_interrupt_clear_write(isp_irq_status);
		/* printk("0xb00 = 0x%0x state = 0x%x\n", APICAL_READ_32(0xb00), isp_irq_status); */
		for(i = 0; i < APICAL_IRQ_COUNT; i++){
			if(isp_irq_status & (1 << i)){
				switch(i){
					case APICAL_IRQ_FRAME_START:
						if(isr_func[i])
							isr_func[i](isr_param[i]);
						/*printk("^~^ frame start ^~^\n");*/
						isp_configure_base_addr(core);
						core->frame_state = 1;
						ret = IRQ_WAKE_THREAD;
						break;
					case APICAL_IRQ_FRAME_WRITER_FR:
						if(isr_func[i])
							isr_func[APICAL_IRQ_FRAME_WRITER_FR](isr_param[APICAL_IRQ_FRAME_WRITER_FR]);
						chan = &core->chans[ISP_FR_VIDEO_CHANNEL];
						isp_core_update_addr(chan);
						isp_modify_dma_direction(chan);
						break;
					case APICAL_IRQ_FRAME_WRITER_DS:
						if(isr_func[i])
							isr_func[APICAL_IRQ_FRAME_WRITER_DS](isr_param[APICAL_IRQ_FRAME_WRITER_DS]);
						chan = &core->chans[ISP_DS1_VIDEO_CHANNEL];
						isp_core_update_addr(chan);
						isp_modify_dma_direction(chan);
						break;
					case APICAL_IRQ_FRAME_WRITER_DS2:
						if(isr_func[i])
							isr_func[i](isr_param[i]);
						#if TX_ISP_EXIST_DS2_CHANNEL
						chan = &core->chans[ISP_DS2_VIDEO_CHANNEL];
						isp_core_update_addr(chan);
						isp_modify_dma_direction(chan);
						#endif
						break;
					case APICAL_IRQ_FRAME_END:
						if(core->hflip_state == apical_isp_top_bypass_mirror_read())						{
							if(core->hflip_state){
								color ^= 1;
							}else{
								if(contrl->pattern != color){
									color ^= 1;
								}
							}
							//	printk("$$$$ %s %d pattern = %d color = %d $$$$$\n", __func__,__LINE__,contrl->pattern,color);
							apical_isp_top_rggb_start_write(color);
							apical_isp_top_bypass_mirror_write(core->hflip_state ?0:1);
						}
						/* APICAL_WRITE_32(0x18,2);  */
						chan = &core->chans[ISP_FR_VIDEO_CHANNEL];
						core->frame_state = 0;
						isp_configure_base_addr(core);
						isp_modify_dma_direction(chan);
						if(chan->dma_state != 1){
							isp_enable_channel(chan);
						}

						if (1 == core->isp_daynight_switch) {
							int ret = 0;
							ret = apical_isp_day_or_night_s_ctrl_internal(core);
							if (ret)
								printk("%s[%d] apical_isp_day_or_night_s_ctrl_internal failed!\n", __func__, __LINE__);
							core->isp_daynight_switch = 0;
						}
					case APICAL_IRQ_AE_STATS:
					case APICAL_IRQ_AWB_STATS:
					case APICAL_IRQ_AF_STATS:
					case APICAL_IRQ_FPGA_FRAME_START:
					case APICAL_IRQ_FPGA_FRAME_END:
					case APICAL_IRQ_FPGA_WDR_BUF:
						if(isr_func[i])
							isr_func[i](isr_param[i]);
						break;
					case APICAL_IRQ_DS1_OUTPUT_END:
						chan = &core->chans[ISP_DS1_VIDEO_CHANNEL];
						isp_modify_dma_direction(chan);
						if(chan->dma_state != 1){
							isp_enable_channel(chan);
						}
						break;
					#if TX_ISP_EXIST_DS2_CHANNEL
					case APICAL_IRQ_DS2_OUTPUT_END:
						chan = &core->chans[ISP_DS2_VIDEO_CHANNEL];
						isp_modify_dma_direction(chan);
						if(chan->dma_state != 1){
							isp_enable_channel(chan);
						}
						break;
					#endif
					default:
						break;
				}
			}
		}
		/*printk("^~^ intc end ^~^\n");*/
	}
	return ret;
}

irqreturn_t isp_irq_thread_handle(int this_irq, void *dev)
{
	tx_isp_device_t *ispdev = (tx_isp_device_t *)dev;
	struct tx_isp_media_pipeline *p = &ispdev->pipeline;
	struct tx_isp_core_device *core = NULL;
	struct v4l2_subdev *sd = NULL;
	struct isp_private_ioctl ioctl;
	struct tx_isp_notify_argument arg;
	int i = 0;

	sd = p->subdevs[TX_ISP_CORE_GRP_IDX];
	core = v4l2_get_subdevdata(sd);
	if(core){
		ioctl.dir = TX_ISP_PRIVATE_IOCTL_SET;
		for(i = 0; i < TX_ISP_I2C_SET_BUTTON; i++){
			if(core->i2c_msgs[i].flag == 0)
				continue;
			core->i2c_msgs[i].flag = 0;
			switch(i){
				case TX_ISP_I2C_SET_AGAIN:
					ioctl.cmd = TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN;
					break;
				case TX_ISP_I2C_SET_DGAIN:
					ioctl.cmd = TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN;
					break;
				case TX_ISP_I2C_SET_INTEGRATION:
					ioctl.cmd = TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME;
					break;
				default:
					break;
			}
			ioctl.value = core->i2c_msgs[i].value;
			arg.value = (int)&ioctl;
			tx_isp_sd_notify(sd, TX_ISP_NOTIFY_PRIVATE_IOCTL, &arg);
		}
	}
	return IRQ_HANDLED;
}

/*
 * the ops of isp's clks
 */
//#ifdef CONFIG_PRODUCT_FPGA
#if 0
static int isp_core_init_clk(struct tx_isp_core_device *core)
{
	*(volatile unsigned int*)(0xb0000080) = 0x20000007;
	while(*(volatile unsigned int*)(0xb0000080) & (1 << 28));
	return 0;
}
static long isp_core_set_clk(struct tx_isp_core_device *core, int state)
{
	*(volatile unsigned int*)(0xb0000020) &= ~(1 << 23);
	return 0;
}
static int isp_core_release_clk(struct tx_isp_core_device *core)
{
	return 0;
}
#else
static int isp_core_init_clk(struct tx_isp_core_device *core)
{
	struct clk **clks = NULL;
	struct v4l2_device *v4l2_dev = core->sd.v4l2_dev;
	struct tx_isp_subdev_platform_data *pdata = core->pdata;
	struct tx_isp_subdev_clk_info *info = pdata->clks;
	unsigned int num = pdata->clk_num;
	int ret = 0;
	int i;

	core->clk_num = pdata->clk_num;
	if(!core->clk_num){
		core->clks = NULL;
		return ISP_SUCCESS;
	}

	clks = (struct clk **)kzalloc(sizeof(clks) * num, GFP_KERNEL);
	if(!clks){
		v4l2_err(v4l2_dev, "%s[%d] Failed to allocate core's clks\n",__func__,__LINE__);
		ret = -ENOMEM;
		goto exit;
	}
	for (i = 0; i < num; i++) {
		clks[i] = clk_get(core->dev, info[i].name);
		if (IS_ERR(clks[i])) {
			v4l2_err(v4l2_dev, "Failed to get %s clock %ld\n", info[i].name, PTR_ERR(clks[i]));
			ret = PTR_ERR(clks[i]);
			goto failed_to_get_clk;
		}
		if(info[i].rate != DUMMY_CLOCK_RATE) {
			ret = clk_set_rate(clks[i], isp_clk);
			if(ret){
				v4l2_err(v4l2_dev, "Failed to set %s clock rate(%ld)\n", info[i].name, info[i].rate);
				goto failed_to_get_clk;
			}
		}
	}
	core->clks = clks;
	return ISP_SUCCESS;
failed_to_get_clk:
	while(--i >= 0){
		clk_put(clks[i]);
	}
	kfree(clks);
exit:
	return ret;
}

static int isp_core_release_clk(struct tx_isp_core_device *core)
{
	struct clk **clks = core->clks;
	int i = 0;

	if(clks == NULL)
		return 0;
	for (i = 0; i < core->clk_num; i++)
		clk_put(clks[i]);

	kfree(clks);
	return 0;
}


static long isp_core_set_clk(struct tx_isp_core_device *core, int state)
{
	struct clk **clks = core->clks;
	int i;

	if(state){
		for(i = 0; i < core->clk_num; i++)
			clk_enable(clks[i]);
	}else{
		for(i = core->clk_num - 1; i >=0; i--)
			clk_disable(clks[i]);
	}
	return ISP_SUCCESS;
}
#endif
static int isp_fw_process(void *data)
{
	while(!kthread_should_stop()){
		apical_process();
#if ISP_HAS_CONNECTION_DEBUG && ISP_HAS_API
		apical_cmd_process();
#endif //ISP_HAS_API
#if ISP_HAS_STREAM_CONNECTION
		apical_connection_process();
#endif
	}
#if ISP_HAS_STREAM_CONNECTION
	apical_connection_destroy();
#endif
	return 0;
}

static int isp_core_ops_init(struct v4l2_subdev *sd, u32 on)
{
	struct tx_isp_core_device *core = sd_to_tx_isp_core_device(sd);
	struct v4l2_device *v4l2_dev = core->sd.v4l2_dev;
	int ret = ISP_SUCCESS;

	if(on){
		if(atomic_read(&core->state) == TX_ISP_STATE_STOP){
			core->param = load_tx_isp_parameters(core->vin.attr);
			apical_init();
#if ISP_HAS_STREAM_CONNECTION
			apical_connection_init();
#endif
			system_program_interrupt_event(APICAL_IRQ_DS1_OUTPUT_END, 50);
#if TX_ISP_EXIST_DS2_CHANNEL
			system_program_interrupt_event(APICAL_IRQ_DS2_OUTPUT_END, 53);
#endif
			core->process_thread = kthread_run(isp_fw_process, NULL, "apical_isp_fw_process");
			if(IS_ERR_OR_NULL(core->process_thread)){
				v4l2_err(v4l2_dev, "%s[%d] kthread_run was failed!\n",__func__,__LINE__);
				ret = -ISP_ERROR;
				goto exit;
			}
			atomic_set(&core->state, TX_ISP_STATE_START);
		}
	}else{
		if(atomic_read(&core->state) == TX_ISP_STATE_RUN){
			v4l2_err(v4l2_dev, "%s[%d] Cann't deinit the device, please streamoff it firstly!\n",__func__,__LINE__);
			ret = -ISP_ERROR;
			goto exit;
		}
		if(atomic_read(&core->state) == TX_ISP_STATE_START){
			ret = kthread_stop(core->process_thread);
			isp_clear_irq_source();
			atomic_set(&core->state, TX_ISP_STATE_STOP);
			/*printk("^^^^ %s[%d] ^^^^\n",__func__,__LINE__);*/
		}
	}
exit:
	return ret;
}

static int isp_core_ops_reset(struct v4l2_subdev *sd, u32 enable)
{
	return ISP_SUCCESS;
}

static int isp_core_ops_s_power(struct v4l2_subdev *sd, int on)
{
	return ISP_SUCCESS;
}

static void isp_core_config_top_ctl_register(unsigned int name, unsigned int value)
{
	apical_isp_top_ctl_t top;
	top.reg.low = APICAL_READ_32(0x40);
	top.reg.high = APICAL_READ_32(0x44);

	switch(name){
		case TEST_GEN_BIT:
			top.bits.test_gen = value;
			break;
		case MIRROR_BIT:
			top.bits.mirror = value;
			break;
		case SENSOR_OFFSET_BIT:
			top.bits.sensor_offset = value;
			break;
		case DIG_GAIN_BIT:
			top.bits.dig_gain = value;
			break;
		case GAMMA_FE_BIT:
			top.bits.gamma_fe = value;
			break;
		case RAW_FRONT_BIT:
			top.bits.raw_front = value;
			break;
		case DEFECT_PIXEL_BIT:
			top.bits.defect_pixel = value;
			break;
		case FRAME_STITCH_BIT:
			top.bits.frame_stitch = value;
			break;
		case GAMMA_FE_POS_BIT:
			top.bits.gamma_fe_pos = value;
			break;
		case SINTER_BIT:
			top.bits.sinter = value;
			break;
		case TEMPER_BIT:
			top.bits.temper = value;
			break;
		case ORDER_BIT:
			top.bits.order = value;
			break;
		case WB_BIT:
			top.bits.wb = value;
			break;
		case RADIAL_BIT:
			top.bits.radial = value;
			break;
		case MESH_BIT:
			top.bits.mesh = value;
			break;
		case IRIDIX_BIT:
			top.bits.iridix = value;
			break;
		case DEMOSAIC_BIT:
			top.bits.demosaic = value;
			break;
		case MATRIX_BIT:
			top.bits.matrix = value;
			break;
		case FR_CROP_BIT:
			top.bits.fr_crop = value;
			break;
		case FR_GAMMA_BIT:
			top.bits.fr_gamma = value;
			break;
		case FR_SHARPEN_BIT:
			top.bits.fr_sharpen = value;
			break;
		case FR_LOGO_BIT:
			top.bits.fr_logo = value;
			break;
		case FR_CSC_BIT:
			top.bits.fr_csc = value;
			break;
		case DS1_CROP_BIT:
			top.bits.ds1_crop = value;
			break;
		case DS1_SCALER_BIT:
			top.bits.ds1_scaler = value;
			break;
		case DS1_GAMMA_BIT:
			top.bits.ds1_gamma = value;
			break;
		case DS1_SHARPEN_BIT:
			top.bits.ds1_sharpen = value;
			break;
		case DS1_LOGO_BIT:
			top.bits.ds1_logo = value;
			break;
		case DS1_CSC_BIT:
			top.bits.ds1_csc = value;
			break;
		case DS1_DMA_BIT:
			top.bits.ds1_dma = value;
			break;
		case DS2_SCALER_SOURCE_BIT:
			top.bits.ds2_scaler_source = value;
			break;
		case DS2_CROP_BIT:
			top.bits.ds2_crop = value;
			break;
		case DS2_SCALER_BIT:
			top.bits.ds2_scaler = value;
			break;
		case DS2_GAMMA_BIT:
			top.bits.ds2_gamma = value;
			break;
		case DS2_SHARPEN_BIT:
			top.bits.ds2_sharpen = value;
			break;
		case DS2_LOGO_BIT:
			top.bits.ds2_logo = value;
			break;
		case DS2_CSC_BIT:
			top.bits.ds2_csc = value;
			break;
		case RAW_BYPASS_BIT:
			top.bits.raw_bypass = value;
			break;
		case DEBUG_BIT:
			top.bits.debug = value;
			break;
		case PROC_BYPASS_MODE_BIT:
			top.bits.proc_bypass_mode = value;
			break;
		default:
			break;
	}

	APICAL_WRITE_32(0x40, top.reg.low);
	APICAL_WRITE_32(0x44, top.reg.high);
}

static int isp_core_config_dma_channel_write(struct tx_isp_core_device *core,
		frame_chan_vdev_t *vdev)
{
	struct apical_isp_contrl *contrl = &core->contrl;
	struct frame_channel_attribute *attr = &vdev->attr;
	struct v4l2_format *output = &(attr->output);
	struct v4l2_mbus_framefmt *mbus = &core->vin.mbus;
	struct frame_channel_format *cfmt = (struct frame_channel_format *)(output->fmt.pix.priv);
	unsigned int base = 0x00b00 + 0x100 * vdev->index; // the base of address of dma channel write
	apical_api_control_t api;
	unsigned char status = 0;
	int ret = ISP_SUCCESS;
	unsigned int csc = 0xf;

	switch(output->fmt.pix.pixelformat){
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV21:
			/* dma channel uv write */
			APICAL_WRITE_32(base + 0x80, cfmt->priv);
			APICAL_WRITE_32(base + 0x84, output->fmt.pix.height << 16 | output->fmt.pix.width);
			APICAL_WRITE_32(base + 0xa0, vdev->attr.lineoffset);//lineoffset
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_UYVY:
		case V4L2_PIX_FMT_YUV444:
			switch(vdev->index){
				case ISP_FR_VIDEO_CHANNEL:
					if(mbus->code == V4L2_MBUS_FMT_YUYV8_1X16)
						csc = 0x08;
					else
						csc = 0x0f;

					isp_core_config_top_ctl_register(FR_CSC_BIT,
							ISP_MODULE_BYPASS_DISABLE);
					break;
				case ISP_DS1_VIDEO_CHANNEL:
					csc = 0x0f;
					isp_core_config_top_ctl_register(DS1_CSC_BIT,
							ISP_MODULE_BYPASS_DISABLE);
					break;
				#if TX_ISP_EXIST_DS2_CHANNEL
				case ISP_DS2_VIDEO_CHANNEL:
					csc = 0x0f;
					isp_core_config_top_ctl_register(DS2_CSC_BIT,
							ISP_MODULE_BYPASS_DISABLE);
					break;
				#endif
				default:
					break;
			}
			if(contrl->fmt_end == APICAL_ISP_INPUT_RAW_FMT_INDEX_END){
				isp_core_config_top_ctl_register(DEMOSAIC_BIT, ISP_MODULE_BYPASS_DISABLE);
			}
			APICAL_WRITE_32(base, cfmt->priv & 0x0f);
			APICAL_WRITE_32(base + 0x04, output->fmt.pix.height << 16 | output->fmt.pix.width);
			APICAL_WRITE_32(base + 0x20, vdev->attr.lineoffset);//lineoffset
			break;
		case V4L2_PIX_FMT_RGB565:
		case V4L2_PIX_FMT_RGB24:
		case V4L2_PIX_FMT_RGB310:
			switch(vdev->index){
				case ISP_FR_VIDEO_CHANNEL:
					isp_core_config_top_ctl_register(FR_CSC_BIT,
							ISP_MODULE_BYPASS_ENABLE);
					break;
				case ISP_DS1_VIDEO_CHANNEL:
					isp_core_config_top_ctl_register(DS1_CSC_BIT,
							ISP_MODULE_BYPASS_ENABLE);
					break;
				#if TX_ISP_EXIST_DS2_CHANNEL
				case ISP_DS2_VIDEO_CHANNEL:
					isp_core_config_top_ctl_register(DS2_CSC_BIT,
							ISP_MODULE_BYPASS_ENABLE);
					break;
				#endif
				default:
					break;
			}
			if(contrl->fmt_end == APICAL_ISP_INPUT_RAW_FMT_INDEX_END){
				isp_core_config_top_ctl_register(DEMOSAIC_BIT, ISP_MODULE_BYPASS_DISABLE);
			}
		case V4L2_PIX_FMT_SBGGR12:
		case V4L2_PIX_FMT_SGBRG12:
		case V4L2_PIX_FMT_SGRBG12:
		case V4L2_PIX_FMT_SRGGB12:
			APICAL_WRITE_32(base, cfmt->priv & 0x0f);
			APICAL_WRITE_32(base + 0x04, output->fmt.pix.height << 16 | output->fmt.pix.width);
			APICAL_WRITE_32(base + 0x20, vdev->attr.lineoffset);//lineoffset
			break;
		default:
			break;
	}

	api.type = TSCENE_MODES;
	api.dir = COMMAND_SET;
	api.id = -1;
	api.value = -1;
	switch(vdev->index){
		case ISP_FR_VIDEO_CHANNEL:
			api.id = FR_OUTPUT_MODE_ID;
			break;
		case ISP_DS1_VIDEO_CHANNEL:
			api.id = DS1_OUTPUT_MODE_ID;
			break;
#if TX_ISP_EXIST_DS2_CHANNEL
		case ISP_DS2_VIDEO_CHANNEL:
			api.id = DS2_OUTPUT_MODE_ID;
			break;
#endif
		default:
			ret = -EINVAL;
			break;
	}
	switch(output->fmt.pix.pixelformat){
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV21:
			api.value = YUV420;
			break;
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_UYVY:
			api.value = YUV422;
			break;
		case V4L2_PIX_FMT_YUV444:
			api.value = YUV444;
			break;
		case V4L2_PIX_FMT_RGB565:
		case V4L2_PIX_FMT_RGB24:
		case V4L2_PIX_FMT_RGB310:
			api.value = RGB;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	if(api.value != -1 && api.id != -1){
		status = apical_command(api.type, api.id, api.value, api.dir, &ret);
	}

	switch(vdev->index){
	case ISP_FR_VIDEO_CHANNEL:
		APICAL_WRITE_32(0x570, csc);
		break;
	case ISP_DS1_VIDEO_CHANNEL:
		APICAL_WRITE_32(0x6b0, csc);
		break;
#if TX_ISP_EXIST_DS2_CHANNEL
	case ISP_DS2_VIDEO_CHANNEL:
		APICAL_WRITE_32(0x7b0, csc);
		break;
#endif
	default:
		break;
	}

	return ISP_SUCCESS;
}

static struct frame_channel_format isp_output_fmt[APICAL_ISP_FMT_MAX_INDEX] = {
	{
		.name     = "YUV 4:2:0 semi planar, Y/CbCr",
		.fourcc   = V4L2_PIX_FMT_NV12,
		.depth    = 12,
		.priv     = DMA_FORMAT_NV12_UV,
	},
	{
		.name     = "YUV 4:2:0 semi planar, Y/CrCb",
		.fourcc   = V4L2_PIX_FMT_NV21,
		.depth    = 12,
		.priv     = DMA_FORMAT_NV12_VU,
	},
	{
		.name     = "YUV 4:2:2 packed, YCbYCr",
		.fourcc   = V4L2_PIX_FMT_YUYV,
		.depth    = 16,
		.priv     = DMA_FORMAT_YUY2,
	},
	{
		.name     = "YUV 4:2:2 packed, CbYCrY",
		.fourcc   = V4L2_PIX_FMT_UYVY,
		.depth    = 16,
		.priv	  = DMA_FORMAT_UYVY,
	},
	{
		.name     = "YUV 4:4:4 packed, YCbCr",
		.fourcc   = V4L2_PIX_FMT_YUV444,
		.depth    = 32,
		.priv     = DMA_FORMAT_AYUV,
	},
	{
		.name     = "RGB565, RGB-5-6-5",
		.fourcc   = V4L2_PIX_FMT_RGB565,
		.depth    = 16,
		.priv     = DMA_FORMAT_RGB565,
	},
	{
		.name     = "RGB24, RGB-8-8-8",
		.fourcc   = V4L2_PIX_FMT_RGB24,
		.depth    = 24,
		.priv     = DMA_FORMAT_RGB24,
	},
	{
		.name     = "RGB101010, RGB-10-10-10",
		.fourcc   = V4L2_PIX_FMT_RGB310,
		.depth    = 32,
		.priv     = DMA_FORMAT_RGB32,
	},
	/* the last member will be determined when isp input confirm.*/
	{
		.name     = "undetermine",
		.fourcc   = 0,
		.depth    = 0,
		.priv     = 0,
	},
};

static int isp_core_frame_channel_enum_fmt (struct tx_isp_core_device *core, int value)
{
	struct apical_isp_contrl *contrl = &core->contrl;
	frame_chan_vdev_t *vdev = (frame_chan_vdev_t *)value;
	struct v4l2_fmtdesc *f = vdev->fmtdesc;
	struct frame_channel_format *cfmt;
	int index = 0;
	//	printk("contrl->fmt_start + f->index============%d\n",contrl->fmt_start + f->index);

	if(core->bypass == TX_ISP_FRAME_CHANNEL_BYPASS_ISP_ENABLE){
		index = contrl->fmt_end;
	}else{
		index = contrl->fmt_start;
	}

	if(index + f->index <= contrl->fmt_end){
		cfmt = &(isp_output_fmt[index + f->index]);
	}else{
		return -ISP_ERROR;
	}
	strlcpy(f->description, cfmt->name, sizeof(f->description));
	f->pixelformat = cfmt->fourcc;
	return ISP_SUCCESS;
}

static int isp_core_frame_channel_try_fmt (struct tx_isp_core_device *core, int value)
{
	struct apical_isp_contrl *contrl = &core->contrl;
	frame_chan_vdev_t *vdev = (frame_chan_vdev_t *)value;
	struct v4l2_format *f = vdev->fmt;
	struct frame_channel_format *cfmt;
	int i = 0;
	if(core->bypass == TX_ISP_FRAME_CHANNEL_BYPASS_ISP_ENABLE){
		i = contrl->fmt_end;
	}else{
		i = contrl->fmt_start;
	}
	//	printk("^^^^ %s[%d] i = %d^^^^\n",__func__,__LINE__,i);
	for(; i <= contrl->fmt_end; i++){
		cfmt = &(isp_output_fmt[i]);
		//	printk("^^^^ %s[%d] i = %d  0x%08x^^^^\n",__func__,__LINE__,i,
		//		f->fmt.pix.pixelformat);//, cfmt->fourcc);
		//	printk("^^^^ %s[%d] i = %d  0x%08x^^^^\n",__func__,__LINE__,i,
		//		cfmt->fourcc);
		if(f->fmt.pix.pixelformat == cfmt->fourcc){
			break;
		}
	}
	//	printk("^^^^ %s[%d] i = %d^^^^\n",__func__,__LINE__,i);
	if(i > contrl->fmt_end){
		printk("%s[%d], don't support the format(0x%08x)\n",
				__func__, __LINE__, f->fmt.pix.pixelformat);
		return -ISP_ERROR;
	}
	return ISP_SUCCESS;
}

static int isp_core_frame_channel_set_fmt(struct tx_isp_core_device *core, int value)
{
	struct apical_isp_contrl *contrl = &core->contrl;
	frame_chan_vdev_t *vdev = (frame_chan_vdev_t *)value;
	struct frame_channel_attribute *attr = &vdev->attr;
	struct v4l2_format *output = &(attr->output);
	struct v4l2_format *f = vdev->fmt;
	struct frame_channel_format *cfmt = NULL;
	int i = 0;

	if(f->fmt.pix.width != output->fmt.pix.width ||
			f->fmt.pix.height != output->fmt.pix.height){
		printk("%s[%d] f->width = %d f->height = %d, output->width=%d, output->height=%d\n",
				__func__, __LINE__,
				f->fmt.pix.width, f->fmt.pix.height,
				output->fmt.pix.width, output->fmt.pix.height);
		return -EINVAL;
	}

	if(output->fmt.pix.width > attr->max_width ||
			output->fmt.pix.height > attr->max_height){
		printk("%s[%d] attr->max_width = %d attr->max_height = %d, output->width=%d, output->height=%d\n",
				__func__, __LINE__,
				attr->max_width, attr->max_height,
				output->fmt.pix.width, output->fmt.pix.height);
		return -EINVAL;
	}

	for(i = contrl->fmt_start; i <= contrl->fmt_end; i++){
		cfmt = &(isp_output_fmt[i]);
		if(f->fmt.pix.pixelformat == cfmt->fourcc)
			break;
	}

	if (i > contrl->fmt_end) {
		printk("%s[%d] unfound the pixelformat = %d\n", __func__, __LINE__, f->fmt.pix.pixelformat);
		return -EINVAL;
	}

	f->fmt.pix.bytesperline = f->fmt.pix.width * cfmt->depth / 8;
	f->fmt.pix.sizeimage = f->fmt.pix.bytesperline * f->fmt.pix.height;
	output->fmt.pix.pixelformat = f->fmt.pix.pixelformat;
	output->fmt.pix.bytesperline = f->fmt.pix.bytesperline;
	output->fmt.pix.sizeimage = f->fmt.pix.sizeimage;
	vdev->attr.lineoffset = output->fmt.pix.width * (cfmt->depth / 8);
	output->fmt.pix.priv = (unsigned int)cfmt;
//	printk("~~~~~ %s[%d] width = %d, height = %d bytesperline = %d sizeimage = %d ~~~~~\n",__func__,__LINE__,
//			output->fmt.pix.width, output->fmt.pix.height, output->fmt.pix.bytesperline, output->fmt.pix.sizeimage);
	return isp_core_config_dma_channel_write(core, vdev);
}

static int isp_core_frame_channel_crop_capture(struct tx_isp_core_device *core, int value)
{
	struct apical_isp_contrl *contrl = &core->contrl;
	struct v4l2_mbus_framefmt *mbus = &core->vin.mbus;
	frame_chan_vdev_t *vdev = (frame_chan_vdev_t *)value;
	struct frame_channel_attribute *attr = &vdev->attr;
	int width, height;
	int ret = ISP_SUCCESS;

	if(core->bypass == TX_ISP_FRAME_CHANNEL_BYPASS_ISP_ENABLE ||
			mbus->code == V4L2_MBUS_FMT_YUYV8_1X16){
		width = height = 0;
		ret = -EPERM;
	}else{
		width = contrl->inwidth;
		height = contrl->inheight;
	}
	attr->bounds.top = 0;
	attr->bounds.left = 0;
	attr->bounds.width = width;
	attr->bounds.height = height;

	return ret;
}

static int isp_core_frame_channel_set_crop(struct tx_isp_core_device *core, int value)
{
	frame_chan_vdev_t *vdev = (frame_chan_vdev_t *)value;
	struct frame_channel_attribute *attr = &vdev->attr;
	apical_api_control_t api;
	unsigned int chan;
	unsigned char status = 0;
	int ret = ISP_SUCCESS;

	if(core->bypass == TX_ISP_FRAME_CHANNEL_BYPASS_ISP_ENABLE ){
		ret = -EPERM;
	}
	switch(vdev->index){
		case ISP_FR_VIDEO_CHANNEL:
			apical_isp_top_bypass_fr_crop_write(attr->crop_enable?0:1);
			chan = CROP_FR;
			break;
		case ISP_DS1_VIDEO_CHANNEL:
			apical_isp_top_bypass_ds1_crop_write(attr->crop_enable?0:1);
			chan = CROP_DS;
			break;
#if TX_ISP_EXIST_DS2_CHANNEL
		case ISP_DS2_VIDEO_CHANNEL:
			apical_isp_top_bypass_ds2_crop_write(attr->crop_enable?0:1);
			chan = CROP_DS2;
			break;
#endif
		default:
			ret = -EINVAL;
			break;
	}
	if(ret < 0)
		return ret;
	api.type = TIMAGE;
	api.dir = COMMAND_SET;
	if(attr->crop_enable){
	api.value = (chan << 16) + attr->crop.width;
	api.id = IMAGE_RESIZE_WIDTH_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &ret);
	//	printk("[%d]apical command: status = %d, ret = 0x%08x\n",__LINE__, status, ret);

	api.value = (chan << 16) + attr->crop.height;
	api.id = IMAGE_RESIZE_HEIGHT_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &ret);
	//	printk("[%d]apical command: status = %d, ret = 0x%08x\n",__LINE__, status, ret);

	api.value = (chan << 16) + attr->crop.left;
	api.id = IMAGE_CROP_XOFFSET_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &ret);
	//	printk("[%d]apical command: status = %d, ret = 0x%08x\n",__LINE__, status, ret);

	api.value = (chan << 16) + attr->crop.top;
	api.id = IMAGE_CROP_YOFFSET_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &ret);
	//	printk("[%d]apical command: status = %d, ret = 0x%08x\n",__LINE__, status, ret);

	api.value = (chan << 16) + ENABLE;
	api.id = IMAGE_RESIZE_ENABLE_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &ret);
	//	printk("[%d]apical command: status = %d, ret = 0x%08x\n",__LINE__, status, ret);

	attr->output.fmt.pix.width = attr->crop.width;
	attr->output.fmt.pix.height = attr->crop.height;
	}else{
	api.value = (chan << 16) + DISABLE;
	api.id = IMAGE_RESIZE_ENABLE_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &ret);
	}
	return ret;
}

static int isp_core_frame_channel_scaler_capture(struct tx_isp_core_device *core, int value)
{
	struct v4l2_mbus_framefmt *mbus = &core->vin.mbus;
	frame_chan_vdev_t *vdev = (frame_chan_vdev_t *)value;
	struct frame_channel_attribute *attr = &vdev->attr;
	int inwidth, inheight;
	int ret = ISP_SUCCESS;

	if(core->bypass == TX_ISP_FRAME_CHANNEL_BYPASS_ISP_ENABLE ||
			vdev->index == ISP_FR_VIDEO_CHANNEL ||
			mbus->code == V4L2_MBUS_FMT_YUYV8_1X16){
		inwidth = inheight = 0;
		attr->scalercap.min_width = 0;
		attr->scalercap.min_height = 0;
//		printk("&&&&&&&&&&&&&&&&&&&& %s[%d] chan%d bypass = 0x%08x &&&&&&&&&&&&&&&&&&&&&&&\n",
//				__func__, __LINE__, vdev->index, vdev->bypass);
		ret = -EPERM;
	}else{
		inwidth = attr->output.fmt.pix.width;
		inheight = attr->output.fmt.pix.height;
		attr->scalercap.min_width = attr->min_width;
		attr->scalercap.min_height = attr->min_height;
	}
	attr->scalercap.max_width = inwidth > attr->max_width
		? attr->max_width : inwidth;
	attr->scalercap.max_height = inheight > attr->max_height
		? attr->max_height : inheight;
	printk("&&& chan%d  scaler.max_width = %d max_height = %d  min_width = %d min_height = %d &&&\n",
			vdev->index, attr->scalercap.max_width, attr->scalercap.max_height,
			attr->scalercap.min_width, attr->scalercap.min_height);
	return ret;
}

static int isp_core_frame_channel_set_scaler(struct tx_isp_core_device *core, int value)
{
	frame_chan_vdev_t *vdev = (frame_chan_vdev_t *)value;
	struct frame_channel_attribute *attr = &vdev->attr;
	apical_api_control_t api;
	unsigned int chan;
	unsigned char status = 0;
	int ret = ISP_SUCCESS;

	if(core->bypass == TX_ISP_FRAME_CHANNEL_BYPASS_ISP_ENABLE){
		ret = -EPERM;
	}

	switch(vdev->index){
		case ISP_DS1_VIDEO_CHANNEL:
			apical_isp_top_bypass_ds1_scaler_write(attr->scaler_enable?0:1);
			chan = SCALER;
			break;
#if TX_ISP_EXIST_DS2_CHANNEL
		case ISP_DS2_VIDEO_CHANNEL:
			apical_isp_top_bypass_ds2_scaler_write(attr->scaler_enable?0:1);
			chan = SCALER2;
			break;
#endif
		case ISP_FR_VIDEO_CHANNEL:
		default:
			ret = -EINVAL;
			break;
	}
	if(ret < 0)
		return ret;
	api.type = TIMAGE;
	api.dir = COMMAND_SET;
	if(attr->scaler_enable){
	api.value = (chan << 16) + attr->scaler.out_width;
	api.id = IMAGE_RESIZE_WIDTH_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &ret);
	//	printk("[%d]apical command: status = %d, ret = 0x%08x\n",__LINE__, status, ret);

	api.value = (chan << 16) + attr->scaler.out_height;
	api.id = IMAGE_RESIZE_HEIGHT_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &ret);
	//	printk("[%d]apical command: status = %d, ret = 0x%08x\n",__LINE__, status, ret);

	api.value = (chan << 16) + ENABLE;
	api.id = IMAGE_RESIZE_ENABLE_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &ret);
	//	printk("[%d]apical command: status = %d, ret = 0x%08x\n",__LINE__, status, ret);	return ret;

	attr->output.fmt.pix.width = attr->scaler.out_width;
	attr->output.fmt.pix.height = attr->scaler.out_height;
	}else{
	api.value = (chan << 16) + DISABLE;
	api.id = IMAGE_RESIZE_ENABLE_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &ret);
	}
	return ret;
}

static int isp_core_frame_channel_streamon(struct tx_isp_core_device *core, int value)
{
	frame_chan_vdev_t *vdev = (frame_chan_vdev_t *)value;
	struct tx_isp_frame_channel *chan = &(core->chans[vdev->index]);
	struct frame_channel_attribute *attr = &vdev->attr;
	struct v4l2_format *output = &(attr->output);
	unsigned int base = 0x00b00 + 0x100 * vdev->index; // the base of address of dma channel write
	unsigned int primary;
	unsigned long flags;

	memset(chan->bank_flag, 0 ,sizeof(chan->bank_flag));
	memset(chan->vflip_flag, 0 ,sizeof(chan->vflip_flag));
	memset(chan->banks_addr, 0 ,sizeof(chan->banks_addr));
	chan->end = 0;
	chan->start = 0;
	chan->dma_state = 0;
	chan->vflip_state = 0xff;
	chan->state = TX_ISP_STATE_STOP;

	chan->usingbanks = vdev->reqbufs > ISP_DMA_WRITE_MAXBASE_NUM ? ISP_DMA_WRITE_MAXBASE_NUM : vdev->reqbufs;

	switch(output->fmt.pix.pixelformat){
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV21:
			/* dma channel uv write */
			//			APICAL_WRITE_32(base + 0xa4, 0x03);// axi_port_enable set 1, frame_write_cancel set 1.
			primary = APICAL_READ_32(base + 0x80);
			primary |= (1 << 9);
			APICAL_WRITE_32(base + 0x80, primary);
			APICAL_WRITE_32(base + 0x9c, chan->usingbanks > 0 ? chan->usingbanks -1 : 0);//MAX BANK
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_UYVY:
		case V4L2_PIX_FMT_YUV444:
		case V4L2_PIX_FMT_RGB565:
		case V4L2_PIX_FMT_RGB24:
		case V4L2_PIX_FMT_RGB310:
		case V4L2_PIX_FMT_SBGGR12:
		case V4L2_PIX_FMT_SGBRG12:
		case V4L2_PIX_FMT_SGRBG12:
		case V4L2_PIX_FMT_SRGGB12:
			/* dma channel y write */
			//			APICAL_WRITE_32(base + 0x24, 0x03);// axi_port_enable set 1, frame_write_cancel set 1.
			primary = APICAL_READ_32(base);
			primary |= (1 << 9);
			APICAL_WRITE_32(base, primary);
			APICAL_WRITE_32(base + 0x1c, chan->usingbanks > 0 ? chan->usingbanks -1 : 0);//MAX BANK
			break;
		default:
			break;
	}
	/* streamon */
	spin_lock_irqsave(&chan->slock, flags);
	chan->state = TX_ISP_STATE_RUN;
	spin_unlock_irqrestore(&chan->slock, flags);
	apical_isp_input_port_field_mode_write(0); // Temporary measures
	mutex_lock(&core->mlock);
	atomic_add(TX_ISP_STATE_RUN, &core->chan_state);
	mutex_unlock(&core->mlock);

#if 0
	printk("0x%08x = 0x%08x\n",0x40, APICAL_READ_32(0x40));
	printk("0x%08x = 0x%08x\n",0x44, APICAL_READ_32(0x44));
	printk("0x%08x = 0x%08x\n", base + 0x00, APICAL_READ_32(base + 0x00));
	printk("0x%08x = 0x%08x\n", base + 0x04, APICAL_READ_32(base + 0x04));
	printk("0x%08x = 0x%08x\n", base + 0x1c, APICAL_READ_32(base + 0x1c));
	printk("0x%08x = 0x%08x\n", base + 0x20, APICAL_READ_32(base + 0x20));
	printk("0x%08x = 0x%08x\n", base + 0x80 + 0x00, APICAL_READ_32(base + 0x80 + 0x00));
	printk("0x%08x = 0x%08x\n", base + 0x80 + 0x04, APICAL_READ_32(base + 0x80 + 0x04));
	printk("0x%08x = 0x%08x\n", base + 0x80 + 0x1c, APICAL_READ_32(base + 0x80 + 0x1c));
	printk("0x%08x = 0x%08x\n", base + 0x80 + 0x20, APICAL_READ_32(base + 0x80 + 0x20));
	{
		unsigned int reg;
		for(reg = 0x80; reg < 0xa4; reg = reg + 4)
			printk("0x%08x = 0x%08x\n", reg, APICAL_READ_32(reg));
		for(reg = 0x640; reg < 0x66c; reg = reg + 4)
			printk("0x%08x = 0x%08x\n", reg, APICAL_READ_32(reg));
	}
#endif
	return ISP_SUCCESS;
}

static int isp_core_frame_channel_streamoff(struct tx_isp_core_device *core, int value)
{
	frame_chan_vdev_t *vdev = (frame_chan_vdev_t *)value;
	struct apical_isp_contrl *contrl = &core->contrl;
	struct frame_channel_format *cfmt;
	struct tx_isp_frame_channel *chan = &(core->chans[vdev->index]);
	struct frame_channel_attribute *attr = &vdev->attr;
	struct v4l2_format *output = &(attr->output);
	unsigned int base = 0x00b00 + 0x100 * vdev->index; // the base of address of dma channel write
	unsigned int primary;
	unsigned long flags;
	int ret = ISP_SUCCESS;

	spin_lock_irqsave(&chan->slock, flags);
	if(chan->state == TX_ISP_STATE_STOP){
		spin_unlock_irqrestore(&chan->slock, flags);
		return ISP_SUCCESS;
	}
	spin_unlock_irqrestore(&chan->slock, flags);

	switch(output->fmt.pix.pixelformat){
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV21:
			/* dma channel uv write */
			while(APICAL_READ_32(base + 0xa4) & (1<<16));
			primary = APICAL_READ_32(base + 0x80);
			primary &= ~(1 << 9);
			APICAL_WRITE_32(base + 0x80, primary);
			APICAL_WRITE_32(base + 0x9c, 0x08);// axi_port_enable set 0, frame_write_cancel set 1.
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_UYVY:
		case V4L2_PIX_FMT_YUV444:
		case V4L2_PIX_FMT_RGB565:
		case V4L2_PIX_FMT_RGB24:
		case V4L2_PIX_FMT_RGB310:
			/* dma channel y write */
			while(APICAL_READ_32(base + 0x24) & (1<<16));
			primary = APICAL_READ_32(base);
			primary &= ~(1 << 9);
			APICAL_WRITE_32(base, primary);
			APICAL_WRITE_32(base + 0x1c, 0x08);// axi_port_enable set 0, frame_write_cancel set 1.
			break;
		default:
			break;
	}

	/* streamoff */
	spin_lock_irqsave(&chan->slock, flags);
	chan->state = TX_ISP_STATE_STOP;
	cleanup_buffer_fifo(&chan->fifo);
	cleanup_chan_banks(chan);
	spin_unlock_irqrestore(&chan->slock, flags);

	atomic_sub(TX_ISP_STATE_RUN, &core->chan_state);

	/* reset output parameters */
	cfmt = &(isp_output_fmt[contrl->fmt_end]);
	output->fmt.pix.width = contrl->inwidth;
	output->fmt.pix.height = contrl->inheight;
	output->fmt.pix.pixelformat = cfmt->fourcc;
	output->fmt.pix.bytesperline = output->fmt.pix.width * cfmt->depth / 8;
	output->fmt.pix.sizeimage = output->fmt.pix.bytesperline * output->fmt.pix.height;

	return ret;
}

static int isp_core_frame_channel_queue_buffer(struct tx_isp_core_device *core, int value)
{
	struct frame_channel_video_buffer *vbuf = (struct frame_channel_video_buffer *)value;
	frame_chan_vdev_t *vdev = (frame_chan_vdev_t *)vbuf->buf.priv;
	struct tx_isp_frame_channel *chan = &(core->chans[vdev->index]);
	unsigned long flags;
	spin_lock_irqsave(&chan->slock, flags);
	push_buffer_fifo(&chan->fifo, &vbuf->buf);
	spin_unlock_irqrestore(&chan->slock, flags);
	return ISP_SUCCESS;
}
static long isp_core_ops_private_ioctl(struct tx_isp_core_device *core, struct isp_private_ioctl *ctl)
{
	long ret = ISP_SUCCESS;
	switch(ctl->cmd){
		case TX_ISP_PRIVATE_IOCTL_MODULE_CLK:
			ret = isp_core_set_clk(core, ctl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
			if(atomic_read(&core->chan_state) > TX_ISP_STATE_START)
				ret = -ISP_ERROR;
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
			if(atomic_read(&core->chan_state) > TX_ISP_STATE_START)
				ret = -ISP_ERROR;
			break;
		case TX_ISP_PRIVATE_IOCTL_SYNC_VIDEO_IN:
			if(ctl->value){
				memcpy(&core->vin, (void *)(ctl->value), sizeof(struct tx_isp_video_in));
				stab.global_max_integration_time = core->vin.attr->max_integration_time;
			}else
				memset(&core->vin, 0, sizeof(struct tx_isp_video_in));
			break;
		case TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_ENUM_FMT:
			ret = isp_core_frame_channel_enum_fmt(core, ctl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_TRY_FMT:
			ret = isp_core_frame_channel_try_fmt(core, ctl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SET_FMT:
			ret = isp_core_frame_channel_set_fmt(core, ctl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_CROP_CAP:
			ret = isp_core_frame_channel_crop_capture(core, ctl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SET_CROP:
			ret = isp_core_frame_channel_set_crop(core, ctl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SCALER_CAP:
			ret = isp_core_frame_channel_scaler_capture(core, ctl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_SET_SCALER:
			ret = isp_core_frame_channel_set_scaler(core, ctl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_STREAM_ON:
			ret = isp_core_frame_channel_streamon(core, ctl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_STREAM_OFF:
			ret = isp_core_frame_channel_streamoff(core, ctl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_FRAME_CHAN_QUEUE_BUFFER:
			ret = isp_core_frame_channel_queue_buffer(core, ctl->value);
			break;
		default:
			break;
	}
	return ret;
}

static long isp_core_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_core_device *core = sd_to_tx_isp_core_device(sd);
	long ret = ISP_SUCCESS;

	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = isp_core_ops_private_ioctl(core, arg);
			break;
		default:
			break;
	}
	return ret;
}
static const struct v4l2_subdev_core_ops isp_core_subdev_core_ops ={
	.init = isp_core_ops_init,
	.reset = isp_core_ops_reset,
	.s_power = isp_core_ops_s_power,
	.g_ctrl = isp_core_ops_g_ctrl,
	.s_ctrl = isp_core_ops_s_ctrl,
	.ioctl = isp_core_ops_ioctl,
	.interrupt_service_routine = isp_core_interrupt_service_routine,
};

static void isp_core_config_frame_channel_attr(struct tx_isp_core_device *core)
{
	//	struct v4l2_subdev *sd = &core->sd;
	struct apical_isp_contrl *contrl = &core->contrl;
	struct tx_isp_frame_channel *chan = core->chans;
	struct frame_channel_attribute *attr;
	struct frame_channel_format *cfmt;
	int index = 0;

	cfmt = &(isp_output_fmt[contrl->fmt_end]);
	for(index = 0; index < ISP_MAX_OUTPUT_VIDEOS; index++){
		attr = &(chan[index].video.attr);
		attr->output.fmt.pix.width = contrl->inwidth;
		attr->output.fmt.pix.height = contrl->inheight;
		attr->output.fmt.pix.pixelformat = cfmt->fourcc;
		attr->output.fmt.pix.bytesperline = attr->output.fmt.pix.width * cfmt->depth / 8;
		attr->output.fmt.pix.sizeimage = attr->output.fmt.pix.bytesperline * attr->output.fmt.pix.height;
		//	printk("## %s[%d] width = %d height = %d pixformat = 0x%08x ##\n",
		//		__func__,__LINE__,contrl->inwidth,contrl->inheight,cfmt->fourcc);
	}
}


static int isp_config_input_port(struct tx_isp_core_device *core)
{
	struct v4l2_device *v4l2_dev = core->sd.v4l2_dev;
	struct v4l2_mbus_framefmt *mbus = &core->vin.mbus;
	struct apical_isp_contrl *contrl = &core->contrl;
	unsigned char color = APICAL_ISP_TOP_RGGB_START_DEFAULT;
	int ret = ISP_SUCCESS;
	int index;

	if(mbus->width > TX_ISP_INPUT_PORT_MAX_WIDTH || mbus->height > TX_ISP_INPUT_PORT_MAX_HEIGHT){
		printk("Sensor outputs bigger resolution than that ISP device can't deal with!\n");
		return -1;
	}
	switch(mbus->code){
		case V4L2_MBUS_FMT_SBGGR8_1X8:
		case V4L2_MBUS_FMT_SBGGR10_DPCM8_1X8:
		case V4L2_MBUS_FMT_SBGGR10_2X8_PADHI_BE:
		case V4L2_MBUS_FMT_SBGGR10_2X8_PADHI_LE:
		case V4L2_MBUS_FMT_SBGGR10_2X8_PADLO_BE:
		case V4L2_MBUS_FMT_SBGGR10_2X8_PADLO_LE:
		case V4L2_MBUS_FMT_SBGGR10_1X10:
		case V4L2_MBUS_FMT_SBGGR12_1X12:
			color = APICAL_ISP_TOP_RGGB_START_B_GB_GR_R;
			memcpy(isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].name, "Bayer formats (sBGGR)", 32);
			isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].fourcc = V4L2_PIX_FMT_SBGGR12;
			isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].depth = 16;
			isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].priv = DMA_FORMAT_RAW16;
			contrl->fmt_start = APICAL_ISP_INPUT_RAW_FMT_INDEX_START;
			contrl->fmt_end = APICAL_ISP_INPUT_RAW_FMT_INDEX_END;
			break;
		case V4L2_MBUS_FMT_SGBRG8_1X8:
		case V4L2_MBUS_FMT_SGBRG10_DPCM8_1X8:
		case V4L2_MBUS_FMT_SGBRG10_1X10:
		case V4L2_MBUS_FMT_SGBRG12_1X12:
			color = APICAL_ISP_TOP_RGGB_START_GB_B_R_GR;
			memcpy(isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].name, "Bayer formats (sGBRG)", 32);
			isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].fourcc = V4L2_PIX_FMT_SGBRG12;
			isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].depth = 16;
			isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].priv = DMA_FORMAT_RAW16;
			contrl->fmt_start = APICAL_ISP_INPUT_RAW_FMT_INDEX_START;
			contrl->fmt_end = APICAL_ISP_INPUT_RAW_FMT_INDEX_END;
			break;
		case V4L2_MBUS_FMT_SGRBG8_1X8:
		case V4L2_MBUS_FMT_SGRBG10_DPCM8_1X8:
		case V4L2_MBUS_FMT_SGRBG10_1X10:
		case V4L2_MBUS_FMT_SGRBG12_1X12:
			color = APICAL_ISP_TOP_RGGB_START_GR_R_B_GB;
			memcpy(isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].name, "Bayer formats (sGRBG)", 32);
			isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].fourcc = V4L2_PIX_FMT_SGRBG12;
			isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].depth = 16;
			isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].priv = DMA_FORMAT_RAW16;
			contrl->fmt_start = APICAL_ISP_INPUT_RAW_FMT_INDEX_START;
			contrl->fmt_end = APICAL_ISP_INPUT_RAW_FMT_INDEX_END;
			break;
		case V4L2_MBUS_FMT_SRGGB8_1X8:
		case V4L2_MBUS_FMT_SRGGB10_DPCM8_1X8:
		case V4L2_MBUS_FMT_SRGGB10_1X10:
		case V4L2_MBUS_FMT_SRGGB12_1X12:
			color = APICAL_ISP_TOP_RGGB_START_R_GR_GB_B;
			memcpy(isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].name, "Bayer formats (sRGGB)", 32);
			isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].fourcc = V4L2_PIX_FMT_SRGGB12;
			isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].depth = 16;
			isp_output_fmt[APICAL_ISP_RAW_FMT_INDEX].priv = DMA_FORMAT_RAW16;
			contrl->fmt_start = APICAL_ISP_INPUT_RAW_FMT_INDEX_START;
			contrl->fmt_end = APICAL_ISP_INPUT_RAW_FMT_INDEX_END;
			break;
		case V4L2_MBUS_FMT_RGB565_2X8_LE:
			contrl->fmt_start = APICAL_ISP_INPUT_RGB565_FMT_INDEX_START;
			contrl->fmt_end = APICAL_ISP_INPUT_RGB565_FMT_INDEX_END;
			apical_isp_top_isp_raw_bypass_write(1);
			break;
		case V4L2_MBUS_FMT_RGB888_3X8_LE:
			contrl->fmt_start = APICAL_ISP_INPUT_RGB888_FMT_INDEX_START;
			contrl->fmt_end = APICAL_ISP_INPUT_RGB888_FMT_INDEX_START;
			apical_isp_top_isp_raw_bypass_write(1);
			break;
		case V4L2_MBUS_FMT_YUYV8_1X16:
			contrl->fmt_start = APICAL_ISP_INPUT_YUV_FMT_INDEX_START;
			contrl->fmt_end = APICAL_ISP_INPUT_YUV_FMT_INDEX_END;
			apical_isp_top_isp_processing_bypass_mode_write(3);
			apical_isp_top_isp_raw_bypass_write(1);
			break;
		default:
			contrl->fmt_start = contrl->fmt_end = APICAL_ISP_INPUT_RAW_FMT_INDEX_END;
			v4l2_err(v4l2_dev, "%s[%d] the format(0x%08x) of input couldn't be handled!\n",
					__func__,__LINE__, mbus->code);
			ret = -ISP_ERROR;
			break;
	}
	if(ret == ISP_SUCCESS){
		contrl->inwidth = mbus->width;
		contrl->inheight = mbus->height;
		contrl->pattern = color;
		apical_isp_top_active_width_write(contrl->inwidth);
		apical_isp_top_active_height_write(contrl->inheight);
		apical_isp_top_rggb_start_write(color); //Starting color of the rggb pattern
		isp_core_config_frame_channel_attr(core);
		/* downscaler paramerters */
		for(index = 0; index < (sizeof(apical_downscaler_lut) / sizeof(apical_downscaler_lut[0])); index++)
			APICAL_WRITE_32(apical_downscaler_lut[index].reg, apical_downscaler_lut[index].value);

	}
	return ret;
}

static int inline isp_core_video_streamon(struct tx_isp_core_device *core)
{
	struct v4l2_device *v4l2_dev = core->sd.v4l2_dev;
	apical_api_control_t api;
	struct tx_isp_notify_argument arg;
	unsigned int status, reason;
	int ret = ISP_SUCCESS;

	api.type = TSYSTEM;
	api.id = ISP_SYSTEM_STATE;
	api.dir = COMMAND_SET;
	api.value = PAUSE;
	status = apical_command(api.type, api.id, api.value, api.dir, &ret);

	/* 2-->module config updates during local vertical blanking */
	apical_isp_top_config_buffer_mode_write(2);
	/* set input port mode, mode1 */
	APICAL_WRITE_32(0x100, 0x00100001);

	/* enable default isp modules and disable bypass isp process */
	core->top.reg.low = APICAL_READ_32(0x40);
	core->top.reg.high = APICAL_READ_32(0x44);
	core->bypass = TX_ISP_FRAME_CHANNEL_BYPASS_ISP_DISABLE;

	/* config isp input port */
	isp_config_input_port(core);

	/*
	 * clear interrupts state of isp-core.
	 * Interrupt event clear register writing 0-1 transition will clear the corresponding status bits.
	 */
	apical_isp_interrupts_interrupt_clear_write(0);
	apical_isp_interrupts_interrupt_clear_write(0xffff);
	/* unmask isp's top interrupts */
	arg.value = TX_ISP_TOP_IRQ_ISP;
	//	v4l2_dev->notify(&core->sd, TX_ISP_NOTIFY_UNMASK_IRQ, &arg);
	//	v4l2_dev->notify(&core->sd, TX_ISP_NOTIFY_ENABLE_IRQ, &arg);
	//	v4l2_dev->notify(&core->sd, TX_ISP_NOTIFY_DISABLE_IRQ, &arg);
#if 1
	api.type = TSYSTEM;
	api.id = ISP_SYSTEM_STATE;
	api.dir = COMMAND_SET;
	api.value = RUN;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if(status == ISP_SUCCESS){
		atomic_set(&core->state, TX_ISP_STATE_RUN);
	}else{
		v4l2_err(v4l2_dev, "%s[%d] state = %d, reason = %d!\n",
				__func__,__LINE__, status, ret);
		ret = -ISP_ERROR;
	}
#endif
	return ret;
}


static int isp_core_video_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct tx_isp_core_device *core = sd_to_tx_isp_core_device(sd);
	struct v4l2_device *v4l2_dev = core->sd.v4l2_dev;
	apical_api_control_t api;
	unsigned int status = ISP_SUCCESS, reason;
	struct tx_isp_frame_channel *chans;
	frame_chan_vdev_t *video;
	int ret = ISP_SUCCESS;
	int index = 0;
	if(atomic_read(&core->state) == TX_ISP_STATE_STOP){
		v4l2_err(v4l2_dev, "%s[%d] the device hasn't been inited!\n",__func__,__LINE__);
		return -ISP_ERROR;
	}
	/*printk("~~~~~~ %s[%d] enable = %d ~~~~~~\n",__func__, __LINE__, enable);*/
	core->frame_state = 0;
	core->vflip_state = 0;
	core->hflip_state = 0;
	if(enable){
		/* streamon */
		if(atomic_read(&core->state) == TX_ISP_STATE_START){
			ret = isp_core_video_streamon(core);
		}
	}else{
		/* streamoff */
		if(atomic_read(&core->state) == TX_ISP_STATE_RUN){
			if(atomic_read(&core->chan_state) >= TX_ISP_STATE_RUN){
				/* At least one channel is in streamon state. */
				/* Firstly we must be streamoff them */
				chans = core->chans;
				for(index = 0; index < ISP_MAX_OUTPUT_VIDEOS; index++){
					if(chans[index].state == TX_ISP_STATE_RUN){
						video = &(chans[index].video);
						isp_core_frame_channel_streamoff(core, (int)video);
					}
				}
			}
			if(atomic_read(&core->chan_state) < TX_ISP_STATE_RUN){
				/* all frame channels are streamoff state. */
				api.type = TSYSTEM;
				api.id = ISP_SYSTEM_STATE;
				api.dir = COMMAND_SET;
				api.value = PAUSE;
				status = apical_command(api.type, api.id, api.value, api.dir, &reason);
				if(status == ISP_SUCCESS){
					atomic_set(&core->state, TX_ISP_STATE_START);
				}else{
					v4l2_err(v4l2_dev, "%s[%d] status = %d, reason = %d!\n",
							__func__,__LINE__, status, ret);
					ret = -ISP_ERROR;
				}
			}else{
				/* At least one channel is in streamon state. */
				v4l2_err(v4l2_dev, "%s[%d] status = %d, reason = %d!\n",
						__func__,__LINE__, status, ret);
				ret = -ISP_ERROR;
			}
		}
	}
	return ret;
}

static const struct v4l2_subdev_video_ops isp_core_subdev_video_ops = {
	.s_stream = isp_core_video_s_stream,
};

static const struct v4l2_subdev_ops isp_core_ops ={
	.core = &isp_core_subdev_core_ops,
	.video = &isp_core_subdev_video_ops,
};

/*
 * ----------------- init device ------------------------
 */

static int isp_core_frame_channel_init(struct tx_isp_core_device *core)
{
	struct v4l2_subdev *sd = &core->sd;
	struct v4l2_device *v4l2_dev = sd->v4l2_dev;
	struct tx_isp_frame_channel *chans;
	struct frame_channel_attribute *attr;
	frame_chan_vdev_t *video;
	int ret = ISP_SUCCESS;

	chans = (struct tx_isp_frame_channel *)kzalloc(sizeof(*chans) * ISP_MAX_OUTPUT_VIDEOS, GFP_KERNEL);
	if(!chans){
		v4l2_err(v4l2_dev, "Failed to allocate sensor device\n");
		ret = -ENOMEM;
		goto exit;
	}
#if TX_ISP_EXIST_FR_CHANNEL
	/* init FR channel */
	video = &(chans[ISP_FR_VIDEO_CHANNEL].video);
	video->parent = sd;
	video->vbm = core->vbm;
	video->index = ISP_FR_VIDEO_CHANNEL;
	ret = tx_isp_frame_channel_device_register(video);
	if(ret){
		v4l2_err(v4l2_dev, "Failed to register fr channel device\n");
		goto err_fr_channel;
	}
	attr = &(video->attr);
	attr->max_width = TX_ISP_FR_CAHNNEL_MAX_WIDTH;
	attr->max_height = TX_ISP_FR_CAHNNEL_MAX_HEIGHT;
	attr->min_width = 128;
	attr->min_height = 128;
	attr->step_width = 8;
	attr->step_height = 8;
	init_buffer_fifo(&(chans[ISP_FR_VIDEO_CHANNEL].fifo));
	spin_lock_init(&(chans[ISP_FR_VIDEO_CHANNEL].slock));
#endif
	/* init DS1 channel */
	video = &(chans[ISP_DS1_VIDEO_CHANNEL].video);
	video->parent = sd;
	video->vbm = core->vbm;
	video->index = ISP_DS1_VIDEO_CHANNEL;
	ret = tx_isp_frame_channel_device_register(video);
	if(ret){
		v4l2_err(v4l2_dev, "Failed to register ds1 channel device\n");
		goto err_ds1_channel;
	}
	attr = &(video->attr);
	attr->max_width = TX_ISP_DS1_CAHNNEL_MAX_WIDTH;
	attr->max_height = TX_ISP_DS1_CAHNNEL_MAX_HEIGHT;
	attr->min_width = 128;
	attr->min_height = 128;
	attr->step_width = 8;
	attr->step_height = 8;
	init_buffer_fifo(&(chans[ISP_DS1_VIDEO_CHANNEL].fifo));
	spin_lock_init(&(chans[ISP_DS1_VIDEO_CHANNEL].slock));

#if TX_ISP_EXIST_DS2_CHANNEL
	/* init DS2 channel */
	video = &(chans[ISP_DS2_VIDEO_CHANNEL].video);
	video->parent = sd;
	video->vbm = core->vbm;
	video->index = ISP_DS2_VIDEO_CHANNEL;
	ret = tx_isp_frame_channel_device_register(video);
	if(ret){
		v4l2_err(v4l2_dev, "Failed to register ds2 channel device\n");
		goto err_ds2_channel;
	}
	attr = &(video->attr);
	attr->max_width = TX_ISP_DS2_CAHNNEL_MAX_WIDTH;
	attr->max_height = TX_ISP_DS2_CAHNNEL_MAX_HEIGHT;
	attr->min_width = 128;
	attr->min_height = 128;
	attr->step_width = 8;
	attr->step_height = 8;
	init_buffer_fifo(&(chans[ISP_DS2_VIDEO_CHANNEL].fifo));
	spin_lock_init(&(chans[ISP_DS2_VIDEO_CHANNEL].slock));
#endif
	atomic_set(&core->chan_state, TX_ISP_STATE_START);
	core->chans = chans;
	return ISP_SUCCESS;

#if TX_ISP_EXIST_DS2_CHANNEL
err_ds2_channel:
	video = &(chans[ISP_DS1_VIDEO_CHANNEL].video);
	tx_isp_frame_channel_device_unregister(video);
#endif
err_ds1_channel:
	video = &(chans[ISP_FR_VIDEO_CHANNEL].video);
	tx_isp_frame_channel_device_unregister(video);
#if TX_ISP_EXIST_FR_CHANNEL
err_fr_channel:
#endif
exit:
	kfree(chans);
	return ret;
}

static int isp_core_frame_channel_deinit(struct tx_isp_core_device *core)
{
	struct tx_isp_frame_channel *chans = core->chans;
	frame_chan_vdev_t *video;
	int index = 0;

	for(index = 0; index < ISP_MAX_OUTPUT_VIDEOS; index++){
		video = &(chans[index].video);
		tx_isp_frame_channel_device_unregister(video);
	}
	kfree(chans);
	atomic_set(&core->chan_state, TX_ISP_STATE_STOP);
	return ISP_SUCCESS;
}

/* debug system node */
static int isp_info_show(struct seq_file *m, void *v)
{
	int len=0;
	struct tx_isp_core_device *core = m->private;
	struct tx_isp_video_in *vin = &core->vin;
	struct v4l2_mbus_framefmt *mbus = &core->vin.mbus;
	struct apical_isp_contrl *contrl = &core->contrl;
	apical_api_control_t api;
	unsigned char status = 0;
	int reason = 0;
	char *colorspace = NULL;
	char *ae_strategy = NULL;
	unsigned int flicker = 0;
	unsigned int temperature = 0;
	unsigned int sensor_again = 0;
	unsigned int max_sensor_again = 0;
	unsigned int sensor_dgain = 0;
	unsigned int max_sensor_dgain = 0;
	unsigned int isp_dgain = 0;
	unsigned int max_isp_dgain = 0;
	unsigned int gain_log2_id = 0;
	unsigned int total_gain = 0;
	unsigned int exposure_log2_id = 0;
	unsigned int saturation_target = 0;
	unsigned int saturation = 0;
	unsigned int contrast = 0;
	unsigned int sharpness = 0;
	unsigned int brightness = 0;
	int ret = 0;
	uint8_t evtolux = 0;


	len += seq_printf(m ,"****************** ISP INFO **********************\n");
	if(atomic_read(&core->state) < TX_ISP_STATE_RUN){
		len += seq_printf(m ,"sensor doesn't work, please enable sensor\n");
		return len;
	}
	switch(contrl->pattern){
		case APICAL_ISP_TOP_RGGB_START_R_GR_GB_B:
			colorspace = "RGGB";
			break;
		case APICAL_ISP_TOP_RGGB_START_GR_R_B_GB:
			colorspace = "GRBG";
			break;
		case APICAL_ISP_TOP_RGGB_START_GB_B_R_GR:
			colorspace = "GBRB";
			break;
		case APICAL_ISP_TOP_RGGB_START_B_GB_GR_R:
			colorspace = "BGGR";
			break;
		default:
			colorspace = "The format of isp input is RGB or YUV422";
			break;
	}
	api.type = TALGORITHMS;
	api.dir = COMMAND_GET;
	api.id = ANTIFLICKER_MODE_ID;
	api.value = 0;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	flicker = reason;

	api.id = AWB_TEMPERATURE_ID;
	api.value = 0;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	temperature = reason * 100;

	api.id = AE_SPLIT_PRESET_ID;
	api.value = 0;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	switch(reason){
	case AE_SPLIT_BALANCED:
		ae_strategy = "AE_SPLIT_BALANCED";
		break;
	case AE_SPLIT_INTEGRATION_PRIORITY:
		ae_strategy = "AE_SPLIT_INTEGRATION_PRIORITY";
		break;
	default:
		ae_strategy = "AE_STRATEGY_BUTT";
		break;
	}

	api.type = TSYSTEM;
	api.dir = COMMAND_GET;
	api.value = 0;
	api.id = SYSTEM_SENSOR_ANALOG_GAIN;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	sensor_again = reason;

	api.id = SYSTEM_MAX_SENSOR_ANALOG_GAIN;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	max_sensor_again = reason;

	api.id = SYSTEM_SENSOR_DIGITAL_GAIN;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	sensor_dgain = reason;

	api.id = SYSTEM_MAX_SENSOR_DIGITAL_GAIN;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	max_sensor_dgain = reason;

	api.id = SYSTEM_ISP_DIGITAL_GAIN;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	isp_dgain = reason;

	api.id = SYSTEM_MAX_ISP_DIGITAL_GAIN;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	max_isp_dgain = reason;

	api.id = SYSTEM_SATURATION_TARGET;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	saturation_target = reason;

	api.type = CALIBRATION;
	api.id = GAIN_LOG2_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	gain_log2_id = reason;

	api.id = EXPOSURE_LOG2_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	exposure_log2_id = reason;

	api.type = TSCENE_MODES;
	api.dir = COMMAND_GET;
	api.id = SHARPENING_STRENGTH_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	sharpness = reason;

	api.id = BRIGHTNESS_STRENGTH_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	brightness = reason;

	api.id = SATURATION_STRENGTH_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	saturation = reason;

	api.id = CONTRAST_STRENGTH_ID;
	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	contrast = reason;

	apical_api_calibration(CALIBRATION_EVTOLUX_PROBABILITY_ENABLE,COMMAND_GET,(void *) (&evtolux), 1, &ret);
	total_gain =  stab.global_sensor_analog_gain  + stab.global_sensor_digital_gain + stab.global_isp_digital_gain;
	total_gain = math_exp2(total_gain, 5, 8);

	len += seq_printf(m ,"Software Version : %s\n", SOFT_VERSION);
	len += seq_printf(m ,"Firmware Version : %s\n", FIRMWARE_VERSION);
	len += seq_printf(m ,"SENSOR NAME : %s\n", vin->attr->name);
	len += seq_printf(m ,"SENSOR OUTPUT WIDTH : %d\n", mbus->width);
	len += seq_printf(m ,"SENSOR OUTPUT HEIGHT : %d\n", mbus->height);
	len += seq_printf(m ,"SENSOR OUTPUT RAW PATTERN : %s\n", colorspace);
	len += seq_printf(m ,"SENSOR Integration Time : %d lines\n", stab.global_integration_time);
	len += seq_printf(m ,"ISP Top Value : 0x%x\n", APICAL_READ_32(0x40));
	len += seq_printf(m ,"ISP Runing Mode : %s\n", ((apical_isp_fr_cs_conv_clip_min_uv_read() == 512) ? "Night" : "Day"));
	len += seq_printf(m ,"ISP OUTPUT FPS : %d / %d\n", vin->fps >> 16, vin->fps & 0xffff);
	len += seq_printf(m ,"SENSOR analog gain : %d\n", sensor_again);
	len += seq_printf(m ,"MAX SENSOR analog gain : %d\n", max_sensor_again);
	len += seq_printf(m ,"SENSOR digital gain : %d\n", sensor_dgain);
	len += seq_printf(m ,"MAX SENSOR digital gain : %d\n", max_sensor_dgain);
	len += seq_printf(m ,"ISP digital gain : %d\n", isp_dgain);
	len += seq_printf(m ,"MAX ISP digital gain : %d\n", max_isp_dgain);
	len += seq_printf(m ,"ISP gain log2 id : %d\n", gain_log2_id);
	len += seq_printf(m ,"ISP total gain : %d\n", total_gain);
	len += seq_printf(m ,"ISP exposure log2 id: %d\n", exposure_log2_id);
	len += seq_printf(m ,"ISP AE strategy: %s\n", ae_strategy);
	len += seq_printf(m ,"ISP Antiflicker : %d[0 means disable]\n", flicker);
	len += seq_printf(m ,"Evtolux Probability Enable  : %d\n", evtolux);
	len += seq_printf(m ,"ISP WB Gain00 : %d\n", apical_isp_white_balance_gain_00_read());
	len += seq_printf(m ,"ISP WB Gain01 : %d\n", apical_isp_white_balance_gain_01_read());
	len += seq_printf(m ,"ISP WB Gain10 : %d\n", apical_isp_white_balance_gain_10_read());
	len += seq_printf(m ,"ISP WB Gain11 : %d\n", apical_isp_white_balance_gain_11_read());
	len += seq_printf(m ,"ISP WB rg : %d\n", apical_isp_metering_awb_rg_read());
	len += seq_printf(m ,"ISP WB bg : %d\n", apical_isp_metering_awb_bg_read());
	len += seq_printf(m ,"ISP WB Temperature : %d\n", temperature);
	len += seq_printf(m ,"ISP Black level 00 : %d\n", apical_isp_offset_black_00_read());
	len += seq_printf(m ,"ISP Black level 01 : %d\n", apical_isp_offset_black_01_read());
	len += seq_printf(m ,"ISP Black level 10 : %d\n", apical_isp_offset_black_10_read());
	len += seq_printf(m ,"ISP Black level 11 : %d\n", apical_isp_offset_black_11_read());
	len += seq_printf(m ,"ISP LSC mesh module : %s\n", apical_isp_mesh_shading_enable_read()?"enable":"disable");
	len += seq_printf(m ,"ISP LSC mesh blend mode : %d\n", apical_isp_mesh_shading_mesh_alpha_mode_read());
	len += seq_printf(m ,"ISP LSC mesh R table : %d\n", apical_isp_mesh_shading_mesh_alpha_bank_r_read());
	len += seq_printf(m ,"ISP LSC mesh G table : %d\n", apical_isp_mesh_shading_mesh_alpha_bank_g_read());
	len += seq_printf(m ,"ISP LSC mesh B table : %d\n", apical_isp_mesh_shading_mesh_alpha_bank_b_read());
	len += seq_printf(m ,"ISP LSC mesh R blend : %d\n", apical_isp_mesh_shading_mesh_alpha_r_read());
	len += seq_printf(m ,"ISP LSC mesh G blend : %d\n", apical_isp_mesh_shading_mesh_alpha_g_read());
	len += seq_printf(m ,"ISP LSC mesh B blend : %d\n", apical_isp_mesh_shading_mesh_alpha_b_read());
	len += seq_printf(m ,"ISP LSC mesh shading strength : %d\n", APICAL_READ_32(0x39c));
	len += seq_printf(m ,"ISP Sinter thresh1h : %d\n", apical_isp_sinter_thresh_1h_read());
	len += seq_printf(m ,"ISP Sinter thresh4h : %d\n", apical_isp_sinter_thresh_4h_read());
	len += seq_printf(m ,"ISP Sinter thresh1v : %d\n", apical_isp_sinter_thresh_1v_read());
	len += seq_printf(m ,"ISP Sinter thresh4v : %d\n", apical_isp_sinter_thresh_4v_read());
	len += seq_printf(m ,"ISP Sinter thresh short : %d\n", apical_isp_sinter_thresh_short_read());
	len += seq_printf(m ,"ISP Sinter thresh long : %d\n", apical_isp_sinter_thresh_long_read());
	len += seq_printf(m ,"ISP Sinter strength1 : %d\n", apical_isp_sinter_strength_1_read());
	len += seq_printf(m ,"ISP Sinter strength4 : %d\n", apical_isp_sinter_strength_4_read());
	len += seq_printf(m ,"ISP Temper thresh module :%s\n", apical_isp_temper_enable_read()?"enable":"disable");
	len += seq_printf(m ,"ISP Temper thresh short : %d\n", apical_isp_temper_thresh_short_read());
	len += seq_printf(m ,"ISP Temper thresh long : %d\n", apical_isp_temper_thresh_long_read());
	len += seq_printf(m ,"ISP Iridix strength : %d\n", apical_isp_iridix_strength_read());
	len += seq_printf(m ,"ISP Defect pixel threshold : %d\n", apical_isp_raw_frontend_dp_threshold_read());
	len += seq_printf(m ,"ISP Defect pixel slope : %d\n", apical_isp_raw_frontend_dp_slope_read());
	len += seq_printf(m ,"ISP sharpening directional : %d\n", apical_isp_demosaic_sharp_alt_d_read());
	len += seq_printf(m ,"ISP sharpening undirectional : %d\n", apical_isp_demosaic_sharp_alt_ud_read());
	len += seq_printf(m ,"ISP FR sharpen strength : %d\n", apical_isp_fr_sharpen_strength_read());
	len += seq_printf(m ,"ISP DS1 sharpen strength : %d\n", apical_isp_ds1_sharpen_strength_read());
#if TX_ISP_EXIST_DS2_CHANNEL
	len += seq_printf(m ,"ISP DS2 sharpen strength : %d\n", apical_isp_ds2_sharpen_strength_read());
#endif
	len += seq_printf(m ,"ISP CCM R_R : %d\n", apical_isp_matrix_rgb_coefft_r_r_read());
	len += seq_printf(m ,"ISP CCM R_G : %d\n", apical_isp_matrix_rgb_coefft_r_g_read());
	len += seq_printf(m ,"ISP CCM R_B : %d\n", apical_isp_matrix_rgb_coefft_r_b_read());
	len += seq_printf(m ,"ISP CCM G_R : %d\n", apical_isp_matrix_rgb_coefft_g_r_read());
	len += seq_printf(m ,"ISP CCM G_B : %d\n", apical_isp_matrix_rgb_coefft_g_b_read());
	len += seq_printf(m ,"ISP CCM G_G : %d\n", apical_isp_matrix_rgb_coefft_g_g_read());
	len += seq_printf(m ,"ISP CCM B_R : %d\n", apical_isp_matrix_rgb_coefft_b_r_read());
	len += seq_printf(m ,"ISP CCM B_G : %d\n", apical_isp_matrix_rgb_coefft_b_g_read());
	len += seq_printf(m ,"ISP CCM B_B : %d\n", apical_isp_matrix_rgb_coefft_b_b_read());
	len += seq_printf(m ,"Saturation Target : %d\n", saturation_target);
	len += seq_printf(m ,"Saturation : %d\n", saturation);
	len += seq_printf(m ,"Sharpness : %d\n", sharpness);
	len += seq_printf(m ,"Contrast : %d\n", contrast);
	len += seq_printf(m ,"Brightness : %d\n", brightness);

	return len;
}

static int dump_isp_info_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, isp_info_show, PDE_DATA(inode),8192);
}

static const struct file_operations isp_info_proc_fops ={
	.read = seq_read,
	.open = dump_isp_info_open,
	.llseek = seq_lseek,
	.release = single_release,
};
/* gamma info */
static int isp_gamma_show(struct seq_file *m, void *v)
{
	int len = 0;
	int i = 0;
	int ret = 0;
	uint16_t gamma[129] = {0};

	struct tx_isp_core_device *core = m->private;

	if(atomic_read(&core->state) < TX_ISP_STATE_RUN){
		len += seq_printf(m ,"sensor doesn't work, please enable sensor\n");
		return len;
	}

	apical_api_calibration(CALIBRATION_GAMMA_LINEAR, COMMAND_GET, gamma, 1*129*2, &ret);
	printk("ret = %d\n", ret);
	for (i = 0; i < 129; i++) {
		len += seq_printf(m ," %d", gamma[i]);
	}
	len += seq_printf(m ,"\n");
#if 0
	for (i = 0; i < 129; i++) {
		len += seq_printf(m ," %d", APICAL_READ_32(0x10400+4*i));
	}
	len += seq_printf(m ,"\n");
#endif
	return len;
}

static int dump_isp_gamma_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, isp_gamma_show, PDE_DATA(inode),8192);
}

static ssize_t isp_gamma_put(struct file *file, const char __user *buffer, size_t count, loff_t *f_pos)
{
	int i = 0;
	int ret = 0;
	uint16_t gamma[129] = {0};
	char *ps;
	char *pe;
	char *bufe;

	char *buf = kzalloc((count+1), GFP_KERNEL);
	if(!buf)
		return -ENOMEM;
	if(copy_from_user(buf, buffer, count))
	{
		kfree(buf);
		return EFAULT;
	}
	printk("%s\n", buf);

	ps = buf;
	pe = buf;
	bufe = buf + count;
	for (i = 0; i < (129+1); i++) {
		while((*ps)<'0'||(*ps)>'9')	{
			ps++;
			if ((uint32_t)ps >= (uint32_t)bufe) {
				if (129 != i) {
					goto err_gamma_parse;
				} else {
					goto end_gamma_parse;
				}

			}
		}
		if (i >= 129) {
			goto err_gamma_parse;
		}
		gamma[i] = simple_strtoull(ps, &pe, 0);
		ps = pe;
	}

end_gamma_parse:
	apical_api_calibration(CALIBRATION_GAMMA_LINEAR, COMMAND_SET, gamma, 1*129*2, &ret);
#if 0
	for (i = 0; i < 129; i++) {
		APICAL_WRITE_32(0x10400+4*i, gamma[i]);
	}
#endif
	printk("isp gamma set ok\n");
	kfree(buf);
	return count;

err_gamma_parse:
	printk("err_gamma_parse\n");
	kfree(buf);
	return count;
}

static const struct file_operations isp_gamma_proc_fops ={
	.read = seq_read,
	.open = dump_isp_gamma_open,
	.llseek = seq_lseek,
	.release = single_release,
	.write = isp_gamma_put,
};


/* hilight func info */
static int isp_de_hilight_show(struct seq_file *m, void *v)
{
	int len = 0;
	struct tx_isp_core_device *core = m->private;
	struct v4l2_control ctrl;

	if(atomic_read(&core->state) < TX_ISP_STATE_RUN){
		len += seq_printf(m ,"sensor doesn't work, please enable sensor\n");
		return len;
	}

	ctrl.id = IMAGE_TUNING_CID_HILIGHT_DEPRESS_STRENGTH;
	ctrl.value = 0;
	v4l2_subdev_call(&core->sd, core, g_ctrl, &ctrl);
	len += seq_printf(m ,"de_hilight_strength:%d\n", ctrl.value);

	len += seq_printf(m ,"hist0:%d\n", apical_isp_metering_hist_0_read());
	len += seq_printf(m ,"hist1:%d\n", apical_isp_metering_hist_1_read());
	len += seq_printf(m ,"hist3:%d\n", apical_isp_metering_hist_3_read());
	len += seq_printf(m ,"hist4:%d\n", apical_isp_metering_hist_4_read());

	return len;
}
static ssize_t isp_de_hilight_strength_set(struct file *file, const char __user *buffer, size_t count, loff_t *f_pos)
{
	unsigned v = 0;
	struct seq_file *m = file->private_data;
	struct tx_isp_core_device *core = m->private;
	struct v4l2_control ctrl;

	char *buf = kzalloc((count+1), GFP_KERNEL);
	if(!buf)
		return -ENOMEM;
	if(copy_from_user(buf, buffer, count))
	{
		kfree(buf);
		return EFAULT;
	}
	v = simple_strtoull(buf, NULL, 0);
	ctrl.id = IMAGE_TUNING_CID_HILIGHT_DEPRESS_STRENGTH;
	ctrl.value = v;
	v4l2_subdev_call(&core->sd, core, s_ctrl, &ctrl);
	kfree(buf);
	return count;
}
static int dump_isp_de_hilight_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, isp_de_hilight_show, PDE_DATA(inode),8192);
}
static const struct file_operations isp_de_hilight_fops ={
	.read = seq_read,
	.open = dump_isp_de_hilight_open,
	.llseek = seq_lseek,
	.release = single_release,
	.write = isp_de_hilight_strength_set,
};



/* cmd */
#define ISP_CMD_BUF_SIZE 100
static uint8_t isp_cmd_buf[100];
static int isp_cmd_show(struct seq_file *m, void *v)
{
	int len = 0;
	struct tx_isp_core_device *core = m->private;
	if(atomic_read(&core->state) < TX_ISP_STATE_RUN){
		len += seq_printf(m ,"sensor doesn't work, please enable sensor\n");
		return len;
	}

	len += seq_printf(m ,"%s\n", isp_cmd_buf);
	return len;
}
#ifdef CONFIG_VIDEO_ADV_DEBUG
int isp_sen_reg_read(struct tx_isp_core_device *core, uint32_t reg, uint32_t *val)
{
	int ret = 0;
	struct v4l2_device *v4l2_dev = core->sd.v4l2_dev;
	struct v4l2_subdev *sd;
	struct list_head *pos;
	int find = 0;
	struct tx_isp_video_in_device *vi = 0;
	struct tx_isp_sensor *active;
	struct v4l2_dbg_register dbg_reg;

	if(atomic_read(&core->state) < TX_ISP_STATE_RUN){
		ret = -1;
		goto err_isp_state;
	}

	list_for_each(pos, &v4l2_dev->subdevs) {
		sd = list_entry(pos, struct v4l2_subdev, list);
		if (!strcmp(sd->name, "tx-isp-video-in")) {
			find = 1;
			break;
		}
	}
	if (1 != find) {
		ret = -1;
		goto err_find_video_in;
	}
	vi = sd_to_tx_video_in_device(sd);
	active = vi->active;
	if (NULL == active) {
		ret = -1;
		goto err_active_sensor;
	}
	memset(&dbg_reg, 0, sizeof(dbg_reg));
	dbg_reg.match.type = V4L2_CHIP_MATCH_SUBDEV;
	dbg_reg.reg = reg;
	v4l2_subdev_call(&active->sd, core, g_register, &dbg_reg);
	*val = dbg_reg.val;

	return ret;
err_active_sensor:
err_find_video_in:
err_isp_state:

	printk("##### err %s,%d\n", __func__, __LINE__);
	*val = 0;
	return ret;
}
static int isp_sen_reg_write(struct tx_isp_core_device *core, uint32_t reg, uint32_t val)
{
	int ret = 0;
	struct v4l2_device *v4l2_dev = core->sd.v4l2_dev;
	struct v4l2_subdev *sd;
	struct list_head *pos;
	int find = 0;
	struct tx_isp_video_in_device *vi = 0;
	struct tx_isp_sensor *active;
	struct v4l2_dbg_register dbg_reg;

	if(atomic_read(&core->state) < TX_ISP_STATE_RUN){
		ret = -1;
		goto err_isp_state;
	}

	list_for_each(pos, &v4l2_dev->subdevs) {
		sd = list_entry(pos, struct v4l2_subdev, list);
		if (!strcmp(sd->name, "tx-isp-video-in")) {
			find = 1;
			break;
		}
	}
	if (1 != find) {
		ret = -1;
		goto err_find_video_in;
	}
	vi = sd_to_tx_video_in_device(sd);
	active = vi->active;
	if (NULL == active) {
		ret = -1;
		goto err_active_sensor;
	}
	memset(&dbg_reg, 0, sizeof(dbg_reg));
	dbg_reg.match.type = V4L2_CHIP_MATCH_SUBDEV;
	dbg_reg.reg = reg;
	dbg_reg.val = val;
	ret = v4l2_subdev_call(&active->sd, core, s_register, &dbg_reg);
	return ret;
err_active_sensor:
err_find_video_in:
err_isp_state:

	printk("##### err %s,%d\n", __func__, __LINE__);
	return ret;
}
#endif
static ssize_t isp_cmd_set(struct file *file, const char __user *buffer, size_t count, loff_t *f_pos)
{
	int ret = 0;
	struct seq_file *m = file->private_data;
	struct tx_isp_core_device *core = m->private;

	char *buf = kzalloc((count+1), GFP_KERNEL);
	if(!buf)
		return -ENOMEM;
	if(copy_from_user(buf, buffer, count))
	{
		kfree(buf);
		return EFAULT;
	}
	//printk("##### %s\n", buf);
#ifdef CONFIG_VIDEO_ADV_DEBUG
	if (!strncmp(buf, "r sen_reg", sizeof("r sen_reg")-1)) {
		unsigned reg = 0;
		unsigned val = 0;
		reg = simple_strtoull(buf+sizeof("r sen_reg"), NULL, 0);
		ret = isp_sen_reg_read(core, reg, &val);
		if (ret)
			printk("##### err %s.%d\n", __func__, __LINE__);
		printk("isp: sensor reg read 0x%x(0x%x)\n", reg, val);
		sprintf(isp_cmd_buf, "0x%x\n", val);
	} else if (!strncmp(buf, "w sen_reg", sizeof("w sen_reg")-1)) {
		unsigned reg = 0;
		unsigned val = 0;
		char *p = 0;
		reg = simple_strtoull(buf+sizeof("w sen_reg"), &p, 0);
		val = simple_strtoull(p+1, NULL, 0);
		printk("isp: sensor reg write 0x%x(0x%x)\n", reg, val);
		ret = isp_sen_reg_write(core, reg, val);
		if (!ret)
			sprintf(isp_cmd_buf, "%s\n", "ok");
		else
			sprintf(isp_cmd_buf, "%s\n", "nok");
#else
		if (0) {
			;
#endif
		} else {
			sprintf(isp_cmd_buf, "null");
		}
		kfree(buf);
		return count;
}
static int isp_cmd_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, isp_cmd_show, PDE_DATA(inode),8192);
}
static const struct file_operations isp_cmd_fops ={
	.read = seq_read,
	.open = isp_cmd_open,
	.llseek = seq_lseek,
	.release = single_release,
	.write = isp_cmd_set,
};


static const struct media_entity_operations core_media_ops = {
	.link_setup = NULL,
};

int register_tx_isp_core_device(struct platform_device *pdev, struct v4l2_device *v4l2_dev)
{
	struct tx_isp_subdev_platform_data *pdata = pdev->dev.platform_data;
	struct tx_isp_core_device *core_dev = NULL;
	struct resource *res = NULL;
	struct v4l2_subdev *sd = NULL;
	struct media_pad *pads = NULL;
	struct media_entity *me = NULL;
	struct proc_dir_entry *proc;
	int ret;

	if(!pdata){
		v4l2_err(v4l2_dev, "The platform_data of csi is NULL!\n");
		ret = -ISP_ERROR;
		goto exit;
	};
	core_dev = (struct tx_isp_core_device *)kzalloc(sizeof(*core_dev), GFP_KERNEL);
	if(!core_dev){
		v4l2_err(v4l2_dev, "Failed to allocate sensor device\n");
		ret = -ENOMEM;
		goto exit;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	res = request_mem_region(res->start,
			res->end - res->start + 1, dev_name(&pdev->dev));
	if (!res) {
		v4l2_err(v4l2_dev, "Not enough memory for resources\n");
		ret = -EBUSY;
		goto mem_region_failed;
	}
	core_dev->base = ioremap(res->start, res->end - res->start + 1);
	if (!core_dev->base) {
		v4l2_err(v4l2_dev, "Unable to ioremap registers!\n");
		ret = -ENXIO;
		goto ioremap_failed;
	}
	core_dev->res = res;
	spin_lock_init(&core_dev->slock);
	mutex_init(&core_dev->mlock);
	core_dev->refcnt = 0;
	core_dev->pdata = pdata;
	core_dev->dev = &pdev->dev;
	sd = &core_dev->sd;
	pads = core_dev->pads;
	me = &sd->entity;

	/*printk("&&&&&&&&&&& %s:slock = %p, mlock = %p &&&&&&&&&&\n",__func__,*/
			/*&core_dev->slock, &core_dev->mlock);*/
	v4l2_subdev_init(sd, &isp_core_ops);
	strlcpy(sd->name, "tx-isp-core-subdev", sizeof(sd->name));

	sd->grp_id = pdata->grp_id ;	/* group ID for isp subdevs */
	v4l2_set_subdevdata(sd, core_dev);

	pads[TX_ISP_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	pads[TX_ISP_PAD_LINK].flags = MEDIA_PAD_FL_SINK;

	me->ops = &core_media_ops;
	//	me->parent = v4l2_dev->mdev;
	ret = media_entity_init(me, TX_ISP_PADS_NUM, pads, 0);
	if (ret < 0){
		v4l2_err(v4l2_dev, "Failed to init media entity!\n");
		ret = -ISP_ERROR;
		goto entity_init_failed;
	}

	ret = v4l2_device_register_subdev(v4l2_dev, sd);
	if (ret < 0){
		v4l2_err(v4l2_dev, "Failed to register csi-subdev!\n");
		ret = -ISP_ERROR;
		goto register_failed;
	}

	ret = isp_core_init_clk(core_dev);
	if(ret < 0){
		v4l2_err(v4l2_dev, "Failed to init isp's clks!\n");
		ret = -ISP_ERROR;
		goto failed_to_init_clk;
	}

	/* the frame buffer manager */
	core_dev->vbm = frame_buffer_manager_create(core_dev->dev);
	if(!core_dev->vbm){
		v4l2_err(v4l2_dev, "Failed to create the manager of frame buffer!\n");
		ret = -ISP_ERROR;
		goto failed_to_frame_buffer;
	}

	/* the video node of image tuning is registered */
	core_dev->tun = tx_isp_image_tuning_device_register(sd);
	if(core_dev->tun == NULL){
		v4l2_err(v4l2_dev, "Failed to init image tuning node!\n");
		ret = -ISP_ERROR;
		goto failed_to_image_tuning;
	}

	/* init isp's dma channel */
	ret = isp_core_frame_channel_init(core_dev);
	if(ret){
		v4l2_err(v4l2_dev, "Failed to init frame channels!\n");
		ret = -ISP_ERROR;
		goto failed_to_frame_channel;
	}

	/* init v4l2_priority */
	v4l2_prio_init(&core_dev->prio);

	atomic_set(&core_dev->state, TX_ISP_STATE_STOP);
	platform_set_drvdata(pdev, core_dev);

	/* apical init */
	system_isp_set_base_address(core_dev->base);
	apical_sensor_early_init(core_dev);
	isp_set_interrupt_ops(sd);

	/* creat the node of printing isp info */
	proc = jz_proc_mkdir("isp");
	if (!proc) {
		v4l2_err(v4l2_dev, "create dev_attr_isp_info failed!\n");
		printk("################## %s %d ############################\n",__func__,__LINE__);
	}
	proc_create_data("isp_info", S_IRUGO, proc, &isp_info_proc_fops, (void *)core_dev);
	proc_create_data("isp_gamma", S_IRUGO, proc, &isp_gamma_proc_fops, (void *)core_dev);
	proc_create_data("isp_de_hilight", S_IRUGO, proc, &isp_de_hilight_fops, (void *)core_dev);

	proc_create_data("cmd", S_IRUGO, proc, &isp_cmd_fops, (void *)core_dev);

	core_dev->proc = proc;
	return ISP_SUCCESS;
failed_to_frame_channel:
	tx_isp_image_tuning_device_release(core_dev->tun);
failed_to_image_tuning:
	frame_buffer_manager_cleanup(core_dev->vbm);
failed_to_frame_buffer:
	isp_core_release_clk(core_dev);
failed_to_init_clk:
	v4l2_device_unregister_subdev(sd);
register_failed:
	media_entity_cleanup(me);
entity_init_failed:
	iounmap(core_dev->base);
ioremap_failed:
	release_mem_region(res->start, res->end - res->start + 1);
mem_region_failed:
	kfree(core_dev);
exit:
	return ret;
}

void release_tx_isp_core_device(struct v4l2_subdev *sd)
{
	struct tx_isp_core_device *core = v4l2_get_subdevdata(sd);

	tx_isp_image_tuning_device_release(core->tun);
	isp_core_frame_channel_deinit(core);
	frame_buffer_manager_cleanup(core->vbm);
	free_tx_isp_priv_param_manage();

	isp_core_release_clk(core);
	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);

	iounmap(core->base);
	release_mem_region(core->res->start,core->res->end - core->res->start + 1);
	if (core->proc)
		proc_remove(core->proc);
	kfree(core);
}
