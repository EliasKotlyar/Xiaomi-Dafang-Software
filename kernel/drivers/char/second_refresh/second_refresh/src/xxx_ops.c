#include "xxx_ops.h"




int xxx_memcopy(void *src, void *dst, unsigned int len)
{
	char *p;
	char *n;
	p = src;
	while(len--) {
		*n++ = *p++;
	}

	return 0;
}








