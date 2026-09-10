/**
 * @file
 * @brief This file define all the linker defined symbols.
 */

#pragma once

extern char _ram_start[], _etext[], _edata[], _end[];
extern char _user_start[], _user_end[];
extern char _trampoline_start[], _trampoline_end[];
extern char __bss_start[], __bss_end[];
extern char _heap_start[], _heap_end[];
