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
  init_uart();
  if (init_ecran() != 0) {
    printf("Screen init FAILED\n will only display in UART");
  };

  enable_it();
  init_traitant(mon_traitant);
  enable_timer();
  printf("Truc\nSalut\n1\t2\t3\nAB\bC\nBEEF\rRABBIT");
  for (int i = 0; i < 90; i++) {
    printf("%d\n", i);
  }
  printf("Hello\n");

  // /* on ne doit jamais sortir de kernel_start */
  while (1) {
    /* cette fonction arrete le processeur */
    hlt();
  }
}
