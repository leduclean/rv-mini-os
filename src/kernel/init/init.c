#include "console.h"
#include "cpu.h"
#include "irq.h"
#include "process.h"
#include "shell.h"
#include "minilib/stddef.h"
#include "uart.h"

extern void mon_traitant(void);

void kernel_start() {
  // Plic config
  plic_uart_config();
  init_proc();
  init_screen();
  uart_init();

  // Interupt handling
  enable_it();
  init_trap_entry(mon_traitant);
  enable_external();
  enable_timer();

  spawn_process(shell, "shell", NORMAL);
  idle();
}
