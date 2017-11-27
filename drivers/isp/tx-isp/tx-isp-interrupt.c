#include <tx-isp-common.h>
#include "tx-isp-interrupt.h"

static unsigned int refcnt = 0;
static void tx_isp_enable_irq(struct tx_isp_irq_device *irq_dev, int enable)
{
	volatile unsigned int reg = tx_isp_readl(irq_dev->base, TX_ISP_TOP_IRQ_ENABLE);
	unsigned long flags;
	reg |= enable;
	tx_isp_writel(irq_dev->base, TX_ISP_TOP_IRQ_ENABLE, reg);
	spin_lock_irqsave(&irq_dev->slock, flags);
	if(irq_dev->state == 0){
		irq_dev->state = 1;
		refcnt++;
		enable_irq(irq_dev->irq);
//		printk("mask=0x%08x enable=0x%08x\n", tx_isp_readl(irq_dev->base, TX_ISP_TOP_IRQ_MASK), reg);
//		printk("++ refcnt = %d ++\n",refcnt);
	}
	spin_unlock_irqrestore(&irq_dev->slock, flags);
}
static void tx_isp_disable_irq(struct tx_isp_irq_device *irq_dev, int enable)
{
	volatile unsigned int reg = tx_isp_readl(irq_dev->base, TX_ISP_TOP_IRQ_ENABLE);
	unsigned long flags;
	spin_lock_irqsave(&irq_dev->slock, flags);
	reg &= ~enable;
	tx_isp_writel(irq_dev->base, TX_ISP_TOP_IRQ_ENABLE, reg);
	if(reg == 0 && irq_dev->state){
		irq_dev->state = 0;
		refcnt--;
		disable_irq(irq_dev->irq);
//		printk("-- refcnt = %d --\n",refcnt);
	}
	spin_unlock_irqrestore(&irq_dev->slock, flags);
}
static void tx_isp_mask_irq(struct tx_isp_irq_device *irq_dev, int mask)
{
	volatile unsigned int reg = tx_isp_readl(irq_dev->base, TX_ISP_TOP_IRQ_MASK);
	reg |= mask;
	tx_isp_writel(irq_dev->base, TX_ISP_TOP_IRQ_MASK, reg);
}
static void tx_isp_unmask_irq(struct tx_isp_irq_device *irq_dev, int mask)
{
	volatile unsigned int reg = tx_isp_readl(irq_dev->base, TX_ISP_TOP_IRQ_MASK);
	reg &= ~mask;
	tx_isp_writel(irq_dev->base, TX_ISP_TOP_IRQ_MASK, reg);
}

static inline void tx_isp_clear_irq(struct tx_isp_irq_device *irq_dev, unsigned int clear)
{
	tx_isp_writel(irq_dev->base, TX_ISP_TOP_IRQ_CLR_1, clear);
}

static irqreturn_t isp_irq_handle(int this_irq, void *dev)
{
	tx_isp_device_t *ispdev = (tx_isp_device_t *)dev;
	struct tx_isp_media_pipeline *p = &ispdev->pipeline;
	struct tx_isp_irq_device *irqdev = ispdev->irq_dev;
	struct v4l2_subdev *sd = NULL;
	volatile unsigned int state, pending, mask;
	int index = 0;
	int ret = IRQ_HANDLED;
	irqreturn_t retval = IRQ_HANDLED;
//	if(irqdev == NULL)
//		return IRQ_HANDLED;

	mask = tx_isp_readl(irqdev->base, TX_ISP_TOP_IRQ_MASK);
	state = tx_isp_readl(irqdev->base, TX_ISP_TOP_IRQ_STA);
	pending = state & (~mask);
	if(pending){
		tx_isp_clear_irq(irqdev, pending);
		while(index < TX_ISP_MAX_GRP_IDX && p->subdevs[index]){
			sd = p->subdevs[index];
			ret = v4l2_subdev_call(sd, core, interrupt_service_routine, pending, NULL);
			if(ret == IRQ_WAKE_THREAD)
				retval = IRQ_WAKE_THREAD;
			index++;
		}
	}
	return retval;
}

extern irqreturn_t isp_irq_thread_handle(int this_irq, void *dev);
int tx_isp_request_irq(struct platform_device *pdev, tx_isp_device_t *ispdev)
{
	struct v4l2_device *v4l2_dev;
	struct resource *res;
	struct tx_isp_irq_device *irqdev;
	int irq;
	int ret = ISP_SUCCESS;

	if(!pdev || !ispdev){
		printk("%s[%d] the parameters are invalid!\n",__func__,__LINE__);
		ret = -EINVAL;
		goto exit;

	}
	v4l2_dev = &ispdev->v4l2_dev;
	irqdev = kzalloc(sizeof(*irqdev), GFP_KERNEL);
	if (!irqdev) {
		v4l2_err(v4l2_dev, "%s[%d] Failed to allocate irq device\n",__func__,__LINE__);
		ret = -ENOMEM;
		goto exit;
	}
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);
	if (!res || !irq) {
		v4l2_err(v4l2_dev, "%s[%d] Not enough platform resources",__func__,__LINE__);
		ret = -ENODEV;
		goto err_resource;
	}

	res = request_mem_region(res->start,
			res->end - res->start + 1, dev_name(&pdev->dev));
	if (!res) {
		v4l2_err(v4l2_dev, "%s[%d] Not enough memory for resources\n", __func__,__LINE__);
		ret = -EBUSY;
		goto err_req_mem;
	}

	irqdev->base = ioremap(res->start, res->end - res->start + 1);
	if (!irqdev->base) {
		v4l2_err(v4l2_dev, "%s[%d] Unable to ioremap registers\n", __func__,__LINE__);
		ret = -ENXIO;
		goto err_ioremap;
	}

	spin_lock_init(&irqdev->slock);
//	tx_isp_mask_irq(irqdev, TX_ISP_TOP_IRQ_ALL);
//	ispdev->irq_dev = irqdev;

	ret = request_threaded_irq(irq, isp_irq_handle, isp_irq_thread_handle, IRQF_ONESHOT, "isp", ispdev);
	if(ret){
		v4l2_err(v4l2_dev, "%s[%d] Failed to request irq(%d).\n", __func__,__LINE__, irq);
		ret = -EINTR;
		goto err_req_irq;
	}

	irqdev->irq = irq;
	irqdev->res = res;
	irqdev->enable_irq = tx_isp_enable_irq;
	irqdev->disable_irq = tx_isp_disable_irq;
	irqdev->mask_irq = tx_isp_mask_irq;
	irqdev->unmask_irq = tx_isp_unmask_irq;
	irqdev->state = 1;
	ispdev->irq_dev = irqdev;
	tx_isp_disable_irq(irqdev, TX_ISP_TOP_IRQ_ALL);
	tx_isp_unmask_irq(irqdev, TX_ISP_TOP_IRQ_ISP);
//	printk("^^ %s[%d] irq = %d ^^\n", __func__,__LINE__,irq);

	return ISP_SUCCESS;
err_req_irq:
	iounmap(irqdev->base);
err_ioremap:
	release_mem_region(res->start, res->end - res->start + 1);
err_req_mem:
err_resource:
	kfree(irqdev);
exit:
	return ret;
}

void tx_isp_free_irq(tx_isp_device_t *ispdev)
{
	struct tx_isp_irq_device *irqdev;
	struct resource *res;
	if(!ispdev || !ispdev->irq_dev){
		return;
	}
	irqdev = ispdev->irq_dev;
	res = irqdev->res;
	free_irq(irqdev->irq, ispdev);
	iounmap(irqdev->base);
	release_mem_region(res->start, res->end - res->start + 1);
	kfree(irqdev);
	ispdev->irq_dev = NULL;
}


