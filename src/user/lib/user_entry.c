#include <user/syscall.h>
#include <user/user_entry.h>

__attribute__((section(".user_text.init"))) void user_entry_point(void app(void))
{
	app();
	exit(0);
}
