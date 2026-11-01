file(GLOB_RECURSE DRIVER_SRCS CONFIGURE_DEPENDS *.c)
add_library(kernel_drivers OBJECT ${DRIVER_SRCS})
target_include_directories(
    kernel_drivers
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)
