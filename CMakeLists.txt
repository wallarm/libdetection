cmake_minimum_required (VERSION 2.8.8 FATAL_ERROR)
cmake_policy (VERSION 2.8)

project (PoC-Detect C)

set (CMAKE_C_FLAGS "-Wall -std=gnu99 -O0 -g ${CMAKE_C_FLAGS}")

include(CTest)

add_subdirectory(lib)
add_subdirectory(perf)
