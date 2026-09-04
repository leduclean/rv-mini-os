#pragma once

#define csr_read(csr)                                 \
	({                                            \
		register unsigned long __v;           \
		__asm__ __volatile__("csrr %0, " #csr \
				     : "=r"(__v)      \
				     :                \
				     : "memory");     \
		__v;                                  \
	})

#define csr_write(csr, val)                               \
	do {                                              \
		unsigned long __val = (unsigned long)val; \
		__asm__ __volatile__("csrw " #csr ", %0"  \
				     :                    \
				     : "rK"(__val)        \
				     : "memory");         \
	} while (0)

#define csr_set(csr, val)                                 \
	do {                                              \
		unsigned long __val = (unsigned long)val; \
		__asm__ __volatile__("csrs " #csr ", %0"  \
				     :                    \
				     : "rK"(__val)        \
				     : "memory");         \
	} while (0)

#define csr_clear(csr, val)                               \
	do {                                              \
		unsigned long __val = (unsigned long)val; \
		__asm__ __volatile__("csrc " #csr ", %0"  \
				     :                    \
				     : "r"(__val)         \
				     : "memory");         \
	} while (0)
