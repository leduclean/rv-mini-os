#include <stdint.h>

#include "cmd_registry.h"
#include "syscall.h"
#include "test.h"
#include "thread.h"

#define STACK_SIZE 1024

void thread(void *args)
{
	int *i = args;
	(*i)++;
}

void cmd_thread_test([[maybe_unused]] int argc,
		     [[maybe_unused]] char *const argv[])
{
	int i = 0;

	char stack[STACK_SIZE];
	thread_entry_t entry;
	entry.fn = thread;
	entry.args = &i;

	int8_t tid = thread_create(&entry, &stack[STACK_SIZE]);
	if (tid < 0) {
		exit_group(1);
	}

	thread_join(tid);

	if (i == 1) {
		TEST_SUCCESS();
	} else {
		TEST_ERROR();
	}
}
