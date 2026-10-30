#include <stdint.h>

#include <user/apps.h>
#include <user/thread.h>

#include "test.h"

#define STACK_SIZE 1024

void thread(void *args)
{
	int *i = args;
	(*i)++;
}

void thread_test(void)
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
