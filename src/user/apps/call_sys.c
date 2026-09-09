#include "syscall.h"
#include "apps.h"
#include "minilib/stdio.h"

#define SLEEP_TIME 2

__attribute__((section(".user_text"))) void sleep_call()
{
	sleep(SLEEP_TIME);
}
