#include <tx-isp-common.h>
#include <apical-isp/apical_types.h>

void system_reset_sensor(uint32_t mask)
{
	printk("^^^ %s  ^^^\n",__func__);
	dump_stack();
}
