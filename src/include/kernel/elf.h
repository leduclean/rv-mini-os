#pragma once

#include <stddef.h>
#include <stdint.h>

#include <kernel/vpages.h>

/**
 * @brief Verify the header of the elf file.
 *
 * @param file Pointer to the file.
 * @return 0 on success, error code < 0 on failure.
 */
int elf_verify_file_format(const uintptr_t file);

/**
 * @brief Map a program header segment in the page table.
 *
 * @param root The root of the page table tree.
 * @param file The elf file memory location.
 * @return 0 on successfull computation, error code < 0 on failure.
 */
int elf_map(pte_t *root, const uintptr_t file);

/**
 * @brief Gets the entry point of the elf file.
 *
 * @param file The elf file.
 * @return The entry point addr of the elf file.
 */
uintptr_t elf_get_entry(const uintptr_t file);
