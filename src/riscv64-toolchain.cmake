set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(CMAKE_C_COMPILER   riscv64-elf-gcc)
set(CMAKE_CXX_COMPILER riscv64-elf-g++)
set(CMAKE_ASM_COMPILER riscv64-elf-gcc)

set(CMAKE_C_FLAGS   "-std=c17 -march=rv64ima_zicsr -mabi=lp64 -mcmodel=medany -Wall -Wextra -O0 -ggdb3 -ffunction-sections -fdata-sections -ffreestanding")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
