cmake_minimum_required(VERSION 3.10)
project(HW2)

set(CMAKE_C_STANDARD 11)

add_library(lib lib.c lib.h)
add_executable(test_run main.c)
target_link_libraries(test_run PRIVATE lib)