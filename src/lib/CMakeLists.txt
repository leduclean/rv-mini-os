file(GLOB LIB_SRCS CONFIGURE_DEPENDS *.c)
add_library(lib OBJECT ${LIB_SRCS})

target_include_directories(lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
