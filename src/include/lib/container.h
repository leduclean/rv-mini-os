#pragma once
#include "minilib/stddef.h"

#define container_of(PTR, TYPE, MEMBER)                                        \
  ((TYPE *)((char *)(PTR) - offset_of(TYPE, MEMBER)))
