cmake_minimum_required(VERSION 3.15)
project(
    OS
    C
    ASM
)

add_subdirectory(lib)
add_subdirectory(user)
add_subdirectory(kernel/core)
add_subdirectory(kernel/drivers)
add_subdirectory(kernel/init)
add_subdirectory(arch)

add_executable(
    kernel.bin
    $<TARGET_OBJECTS:arch_riscv>
    $<TARGET_OBJECTS:kernel_init>
    $<TARGET_OBJECTS:kernel_core>
    $<TARGET_OBJECTS:kernel_drivers>
    $<TARGET_OBJECTS:lib>
)

set(KERNEL_LINK_FILE ${CMAKE_CURRENT_SOURCE_DIR}/arch/ld/kernel.ld)
set_target_properties(
    kernel.bin
    PROPERTIES
        LINK_DEPENDS
            ${KERNEL_LINK_FILE}
)

target_link_options(
    kernel.bin
    PRIVATE
        -T
        ${KERNEL_LINK_FILE}
        -nostdlib
        -static
        -Wl,--nmagic
        -Wl,--gc-sections
        -e
        entry
        -g
)

set(USER_LINK_FILE ${CMAKE_CURRENT_SOURCE_DIR}/user/user.ld)
add_executable(user.elf $<TARGET_OBJECTS:user>)
set_target_properties(
    user.elf
    PROPERTIES
        LINK_DEPENDS
            ${USER_LINK_FILE}
)

target_link_options(
    user.elf
    PRIVATE
        -T${USER_LINK_FILE}
        -nostdlib
        -static
)
target_link_libraries(user.elf ulib)

set(DISK_SIZE_M 8)
set(MKFS ${CMAKE_BINARY_DIR}/mkfs)
add_custom_command(
    OUTPUT
        ${MKFS}
    COMMAND
        cc -std=c23 -I${CMAKE_SOURCE_DIR}/include -DDISK_SIZE_M=${DISK_SIZE_M}
        -o ${MKFS} ${CMAKE_SOURCE_DIR}/tools/mkfs.c
    DEPENDS
        ${CMAKE_SOURCE_DIR}/tools/mkfs.c
        ${CMAKE_SOURCE_DIR}/include/kernel/fs_layout.h
)

# Create Disk Image
set(DISK_IMG ${CMAKE_BINARY_DIR}/disk.raw)
add_custom_command(
    OUTPUT
        ${DISK_IMG}
    COMMAND
        qemu-img create -f raw ${DISK_IMG} ${DISK_SIZE_M}M
    COMMAND
        ${MKFS} ${DISK_IMG}
    DEPENDS
        ${MKFS}
)

set(QEMU_DISK_OPTS
    -drive
    file=${DISK_IMG},if=none,format=raw,id=disk0
    -device
    virtio-blk-pci,drive=disk0,id=vblk0,disable-legacy=on,addr=1
)

set(QEMU_BASE_OPTS
    -machine
    virt
    -m
    128M
    -bios
    none
    -no-reboot
    ${QEMU_DISK_OPTS}
)

set(QEMU_RUN_OPTS
    -nographic
    ${QEMU_BASE_OPTS}
)

set(QEMU_DEBUG_OPTS
    -d
    guest_errors,unimp
    -s
    -S
    ${QEMU_RUN_OPTS}
)

set(QEMU_RUN_GUI_OPTS
    -device
    bochs-display,addr=2
    -serial
    mon:stdio
    -display
    default,show-cursor=on
    ${QEMU_BASE_OPTS}
)
set(QEMU_DEBUGG_OPTS
    -s
    -S
    ${QEMU_RUN_GUI_OPTS}
)
set(QEMU_DEBUGT_OPTS
    -s
    -S
    -d
    int,cpu
    -D
    qemu.log
    ${QEMU_RUN_GUI_OPTS}
)

set(QEMU qemu-system-riscv64)

add_custom_target(
    run
    COMMAND
        ${QEMU} ${QEMU_RUN_OPTS} -kernel kernel.bin
    DEPENDS
        kernel.bin
        ${DISK_IMG}
    USES_TERMINAL
)

add_custom_target(
    rung
    COMMAND
        ${QEMU} ${QEMU_RUN_GUI_OPTS} -kernel kernel.bin
    DEPENDS
        kernel.bin
        ${DISK_IMG}
    USES_TERMINAL
)

add_custom_target(
    debug
    COMMAND
        ${QEMU} ${QEMU_DEBUG_OPTS} -kernel kernel.bin
    DEPENDS
        kernel.bin
        ${DISK_IMG}
    USES_TERMINAL
)
add_custom_target(
    debugg
    COMMAND
        ${QEMU} ${QEMU_DEBUGG_OPTS} -kernel kernel.bin
    DEPENDS
        kernel.bin
        ${DISK_IMG}
    USES_TERMINAL
)

add_custom_target(
    debugt
    COMMAND
        ${QEMU} ${QEMU_DEBUGT_OPTS} -kernel kernel.bin
    DEPENDS
        kernel.bin
        ${DISK_IMG}
    USES_TERMINAL
)

set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../tests)
add_custom_target(
    integration
    COMMAND
        ${CMAKE_COMMAND} -E env
        "OS_QEMU_CMD=${QEMU};${QEMU_RUN_OPTS};-kernel;$<TARGET_FILE:kernel.bin>"
        uv run --project ${TESTS_DIR} pytest ${TESTS_DIR}/integration
    DEPENDS
        kernel.bin
        ${DISK_IMG}
    USES_TERMINAL
    VERBATIM
)
