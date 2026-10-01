#include <stdint.h>

#include <user/syscall.h>
#include <user/thread.h>

void thread_entry(thread_entry_t *wrap)
{
	wrap->fn(wrap->args);
	exit(0);
}

uint8_t thread_create(thread_entry_t *wrap, void *stack)
{
	return clone(thread_entry, (void *)wrap, stack, CLONE_VM);
}

int thread_join(uint8_t tid)
{
	return waitpid(tid);
}
