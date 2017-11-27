#include <linux/debugfs.h>
#include "tx-isp-debug.h"

/* -------------------debugfs interface------------------- */
static struct dentry *isp_debug_dir;
static struct dentry *isp_debug_print;
unsigned int isp_print_level = PRINT_LEVEL;

int isp_printf(unsigned int level, unsigned char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int r = 0;

	if(level >= isp_print_level){
		va_start(args, fmt);

		vaf.fmt = fmt;
		vaf.va = &args;

		r = printk("%pV",&vaf);
		va_end(args);
		if(level >= ISP_ERROR_LEVEL)
			dump_stack();
	}
	return r;
}
int isp_debug_init(void)
{
	int ret = 0;
	isp_debug_dir = debugfs_create_dir("isp_debug" , NULL);
	if (!isp_debug_dir) {
		ret = -ENOMEM;
		goto fail;
	}
	isp_debug_print = debugfs_create_u32("isp_print", S_IWUSR | S_IRUSR, isp_debug_dir, &isp_print_level);
	if (!isp_debug_print) {
		ret = -ENOMEM;
		goto fail_u8;
	}
	return ret;
fail_u8:
	debugfs_remove(isp_debug_dir);
fail:
	return ret;
}
int isp_debug_deinit(void)
{
	debugfs_remove(isp_debug_print);
	debugfs_remove(isp_debug_dir);
	return 0;
}
