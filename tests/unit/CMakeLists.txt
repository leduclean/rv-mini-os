file(GLOB_RECURSE UNIT_SRC CONFIGURE_DEPENDS *.c)

set(OS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../src)
set(CLIST_SRC ${OS_ROOT}/lib/clist.c)

add_executable(
    unit
    ${UNIT_SRC}
    ${CLIST_SRC}
)
target_include_directories(
    unit
    PRIVATE
        ${OS_ROOT}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unity/
)

target_compile_definitions(unit PRIVATE UNITY_FIXTURE_NO_EXTRAS)
target_compile_options(
    unit
    PRIVATE
        -Wall
        -Wextra
)

add_test(NAME unit COMMAND unit)
