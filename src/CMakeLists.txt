cmake_minimum_required(VERSION 3.15)
project(OS C ASM)

add_subdirectory(lib)
add_subdirectory(user)
add_subdirectory(kernel/core)
add_subdirectory(kernel/init)
add_subdirectory(arch/riscv)

add_executable(kernel.bin
    $<TARGET_OBJECTS:arch_riscv>
    $<TARGET_OBJECTS:kernel_init>
    $<TARGET_OBJECTS:kernel_core>
    $<TARGET_OBJECTS:lib>
    $<TARGET_OBJECTS:user>
)

target_link_options(kernel.bin PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/arch/riscv/ld/kernel.lds
    -nostdlib -static -lgcc
    -Wl,--nmagic
    -Wl,--gc-sections
    -e entry -g
)

set(QEMU qemu-system-riscv64)
set(QEMU_BASE_OPTS -machine virt -m 128M -bios none)
set(QEMU_RUN_OPTS -nographic ${QEMU_BASE_OPTS})
set(QEMU_DEBUG_OPTS -gdb tcp::1234 -S ${QEMU_RUN_OPTS})
set(QEMU_RUN_GUI_OPTS -device bochs-display -serial mon:stdio -display default,show-cursor=on ${QEMU_BASE_OPTS})
set(QEMU_DEBUGG_OPTS -gdb tcp::1234 -S ${QEMU_RUN_GUI_OPTS})
set(QEMU_DEBUGT_OPTS -gdb tcp::1234 -S -d int,cpu -D qemu.log  ${QEMU_RUN_GUI_OPTS})

add_custom_target(run
    COMMAND ${QEMU} ${QEMU_RUN_OPTS} -kernel kernel.bin
    DEPENDS kernel.bin
)

add_custom_target(rung
    COMMAND ${QEMU} ${QEMU_RUN_GUI_OPTS} -kernel kernel.bin
    DEPENDS kernel.bin
)

add_custom_target(debug
    COMMAND ${QEMU} ${QEMU_DEBUG_OPTS} -kernel kernel.bin
    DEPENDS kernel.bin
)
add_custom_target(debugg
    COMMAND ${QEMU} ${QEMU_DEBUGG_OPTS} -kernel kernel.bin
    DEPENDS kernel.bin
)

add_custom_target(debugt
    COMMAND ${QEMU} ${QEMU_DEBUG_OPTS} -kernel kernel.bin
    DEPENDS kernel.bin
)
