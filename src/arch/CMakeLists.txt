set(CRT0_SRC ${CMAKE_CURRENT_SOURCE_DIR}/crt0.S)

file(GLOB OTHER_ASM_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.S)
list(REMOVE_ITEM OTHER_ASM_SRCS ${CRT0_SRC})

file(GLOB C_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(
    arch_riscv
    OBJECT
    ${CRT0_SRC} # crt0 in first
    ${OTHER_ASM_SRCS}
    ${C_SRCS}
)

target_include_directories(
    arch_riscv
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(
    arch_riscv
    PROPERTIES
        POSITION_INDEPENDENT_CODE
            OFF
)
