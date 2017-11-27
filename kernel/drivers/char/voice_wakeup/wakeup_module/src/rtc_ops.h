#ifndef __RTC_OPS_H__
#define __RTC_OPS_H__

#include "rtc-jz.h"

#define pr_info	printk



/*#define __RTC_ALRM_IS_ENABLED()	()*/
/*#define __RTC_ALRM_ENABLE()	()*/


int rtc_init(void);
int rtc_set_alarm(unsigned long alarm_seconds);
int rtc_int_handler(void);
int rtc_exit(void);


#define SYS_TIMER	0x2
#define DMIC_TIMER	0x3

#endif
