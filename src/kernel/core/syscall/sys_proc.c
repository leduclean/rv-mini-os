#include "process.h"
#include "scheduler.h"
#include "minilib/stdint.h"

uint8_t sys_get_pid()
{
	return get_active_pid();
}

int8_t sys_fork()
{
	//TODO: implement this function
	return 0;
}

void sys_exit(int8_t code)
{
	scheduler_terminate(code);
}
