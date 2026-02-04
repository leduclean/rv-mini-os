#include "console.h"
#include "irq.h"
#include "process.h"
#include "scheduler.h"
#include "time.h"
#include "uart.h"
#include <stddef.h>
#include <stdio.h>

extern void mon_traitant(void);

void proc1() {
  for (int i = 0; i < 2; i++) {
    printf("[temps = %u] processus %s pid = %i\n", seconds(), get_active_name(),
           get_active_pid());
    scheduler_sleep(2);
  }
}

void proc3() {
  for (;;) {
    printf("[temps = %u] processus %s pid = %i\n", seconds(), get_active_name(),
           get_active_pid());
    scheduler_sleep(1);
  }
}

void proc2() {
  for (int i = 0; i < 10; i++) {
    printf("[temps = %u] processus %s pid = %i\n", seconds(), get_active_name(),
           get_active_pid());
    if (i == 1) {
      spawn_process(proc3, "bob3", HIGH);
    }
    scheduler_sleep(3);
  }
}

void console_reader() {
  for (;;) {
    char c;
    while (uart_read(&c) == -1) {
      scheduler_block_on(uart_get_wait_queue());
    }
    printf("%c", c);
  }
}

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

  spawn_process(console_reader, "console", NORMAL);
  spawn_process(proc1, "bob1", HIGH);
  spawn_process(proc2, "bob2", NORMAL);
  idle();
}
