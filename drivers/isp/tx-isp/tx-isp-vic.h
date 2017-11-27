#ifndef __TX_ISP_VIC_H__
#define __TX_ISP_VIC_H__

#include <tx-isp-common.h>
#include <tx-vic-regs.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <jz_proc.h>
struct tx_isp_vic_driver {
	struct device *dev;
	struct v4l2_subdev sd;
	struct media_pad pads[TX_ISP_PADS_NUM];
	struct resource *res;
	struct tx_isp_video_in vin;
	void __iomem *base;
	void * pdata;
	struct clk **clks;
	unsigned int clk_num;
	/* vic frd interrupt cnt */
	struct proc_dir_entry *proc_vic;
	unsigned int vic_frd_c;

};

#define tx_isp_vic_readl(port,reg)						\
	__raw_readl((volatile unsigned int *)(port->base + reg))
#define tx_isp_vic_writel(port,reg, value)					\
	__raw_writel((value), (volatile unsigned int *)(port->base + reg))
#define sd_to_tx_isp_vic_driver(sd) (container_of(sd, struct tx_isp_vic_driver, sd))
int register_tx_isp_vic_device(struct platform_device *pdev, struct v4l2_device *v4l2_dev);
void release_tx_isp_vic_device(struct v4l2_subdev *sd);
void check_vic_error(void);
#endif /* __TX_ISP_VIC_H__ */
