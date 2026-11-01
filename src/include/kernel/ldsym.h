/**
 * @file
 * @brief This file define all the linker defined symbols.
 */

#pragma once

extern char _ram_start[], _ram_end[], _kernel_end[];
extern char _trampoline_start[], _trampoline_end[];
extern char _bss_start[], _bss_end[];
extern char _kheap_start[], _kheap_end[];
extern char _page_start[], _page_end[];
