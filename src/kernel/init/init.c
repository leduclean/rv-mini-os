#include "drivers/console.h"
#include "arch/riscv/irq.h"
#include "kernel/process/process.h"
#include "shell/shell.h"
#include "drivers/uart.h"
#include "lib/stddef.h"

extern void mon_traitant(void);

void kernel_start() {
  // Plic config
  plic_uart_config();
  init_proc();
  init_ecran();
  uart_init();
  // Interupt handling
  init_trap_entry(mon_traitant);
  enable_external();
  enable_timer();

  spawn_process(shell, "shell", NORMAL);
  // spawn_process(proc1, "bob1", HIGH);
  // spawn_process(proc2, "bob2", NORMAL);
  idle();
}
