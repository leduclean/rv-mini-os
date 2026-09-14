#include "user_entry.h"

#include "syscall.h"

__attribute__((section(".user_text.init"))) void user_entry_point(void app())
{
	app();
	exit(0);
}
