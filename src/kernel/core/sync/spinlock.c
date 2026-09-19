#include <stdbool.h>
#include <stdint.h>

#include <kernel/spinlock.h>

extern unsigned long amoswap(unsigned int new, volatile unsigned int *val);
extern unsigned long amoswap_rl(volatile unsigned int *val);

struct spinlock {
	volatile unsigned int flag;
};

void spinlock_init(spinlock_t *s)
{
	s->flag = 0;
}

void spinlock_acquire(spinlock_t *s)
{
	while (amoswap(1, &s->flag) != 0) {
	}
}

void spinlock_release(spinlock_t *s)
{
	amoswap_rl(&s->flag);
}
