#if !defined(__SYSTEM_TIMER_H__)
#define __SYSTEM_TIMER_H__

#include "apical_types.h"

uint32_t system_timer_timestamp(void);
void system_timer_init(void);
uint32_t system_timer_frequency(void);

#endif /* __SYSTEM_TIMER_H__ */
