#pragma once

#include <stddef.h>
#include <stdint.h>

#include <kernel/file.h>
#include <kernel/vpages.h>

#define ELF_HDR_NIDENT 16 /**< Number of id in the elf header */

/**
 * @class elf_hdr
 * @brief Elf header defining file attributes.
 */
struct elf_hdr {
	unsigned char ident[ELF_HDR_NIDENT];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint64_t entry;
	uint64_t phdroff;
	uint64_t _shdroff;
	uint32_t _flags;
	uint16_t size;
	uint16_t phdr_size;
	uint16_t phdr_num;
	/* Non exhaustive: section header fields are not needed for now. */
};

int elf_load_hdr(struct file *f, struct elf_hdr *hdr);

/**
 * @brief Verify the header of the elf file.
 *
 * @param f Pointer to the file.
 * @retval 0 Success.
 * @retval -ENOEXEC Not a riscv64 little-endian executable.
 */
int elf_verify_file_format(const struct elf_hdr *hdr);

/**
 * @brief Map a program header segment in the page table.
 *
 * @param root The root of the page table tree.
 * @param f The elf file memory location.
 * @retval 0 Success.
 * @retval -ENOEXEC Bad program header.
 * @retval -ENOMEM Out of memory.
 */

int elf_map(pte_t *root, struct file *f, const struct elf_hdr *hdr);
