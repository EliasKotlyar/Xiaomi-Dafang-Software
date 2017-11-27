#ifndef __TX_ISP_INTERRUPT_H__
#define __TX_ISP_INTERRUPT_H__

#include <linux/errno.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <asm/io.h>

#include "tx-isp-device.h"
#define TX_ISP_TOP_IRQ_CNT		0x0
#define TX_ISP_TOP_IRQ_CNT1		0x20
#define TX_ISP_TOP_IRQ_CNT2		0x24
#define TX_ISP_TOP_IRQ_CLR_1		0x4
#define TX_ISP_TOP_IRQ_CLR_ALL		0x8
#define TX_ISP_TOP_IRQ_STA		0xC
#define TX_ISP_TOP_IRQ_OVF		0x10
#define TX_ISP_TOP_IRQ_ENABLE		0x14
#define TX_ISP_TOP_IRQ_MASK		0x1c
#define TX_ISP_TOP_IRQ_ISP		0xffff
#define TX_ISP_TOP_IRQ_VIC		0x7f0000
#define TX_ISP_TOP_IRQ_ALL		0x7fffff

struct tx_isp_irq_device {
	struct resource *res;
	void __iomem *base;
	volatile int state;
	spinlock_t slock;
	int irq;
	void (*enable_irq)(struct tx_isp_irq_device *irq_dev, int enable);
	void (*disable_irq)(struct tx_isp_irq_device *irq_dev, int enable);
	void (*mask_irq)(struct tx_isp_irq_device *irq_dev, int mask);
	void (*unmask_irq)(struct tx_isp_irq_device *irq_dev, int mask);
};

int tx_isp_request_irq(struct platform_device *pdev, tx_isp_device_t *ispdev);
void tx_isp_free_irq(tx_isp_device_t *ispdev);
#endif /* __TX_ISP_INTERRUPT_H__ */
