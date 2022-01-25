cmake_minimum_required (VERSION 2.8.8 FATAL_ERROR)
cmake_policy (VERSION 2.8)

project (PoC-Detect C)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set (CMAKE_C_FLAGS "-Wall -std=gnu99 -fno-omit-frame-pointer ${CMAKE_C_FLAGS}")

set(CMAKE_C_FLAGS_DEBUG          "-O0 -g")
set(CMAKE_C_FLAGS_MINSIZEREL     "-O2")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")
set(CMAKE_C_FLAGS_RELEASE        "-O2 -g")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

include(CTest)
include(GNUInstallDirs)

if (NOT ENABLE_STATIC AND NOT ENABLE_SHARED)
  set (ENABLE_SHARED ON)
endif()

add_subdirectory(lib)
add_subdirectory(perf)
