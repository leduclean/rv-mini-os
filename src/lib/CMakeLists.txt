file(GLOB LIB_SRCS CONFIGURE_DEPENDS *.c)
add_library(lib OBJECT ${LIB_SRCS})
target_include_directories(lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Expose the same lib to the user.
set(ULIB_SRC
    doprnt.c
    printf.c
    sprintf.c
    stdfct.c
    string.c
    strtoul.c
)
add_library(ulib STATIC ${ULIB_SRC})
target_include_directories(ulib PRIVATE ${CMAKE_SOURCE_DIR}/include)
