file(GLOB_RECURSE USER_SRCS *.c *.S)
add_library(user OBJECT ${USER_SRCS})

target_include_directories(user PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

