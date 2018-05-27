#ifndef _COMMON_CODEC_H
#define _COMMON_CODEC_H
struct codec_operation {
	struct resource *res;
	void __iomem *iomem;
	struct device *dev;
	char name[16];
	void *priv;
};
#endif

