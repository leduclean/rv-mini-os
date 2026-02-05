#pragma once
#include <stddef.h>

void proc1();
void proc2();
void proc3();

typedef void (*fn_t)(void);

typedef struct {
  char *name;
  void (*fn)(void);
} prog_t;

/* Programs */
static prog_t prog_tab[] = {
    {"proc1", proc1}, {"proc2", proc2}, {"proc3", proc3}, {NULL, NULL}};
