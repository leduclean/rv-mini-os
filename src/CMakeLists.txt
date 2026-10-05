cmake_minimum_required(VERSION 3.15)
project(OS C ASM)

add_subdirectory(lib)
add_subdirectory(user)
add_subdirectory(kernel/core)
add_subdirectory(kernel/drivers)
add_subdirectory(kernel/init)
add_subdirectory(arch/riscv)

add_executable(kernel.bin
    $<TARGET_OBJECTS:arch_riscv>
    $<TARGET_OBJECTS:kernel_init>
    $<TARGET_OBJECTS:kernel_core>
    $<TARGET_OBJECTS:kernel_drivers>
    $<TARGET_OBJECTS:lib>
    $<TARGET_OBJECTS:user>
)

set_target_properties(kernel.bin PROPERTIES
    LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/arch/riscv/ld/kernel.lds)

target_link_options(kernel.bin PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/arch/riscv/ld/kernel.lds
    -nostdlib -static
    -Wl,--nmagic
    -Wl,--gc-sections
    -e entry -g
)


set(QEMU qemu-system-riscv64)

# Create Disk Image
set(DISK_IMG ${CMAKE_BINARY_DIR}/disk.raw)
add_custom_command(
  OUTPUT ${DISK_IMG}
  COMMAND qemu-img create -f raw ${DISK_IMG} 8M
)
set(QEMU_DISK_OPTS
  -drive file=${DISK_IMG},if=none,format=raw,id=disk0
  -device virtio-blk-pci,drive=disk0,id=vblk0,disable-legacy=on
)

set(QEMU_BASE_OPTS
  -machine virt
  -m 128M
  -bios none
  -no-reboot
  ${QEMU_DISK_OPTS}
)

set(QEMU_RUN_OPTS
    -nographic ${QEMU_BASE_OPTS})

set(QEMU_DEBUG_OPTS
  -d guest_errors,unimp
  -s
  -S ${QEMU_RUN_OPTS}
)

set(QEMU_RUN_GUI_OPTS
    -device bochs-display
    -serial mon:stdio
    -display default,show-cursor=on ${QEMU_BASE_OPTS}
)
set(QEMU_DEBUGG_OPTS
  -s
  -S ${QEMU_RUN_GUI_OPTS}
)
set(QEMU_DEBUGT_OPTS
  -s
  -S -d int,cpu
  -D qemu.log
  ${QEMU_RUN_GUI_OPTS}
)

add_custom_target(run
    COMMAND ${QEMU} ${QEMU_RUN_OPTS} -kernel kernel.bin
    DEPENDS kernel.bin ${DISK_IMG}
    USES_TERMINAL
)

add_custom_target(rung
    COMMAND ${QEMU} ${QEMU_RUN_GUI_OPTS} -kernel kernel.bin
    DEPENDS kernel.bin ${DISK_IMG}
    USES_TERMINAL
)

add_custom_target(debug
    COMMAND ${QEMU} ${QEMU_DEBUG_OPTS} -kernel kernel.bin
    DEPENDS kernel.bin ${DISK_IMG}
    USES_TERMINAL
)
add_custom_target(debugg
    COMMAND ${QEMU} ${QEMU_DEBUGG_OPTS} -kernel kernel.bin
    DEPENDS kernel.bin ${DISK_IMG}
    USES_TERMINAL
)

add_custom_target(debugt
    COMMAND ${QEMU} ${QEMU_DEBUGT_OPTS} -kernel kernel.bin
    DEPENDS kernel.bin ${DISK_IMG}
    USES_TERMINAL
)
