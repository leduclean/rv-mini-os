add_library(
    kernel_init
    OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/init.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/user_app.S"
)

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/user_app.S"
    PROPERTIES
        COMPILE_DEFINITIONS
            ELF_PATH="${CMAKE_BINARY_DIR}/user.elf"
        OBJECT_DEPENDS
            ${CMAKE_BINARY_DIR}/user.elf
)

target_include_directories(
    kernel_init
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)
