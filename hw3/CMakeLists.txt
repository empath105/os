cmake_minimum_required(VERSION 3.10)
project(HW3)

set(CMAKE_C_STANDARD 11)

add_library(audit_libs STATIC
    proclib.c proclib.h
    loglib.c loglib.h
    shmlib.c shmlib.h
)
add_executable(test_run main.c)
target_link_libraries(test_run PRIVATE audit_libs)