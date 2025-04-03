cmake_minimum_required(VERSION 3.20)
project(memory-allocator C)

set(CMAKE_C_STANDARD 11)

include(CTest)
add_executable(memory-allocator src/main.c src/alloc.c)
