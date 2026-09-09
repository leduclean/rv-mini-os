set(CRT0_SRC ${CMAKE_CURRENT_SOURCE_DIR}/crt0.S)

file(GLOB OTHER_ASM_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.S
)
list(REMOVE_ITEM OTHER_ASM_SRCS ${CRT0_SRC})

file(GLOB C_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

add_library(arch_riscv OBJECT
    ${CRT0_SRC}      # crt0 en premier
    ${OTHER_ASM_SRCS}
    ${C_SRCS}
)

target_include_directories(arch_riscv PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include  # RISCV headers
    ${CMAKE_SOURCE_DIR}/include/lib
    ${CMAKE_SOURCE_DIR}/include/kernel
    ${CMAKE_SOURCE_DIR}/include/user
)

set_target_properties(arch_riscv PROPERTIES
    POSITION_INDEPENDENT_CODE OFF
)
