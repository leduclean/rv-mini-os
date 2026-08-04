#include "console.h"
#include "cpu.h"
#include "irq.h"
#include "minilib/stddef.h"
#include "process.h"
#include "shell.h"
#include "uart.h"

extern void trap_vector(void);

/** @brief Kernel entry point, called by crt0 once the bss is cleared. */
void kernel_start() {
  // Plic config
  plic_uart_config();
  init_proc();
  init_screen();
  uart_init();

  // Interupt handling
  _enable_it();
  init_trap_entry(trap_vector);
  enable_external();
  enable_timer();

  spawn_process(shell, "shell", NORMAL);
  idle();
}
