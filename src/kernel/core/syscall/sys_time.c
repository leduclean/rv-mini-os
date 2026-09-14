#include <kernel/scheduler.h>

void sys_sleep(uint32_t s)
{
	scheduler_sleep(s);
}
