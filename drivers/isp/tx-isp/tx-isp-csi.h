#ifndef __TX_ISP_CSI_H__
#define __TX_ISP_CSI_H__

#include <tx-isp-common.h>

/* csi host regs, base addr should be defined in board cfg */
#define VERSION				0x00
#define N_LANES				0x04
#define PHY_SHUTDOWNZ			0x08
#define DPHY_RSTZ			0x0C
#define CSI2_RESETN			0x10
#define PHY_STATE			0x14
#define DATA_IDS_1			0x18
#define DATA_IDS_2			0x1C
#define ERR1				0x20
#define ERR2				0x24
#define MASK1				0x28
#define MASK2				0x2C
#define PHY_TST_CTRL0       0x30
#define PHY_TST_CTRL1       0x34

typedef enum
{
	ERR_NOT_INIT = 0xFE,
	ERR_ALREADY_INIT = 0xFD,
	ERR_NOT_COMPATIBLE = 0xFC,
	ERR_UNDEFINED = 0xFB,
	ERR_OUT_OF_BOUND = 0xFA,
	SUCCESS = 0
} csi_error_t;

struct tx_isp_csi_driver {
	struct device *dev;
	struct v4l2_subdev sd;
	struct media_pad pads[TX_ISP_PADS_NUM];
	struct resource *res;
	void __iomem *base;
	struct tx_isp_video_in vin;
	struct clk **clks;
	unsigned int clk_num;
	void * pdata;
	unsigned int lans;
};

#define csi_readl(port,reg)					\
	__raw_readl((unsigned int *)(port->base + reg))
#define csi_writel(port,reg, value)					\
	__raw_writel((value), (unsigned int *)(port->base + reg))

#define csi_core_write(port,addr, value) csi_writel(port,addr, value)
#define csi_core_read(port,addr) csi_readl(port,addr)

#define sd_to_tx_isp_csi_driver(sd) (container_of(sd, struct tx_isp_csi_driver, sd))
int register_tx_isp_csi_device(struct platform_device *pdev, struct v4l2_device *v4l2_dev);
void release_tx_isp_csi_device(struct v4l2_subdev *sd);
void check_csi_error(void);
#endif /* __TX_ISP_CSI_H__  */
