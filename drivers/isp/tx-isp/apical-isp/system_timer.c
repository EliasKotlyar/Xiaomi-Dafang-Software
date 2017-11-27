#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <apical-isp/apical_types.h>

//================================================================================
// timer functions (for FPS calculation)
#if 1
uint32_t system_timer_timestamp(void)
{
	unsigned long long time = sched_clock();
	//	unsigned int ret = 0;
	do_div(time, 1000);
	return (uint32_t)time;
}
uint32_t system_timer_frequency(void)
{
	return 1000000;
}
#else
uint32_t system_timer_timestamp(void)
{
	struct timeval tv;
	unsigned long time = 0;
	do_gettimeofday(&tv);
	time = tv.tv_sec * 1000000 + tv.tv_usec;
	return time;
}
uint32_t system_timer_frequency(void)
{
	return 1000000;
}
#endif
void system_timer_init(void)
{
}
//================================================================================

