add_library(kernel_init OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/init.c")

target_include_directories(kernel_init PUBLIC
    ${CMAKE_SOURCE_DIR}/include/kernel/init
    ${CMAKE_SOURCE_DIR}/include/lib
    ${CMAKE_SOURCE_DIR}/include/user
    $<TARGET_PROPERTY:arch_riscv,INTERFACE_INCLUDE_DIRECTORIES> # Get arch riscv includes
)
