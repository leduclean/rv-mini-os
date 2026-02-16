set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(CMAKE_C_COMPILER   riscv64-unknown-elf-gcc)
set(CMAKE_CXX_COMPILER riscv64-unknown-elf-g++)
set(CMAKE_ASM_COMPILER riscv64-unknown-elf-gcc)

set(CMAKE_C_FLAGS   "-march=rv64ima_zicsr -mabi=lp64 -mcmodel=medany -Wall -Wextra -O0 -ggdb3 -ffunction-sections -fdata-sections")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -static -lgcc -Wl,--nmagic -Wl,--gc-sections -g")
