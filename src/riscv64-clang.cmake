set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(RISCV_TARGET "riscv64-unknown-elf")

set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)

set(CMAKE_C_COMPILER_TARGET   ${RISCV_TARGET})
set(CMAKE_CXX_COMPILER_TARGET ${RISCV_TARGET})
set(CMAKE_ASM_COMPILER_TARGET ${RISCV_TARGET})

set(CMAKE_C_FLAGS   "-std=c17 -march=rv64ima_zicsr -mabi=lp64 -mcmodel=medany -Wall -Wextra -O0 -ggdb3 -ffunction-sections -fdata-sections -ffreestanding")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")


set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
