#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include <apical-isp/apical.h>

enum {
	LOG_NOTHING,
	LOG_EMERG,
	LOG_ALERT,
	LOG_CRIT,
	LOG_ERR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_INFO,
	LOG_DEBUG,
	LOG_MAX
};

extern const char * const log_level[LOG_MAX];

#define FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

int printk(const char *fmt,...);

#define LOG(level,fmt,...) if((level)<=FW_LOG_LEVEL) printk("%s: %s(%d) %s: " fmt "\n",FILE,__func__,__LINE__,log_level[level],##__VA_ARGS__)

#endif // LOG_H_INCLUDED
