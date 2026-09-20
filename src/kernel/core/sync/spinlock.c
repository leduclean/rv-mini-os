#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include <kernel/spinlock.h>

void spinlock_init(spinlock_t *s)
{
	atomic_init(&s->flag, 0);
}

void spinlock_acquire(spinlock_t *s)
{
	while (atomic_exchange_explicit(&s->flag, 1, memory_order_acquire)) {
	}
}

void spinlock_release(spinlock_t *s)
{
	atomic_store_explicit(&s->flag, 0, memory_order_release);
}
