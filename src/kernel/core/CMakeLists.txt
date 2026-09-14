file(GLOB_RECURSE LIB_SRCS *.c)
add_library(kernel_core OBJECT ${LIB_SRCS})

target_include_directories(kernel_core PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/arch/riscv/include  # <asm/...>
)
