cmake_minimum_required (VERSION 2.8.8 FATAL_ERROR)
cmake_policy (VERSION 2.8)

project (PoC-Detect C)

set (CMAKE_C_FLAGS "-Wall -std=gnu99 -O2 -fno-omit-frame-pointer -g ${CMAKE_C_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

include(CTest)
include(GNUInstallDirs)

if (NOT ENABLE_STATIC AND NOT ENABLE_SHARED)
  set (ENABLE_SHARED ON)
endif()

add_subdirectory(lib)
add_subdirectory(perf)
