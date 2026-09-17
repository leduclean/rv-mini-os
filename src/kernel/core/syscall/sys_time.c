#include <kernel/scheduler.h>
#include <kernel/syscall.h>

void sys_sleep(uint32_t s)
{
	scheduler_sleep(s);
}
