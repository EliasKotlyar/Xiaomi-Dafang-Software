#ifndef __DMA_OPS_H__
#define __DMA_OPS_H__

/*void dma_open(void);*/
void dma_config_normal(void);
void dma_config_early_sleep(struct sleep_buffer *);
void dma_set_channel(int channel);

void dma_close(void);



void dma_start(int channel);
void dma_stop(int channel);

void early_sleep_dma_config(unsigned char *buffer, unsigned long len);
#endif




