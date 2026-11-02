cmake_minimum_required(VERSION 3.21)
project(Test C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(unit)
