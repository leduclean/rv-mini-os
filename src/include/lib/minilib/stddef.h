#include "types.h"

#undef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#define offset_of(type, member) ((size_t)&(((type *)0)->member))
#endif
