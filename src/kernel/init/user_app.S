.section .rodata, "a",@progbits
.global _user_elf_start, _user_elf_end

_user_elf_start:
    .incbin ELF_PATH // Defined by the compiler preprocessor with cmake
_user_elf_end:
