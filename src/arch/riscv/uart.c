#include <stdint.h>

#include <lib/stdio.h>

#include <asm/mmio.h>
#include <asm/platform.h>

#include <drivers/uart.h>

#include <kernel/scheduler.h>
#include <kernel/waitqueue.h>

#define UART_RX_BUFFER_SIZE 128

/**
 * @brief Sleeping queue relative to the uart IO.
 *
 * @note All of its processes should be woken on irq.
 */
static wait_queue_t uart_wait_queue;

static inline void _uart_init_queue(void)
{
	wq_init(&uart_wait_queue);
}

wait_queue_t *uart_get_wait_queue(void)
{
	return &uart_wait_queue;
}

/** @brief Ring buffer used to hold the received chars. */
typedef struct {
	char buf[UART_RX_BUFFER_SIZE]; ///< Storage of the received chars.
	volatile uint32_t head; ///< Index the irq handler writes at.
	volatile uint32_t tail; ///< Index uart_read() reads from.
} uart_ring_buffer_t;

static uart_ring_buffer_t rx_buffer;

static inline int _buffer_empty(uart_ring_buffer_t *b)
{
	return b->head == b->tail;
}

static inline int _buffer_full(uart_ring_buffer_t *b)
{
	return ((b->head + 1) % UART_RX_BUFFER_SIZE) == b->tail;
}

static inline void _buffer_put(uart_ring_buffer_t *b, char c)
{
	if (!_buffer_full(b)) {
		b->buf[b->head] = c;
		b->head = (b->head + 1) % UART_RX_BUFFER_SIZE;
	}
}

static inline char _buffer_get(uart_ring_buffer_t *b)
{
	if (!_buffer_empty(b)) {
		char c = b->buf[b->tail];
		b->tail = (b->tail + 1) % UART_RX_BUFFER_SIZE;
		return c;
	}
	return 0;
}

static inline void _uart_enable_fifo(void)
{
	// Enable fifo waiting list
	MMIO8(UART_BASE + UART_FCR) = UART_FCR_EWL;
}

static inline void _uart_config_lcr(void)
{
	// 8 bits transmition/reception config
	MMIO8(UART_BASE + UART_LCR) |= UART_LCR_8BIT |
				       UART_LCR_PODD; // bits 0, 1, 3
}

static void _uart_set_baud(void)
{
	uint16_t rate = (UART_CLOCK_FREQ / (16 * UART_BAUD_RATE));
	uintptr_t lcr_addr = UART_BASE + UART_LCR;

	// Enable divisor latches
	MMIO8(lcr_addr) |= UART_LCR_DLAB;

	// Divisor rate writing
	MMIO8(UART_BASE + UART_DLL) = (uint8_t)(rate);
	MMIO8(UART_BASE + UART_DLH) = (uint8_t)(rate >> 8);

	// Disable divisor latches to acces THR, RBR, IER
	MMIO8(lcr_addr) &= ~UART_LCR_DLAB;
}

/** @brief Enable the RX interupt. */
static inline void _uart_enable_rxirq(void)
{
	MMIO8(UART_BASE + UART_IER) |= UART_RXEN;
}

void uart_init(void)
{
	_uart_init_queue();
	_uart_set_baud();
	_uart_enable_fifo();
	_uart_config_lcr();
	_uart_enable_rxirq();
};

void uart_putchar(char c)
{
	MMIO8(UART_BASE + UART_THR) = c;
};

/**
 * @brief Get a character from the RBR fifo buffer, a received char.
 *
 * @return The received character.
 */
static char _uart_getchar(void)
{
	return MMIO8(UART_BASE + UART_RBR);
};

/**
 * @brief Signals that data is available in rx.
 *
 * @return 1 if a character can be read, 0 otherwise.
 */
static inline uint8_t _uart_rx_data_ready(void)
{
	return MMIO8(UART_BASE + UART_LSR) & 1;
}

/** @brief Fill the rx ring buffer while rx contains data. */
static void _uart_fill_rx_buff(void)
{
	while (_uart_rx_data_ready()) {
		_buffer_put(&rx_buffer, _uart_getchar());
	}
}

//TODO: Maybe it should take a size_t arg to specify how much we want to read
int uart_read(char *c)
{
	if (!_buffer_empty(&rx_buffer)) {
		*c = _buffer_get(&rx_buffer);
		return 0;
	}
	return -1;
}

void uart_irq_handler(void)
{
	_uart_fill_rx_buff();
	scheduler_wake_waiting_queue(&uart_wait_queue);
}
