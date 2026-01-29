#include "process.h"
#include "time.h"
#include <console.h>
#include <cpu.h>
#include <stdint.h>
#include <stdio.h>

extern void mon_traitant(void);

/* on peut s'entrainer a utiliser GDB avec ce code de base */
/* par exemple afficher les valeurs de n et res avec la commande display */
/* une fonction bien connue */
uint32_t fact(uint32_t n) {
  uint32_t res;
  if (n <= 1) {
    res = 1;
  } else {
    res = fact(n - 1) * n;
  }
  return res;
}

void kernel_start() {
  init_proc();
  idle();
}
