/**
 * @file
 * @brief RISC-V control and status registers: accessors and bit layouts.
 */

#pragma once

/* Privilege modes, as encoded in mstatus.MPP */
#define PRV_U 0
#define PRV_S 1
#define PRV_M 3

/* Bits in mstatus */
#define MSTATUS_MIE (1 << 3)
#define MSTATUS_MPIE (1 << 7)
#define MSTATUS_MPP_SHIFT 11
#define MSTATUS_MPP_MASK (0b11 << MSTATUS_MPP_SHIFT)
#define MSTATUS_SUM (1 << 18)
#define MSTATUS_MXR (1 << 19)

/* Bits in sstatus */
#define SSTATUS_SIE (1UL << 1)
#define SSTATUS_SPIE (1UL << 5)
#define SSTATUS_SPP (1UL << 8)

/* Bits in menvcfg / mcounteren */
#define MENVCFG_STCE (1UL << 63) /* stimecmp enable */
#define MCOUNTEREN_TM (1UL << 1) /* Timer counter enable */

/* Set in mcause/scause when the cause is an interrupt, not an exception */
#define XCAUSE_IRQ_BIT (1UL << 63)

/* Machine interrupt causes */
#define M_IRQ_TMR 7 /* Machine timer interrupt */
#define M_IRQ_EXT 11 /* Machine external interrupt */

/* Supervisor interrupt causes */
#define S_IRQ_TMR 5 /* Supervisor timer interrupt */
#define S_IRQ_EXT 9 /* Supervisor external interrupt */

/* Bits in mie / sie, one per cause above */
#define MIE_MTIE (1UL << M_IRQ_TMR)
#define MIE_MEIE (1UL << M_IRQ_EXT)
#define SIE_STIE (1UL << S_IRQ_TMR)
#define SIE_SEIE (1UL << S_IRQ_EXT)

/* Exception causes */
#define ECALL_UMODE 8
#define ECALL_SMODE 9
#define ECALL_MMODE 11
#define INSTRUCTION_PAGE_FAULT 12
#define LOAD_PAGE_FAULT 13
#define STORE_PAGE_FAULT 15

#ifndef __ASSEMBLER__

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

#endif /* __ASSEMBLER__ */
