file(GLOB_RECURSE DRIVER_SRCS *.c)
add_library(kernel_drivers OBJECT ${DRIVER_SRCS})
target_include_directories(kernel_drivers PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    $<TARGET_PROPERTY:arch_riscv,INTERFACE_INCLUDE_DIRECTORIES> # Get arch riscv includes
)
