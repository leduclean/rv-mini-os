#ifndef __PLATFORM_H__
#define __PLATFORM_H__

// Video memory base address
#define BOCHS_DISPLAY_BASE_ADDRESS 0x50000000
#define BOCHS_CONFIG_BASE_ADDRESS 0x40000000
#define BOCHS_CONFIG_DISPI_ADDRESS 0x500

// Config registers addresses
#define VBE_DISPI_INDEX_ID 0x00
#define VBE_DISPI_INDEX_XRES 0x01
#define VBE_DISPI_INDEX_YRES 0x02
#define VBE_DISPI_INDEX_BPP 0x03
#define VBE_DISPI_INDEX_ENABLE 0x04
#define VBE_DISPI_INDEX_BANK 0x05
#define VBE_DISPI_INDEX_VIRT_WIDTH 0x06
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x07
#define VBE_DISPI_INDEX_X_OFFSET 0x08
#define VBE_DISPI_INDEX_Y_OFFSET 0x09
#define VBE_DISPI_INDEX_VIDEO_MEMORY_64K 0x0a
#define VBE_DISPI_INDEX_ENDIAN 0x82

#define VBE_DISPI_ID0 0xb0c0
#define VBE_DISPI_ID1 0xb0c1
#define VBE_DISPI_ID2 0xb0c2
#define VBE_DISPI_ID3 0xb0c3
#define VBE_DISPI_ID4 0xb0c4
#define VBE_DISPI_ID5 0xb0c5

#define VBE_DISPI_DISABLED 0x00
#define VBE_DISPI_ENABLED 0x01
#define VBE_DISPI_GETCAPS 0x02
#define VBE_DISPI_8BIT_DAC 0x20
#define VBE_DISPI_LFB_ENABLED 0x40
#define VBE_DISPI_NOCLEARMEM 0x80

#define VBE_MAX_WIDTH 1024
#define VBE_MAX_HEIGHT 768

#define DISPLAY_ENDIAN 0x1e1e1e1e
#define DISPLAY_WIDTH 1024
#define DISPLAY_HEIGHT 768
#define DISPLAY_BPP 32
#define DISPLAY_BANK 0
#define DISPLAY_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT)
#define DISPLAY_BYTES (DISPLAY_WIDTH * DISPLAY_HEIGHT * (DISPLAY_BPP / 8))

// Info bus PCI
#define PCI_ECAM_BASE_ADDRESS 0x30000000
#define DISPLAY_PCI_ID 0x11111234

// Shift definition for PCI config
#define PCI_BUS_SHIFT 16
#define PCI_DEVICE_SHIFT 11
#define PCI_FUNC_SHIFT 8
#define PCI_MAX_DEVICE 32

// Device registers mapping
#define PCI_DEV_COMMAND 0x04
#define PCI_DEV_DISPLAY_ADDR 0x10
#define PCI_DEV_CONFIG_ADDR 0x18

// PLIC registers addresses
#define PLIC_PENDING 0x0c001000
#define PLIC_SOURCE 0x0c000000
#define PLIC_ENABLE_S 0x0c002080
#define PLIC_TARGET_S 0x0c201000
#define PLIC_IRQ_CLAIM_S 0x0c201004
#define PLIC_MMIO_BASE 0x0c000000UL
#define PLIC_MMIO_SIZE 0x202000UL

// PLIC pushbutton irq
#define PLIC_IRQ_2 0x2
#define PLIC_UART_ID 10
#define PLIC_ENABLE_UART (1 << PLIC_UART_ID)

// Timer options
#define TIMER_FREQ 10000000 // 10MHz
#define TIMER_RATIO 500

#define MENVCFG_STCE (1UL << 63) /* stimecmp enable */
#define MCOUNTEREN_TM (1UL << 1) /* Timer counter enable */

#define XCAUSE_IRQ_BIT (1UL << 63)

// Mcause irq flags
#define M_IRQ_TMR 7 /* Machine time interrupt */
#define M_IRQ_EXT 11 /* Maching external interrupt */

// Scause irq flags
#define S_IRQ_TMR 5 /* Supervisor timer interrupt bit */
#define S_IRQ_EXT 9 /* Supervisor external interrupt bit */

#define MIE_MTIE (1UL << M_IRQ_TMR)
#define MIE_MEIE (1UL << M_IRQ_EXT)
#define SIE_STIE (1UL << S_IRQ_TMR)
#define SIE_SEIE (1UL << S_IRQ_EXT)

// Ecall values
#define ECALL_UMODE 8
#define ECALL_SMODE 9
#define ECALL_MMODE 11
#define INSTRUCTION_PAGE_FAULT 12
#define LOAD_PAGE_FAULT 13
#define STORE_PAGE_FAULT 15

// Bit in mstatus
#define MSTATUS_MIE (1 << 3)
#define MSTATUS_MPIE (1 << 7)
#define MSTATUS_MPP_SHIFT 11
#define MSTATUS_MPP_MASK (0b11 << MSTATUS_MPP_SHIFT)
#define MSTATUS_SUM (1 << 18)
#define MSTATUS_MXR (1 << 19)

// Bit in sstatus
#define SSTATUS_SIE (1UL << 1)
#define SSTATUS_SPIE (1UL << 5)
#define SSTATUS_SPP (1UL << 8)

// Privilege mode
#define U 0
#define S 1
#define M 3

// UART
#define UART_BASE 0x10000000
#define UART_CLOCK_FREQ 1843200
#define UART_BAUD_RATE 115200

#define UART_IER_MSI 0x08 /* Enable Modem status interrupt */
#define UART_IER_RLSI 0x04 /* Enable receiver line status interrupt */
#define UART_IER_THRI 0x02 /* Enable Transmitter holding register int. */
#define UART_IER_RDI 0x01 /* Enable receiver data interrupt */

#define UART_IIR_NO_INT 0x01 /* No interrupts pending */
#define UART_IIR_ID 0x06 /* Mask for the interrupt ID */

#ifndef __ASSEMBLER__
#include <stdint.h>

enum {
	UART_RBR = 0x00, /* Receive Buffer Register */
	UART_THR = 0x00, /* Transmit Hold Register */
	UART_IER = 0x01, /* Interrupt Enable Register */
	UART_DLL = 0x00, /* Divisor LSB (LCR_DLAB) */
	UART_DLH = 0x01, /* Divisor MSB (LCR_DLAB) */
	UART_FCR = 0x02, /* FIFO Control Register */
	UART_LCR = 0x03, /* Line Control Register */
	UART_MCR = 0x04, /* Modem Control Register */
	UART_LSR = 0x05, /* Line Status Register */
	UART_MSR = 0x06, /* Modem Status Register */
	UART_SCR = 0x07, /* Scratch Register */

	UART_FCR_EWL = 0x01, /* Waiting list enable Bit */
	UART_LCR_DLAB = 0x80, /* Divisor Latch Bit */
	UART_LCR_8BIT = 0x03, /* 8-bit */
	UART_LCR_PODD = 0x08, /* Parity Odd */

	UART_LSR_DA = 0x01, /* Data Available */
	UART_LSR_OE = 0x02, /* Overrun Error */
	UART_LSR_PE = 0x04, /* Parity Error */
	UART_LSR_FE = 0x08, /* Framing Error */
	UART_LSR_BI = 0x10, /* Break indicator */
	UART_LSR_RE = 0x20, /* THR is empty */
	UART_LSR_RI = 0x40, /* THR is empty and line is idle */
	UART_LSR_EF = 0x80, /* Erroneous data in FIFO */
};

enum {
	/* UART Registers */
	UART_REG_TXFIFO = 0,
	UART_REG_RXFIFO = 1,
	UART_REG_TXCTRL = 2,
	UART_REG_RXCTRL = 3,
	UART_REG_IE = 4,
	UART_REG_IP = 5,
	UART_REG_DIV = 6,

	/* TXCTRL register */
	UART_TXEN = 1,
	UART_TXSTOP = 2,

	/* RXCTRL register */
	UART_RXEN = 1,

	/* IP register */
	UART_IP_TXWM = 1,
	UART_IP_RXWM = 2,

	/* INTERRUPT ENABLE */
	UART_RX_IT_EN = 2,
	UART_TX_IT_EN = 1
};

void timer_set(uint32_t period, uint32_t start_value);
void timer_wait(void);
void timer_set_and_wait(uint32_t period, uint32_t time);
void led_set(uint32_t value);
uint32_t push_button_get(void);
#endif

#endif // __PLATFORM_H__
