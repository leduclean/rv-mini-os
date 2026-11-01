#include <stddef.h>
#include <stdint.h>

#include <lib/string.h>

#include <kernel/elf.h>
#include <kernel/ldsym.h>
#include <kernel/pages.h>
#include <kernel/vpages.h>

#define ELF_HDR_NIDENT 16 /**< Number of id in the elf header */

/**
 * @name The header identificator indexes
 * @{
 */
#define ELF_HDR_IDENT_MAG_NIDENT 4 /**< Number of magic file id. */
#define ELF_HDR_IDENT_CLASS 4 /**< File class. */
#define ELF_HDR_IDENT_DATA 5 /**< Data encoding. */
/** @} */

/**
 * @name Required ELF header values for our system.
 * @{
 */

#define ELF_MAGIC_STR \
	"\x7f"        \
	"ELF" /**< Magic string. */
#define ELF_HDR_IDENT_CLASS_64 2 /**< 64 bit architecture. */
#define ELF_HDR_IDENT_DATA_2LSB 1 /**< LSB (little-endian) encoding. */
#define ELF_HDR_TYPE_EXEC 2 /**< Executable elf file. */
#define ELF_PHDR_TYPE_LOAD 1 /**< Load program header type. */
#define ELF_HDR_MACHINE_RISCV 243 /**< RISCV elf machine. */
/** @} */

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

/**
 * @name Segments flags.
 * @{
 */
#define ELF_PHDR_FLAG_X 1 /**< Execute segment. */
#define ELF_PHDR_FLAG_W 2 /**< Write segment. */
#define ELF_PHDR_FLAG_R 4 /**< Read segment. */
/** @} */

/**
 * @class elf_phdr
 * @brief Elf program header defining segment attributes.
 */
struct elf_phdr {
	uint32_t type;
	uint32_t flags;
	uint64_t offset;
	uint64_t va;
	uint64_t pa;
	uint64_t filesz;
	uint64_t memsz;
	uint64_t align;
};

int elf_verify_file_format(const uintptr_t file)
{
	struct elf_hdr *hdr = (struct elf_hdr *)file;

	if (memcmp(hdr->ident, ELF_MAGIC_STR, ELF_HDR_IDENT_MAG_NIDENT) != 0) {
		return -1;
	}

	if (hdr->ident[ELF_HDR_IDENT_CLASS] != ELF_HDR_IDENT_CLASS_64) {
		return -1;
	}

	if (hdr->ident[ELF_HDR_IDENT_DATA] != ELF_HDR_IDENT_DATA_2LSB) {
		return -1;
	}

	if (hdr->type != ELF_HDR_TYPE_EXEC) {
		return -1;
	}

	if (hdr->machine != ELF_HDR_MACHINE_RISCV) {
		return -1;
	}

	return 0;
}

static int load_phdr(uintptr_t file, struct elf_phdr *phdr[])
{
	struct elf_hdr *hdr = (struct elf_hdr *)file;
	if (hdr->phdr_num == 0) {
		return -1;
	}
	if (hdr->phdr_size != sizeof(struct elf_phdr)) {
		return -1;
	}

	*phdr = (struct elf_phdr *)(file + (uintptr_t)hdr->phdroff);
	return hdr->phdr_num;
}

static inline bool is_load_phdr(const struct elf_phdr *phdr)
{
	return phdr->type == ELF_PHDR_TYPE_LOAD;
}

static inline bool is_valid_phdr(const struct elf_phdr *phdr)
{
	if (phdr->memsz < phdr->filesz) {
		return false;
	}

	if (phdr->va % PAGE_SIZE != 0) {
		return false;
	}

	if ((uintptr_t)(phdr->va + phdr->memsz) > (uintptr_t)_ram_start) {
		return false;
	}

	return true;
}

static unsigned long phdr_flag_to_pte_flag(uint32_t flags)
{
	unsigned long pte_flag = 0;
	if (flags & ELF_PHDR_FLAG_X) {
		pte_flag |= PTE_X;
	}

	if (flags & ELF_PHDR_FLAG_W) {
		pte_flag |= PTE_W;
	}

	if (flags & ELF_PHDR_FLAG_R) {
		pte_flag |= PTE_R;
	}

	return pte_flag;
}

static int map_phdr(pte_t *root, uintptr_t file, const struct elf_phdr *phdr)
{
	for (size_t off = 0; off < phdr->memsz; off += PAGE_SIZE) {

		void *page = page_alloc();
		if (!page) {
			return -1;
		}

		/* Copy the file content */
		if (off < phdr->filesz) {
			size_t n = phdr->filesz - off < PAGE_SIZE
					   ? phdr->filesz - off
					   : PAGE_SIZE;
			memcpy(page, (void *)(file + phdr->offset + off), n);
		}

		int res = vpage_map_user(root, (void *)phdr->va + off, page,
					 phdr_flag_to_pte_flag(phdr->flags));
		if (res < 0) {
			page_put(page);
			return res;
		}
	}

	return 0;
}

int elf_map(pte_t *root, const uintptr_t file)
{

	struct elf_phdr *phdr_tab;

	int nb_phdr = load_phdr(file, &phdr_tab);
	if (nb_phdr < 0) {
		return nb_phdr;
	}

	for (int i = 0; i < nb_phdr; i++) {
		if (!is_load_phdr(&phdr_tab[i])) {
			continue;
		}

		if (!is_valid_phdr(&phdr_tab[i])) {
			return -1;
		}

		int res = map_phdr(root, file, &phdr_tab[i]);
		if (res < 0) {
			return res;
		}
	}

	return 0;
}

uintptr_t elf_get_entry(const uintptr_t file)
{
	return ((struct elf_hdr *)file)->entry;
}
