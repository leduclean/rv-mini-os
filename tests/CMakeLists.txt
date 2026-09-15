cmake_minimum_required(VERSION 3.15)
project(UnitTests C)

set(OS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../src)

set(SRC_CLIST ${OS_ROOT}/lib/clist.c)

set(UNITY_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/unity/unity_fixture.c
    ${CMAKE_CURRENT_SOURCE_DIR}/unity/unity.c
)

set(SRC_MOCKS
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/kernel_mocks.c
)

file(GLOB_RECURSE TEST_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_*.c
)

add_executable(tests
    ${SRC_CLIST}
    ${UNITY_SRC}
    ${TEST_SRC}
)

target_include_directories(tests PRIVATE
    ${OS_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(tests PRIVATE UNIT_TEST UNITY_FIXTURE_NO_EXTRAS)
target_compile_options(tests PRIVATE -O0 -g -Wall -Wextra -std=c11)
