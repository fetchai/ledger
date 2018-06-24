################################################################################
# F E T C H   L E D G E R   P R O J  E C T
################################################################################
cmake_minimum_required (VERSION 3.1)
project (fetch)

#-------------------------------------------------------------------------------
# Configuration Options
#-------------------------------------------------------------------------------

option(FETCH_ENABLE_TESTS           "Enable test targets"       ON)
option(FETCH_ENABLE_EXAMPLES        "Enable example targets"    ON)
option(FETCH_ENABLE_BENCHMARKS      "Enable benchmark targets"  ON)
option(FETCH_ENABLE_PYTHON_BINDINGS "Build Python examples"     OFF)
option(FETCH_WARNINGS_AS_ERRORS     "Enable warnings as errors" ON)

# target architecture (to be replaced with automatic detection)
## sysctl -a | grep machdep.cpu.features
option(FETCH_ARCH_SSE3  "Architecture maximally supports SSE3"    OFF)
option(FETCH_ARCH_SSE42 "Architecture maximally supports SSE4.2"  ON)
option(FETCH_ARCH_AVX   "Architecture maximally supports AVX"     OFF)
option(FETCH_ARCH_FMA   "Architecture maximally supports FMA"     OFF)
option(FETCH_ARCH_AVX2  "Architecture maximally supports AVX2"    OFF)

#
#-------------------------------------------------------------------------------
# Top Level Setup
#-------------------------------------------------------------------------------

set (FETCH_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (FETCH_ROOT_CMAKE_DIR ${FETCH_ROOT_DIR}/cmake)
set (FETCH_ROOT_VENDOR_DIR ${FETCH_ROOT_DIR}/vendor)

# Default to a release build
if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "Defaulting to Release build")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "The build configuration")
endif (NOT CMAKE_BUILD_TYPE)

# Global CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)
include(${FETCH_ROOT_CMAKE_DIR}/BuildTargets.cmake)

if (FETCH_ENABLE_TESTS)
  enable_testing()
endif (FETCH_ENABLE_TESTS)


#
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/vendor/asio/asio/include")
#include_directories("${OPENSSL_INCLUDE_DIR}")
#
#add_definitions(-DASIO_STANDALONE -DASIO_HEADER_ONLY -DASIO_HAS_STD_SYSTEM_ERROR)
#
#enable_testing()
##exec_program(
##    "git"
##    ${CMAKE_CURRENT_SOURCE_DIR}
##    ARGS "describe"
##    OUTPUT_VARIABLE ${GIT_DESCRIBE})
#
#
#add_definitions( --std=c++11 )
#
#if(BUILD_MACH_SSE3)
#add_definitions( -msse3 )
#endif()
#
#if(BUILD_MACH_SSE42)
#add_definitions( -msse4.2 )
#endif()
#
#if(BUILD_MACH_AVX)
#add_definitions( -mavx )
#endif()
#
#if(BUILD_MACH_AVX2)
#add_definitions( -mavx2 )
#endif()
#
#if(BUILD_MACH_FMA)
#add_definitions( -mfma )
#endif()
#
#include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/RemoveFlag.cmake")
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
#
#include_directories(${PNG_INCLUDE_DIR})
#
#if(BUILD_PYTHON_BINDINGS)
#add_subdirectory( libfetchcore/ )
#endif()
#




#-------------------------------------------------------------------------------
# System Dependencies
#-------------------------------------------------------------------------------

find_package(OpenSSL REQUIRED)

#find_package(PNG REQUIRED)
#set(THREADS_PREFER_PTHREAD_FLAG ON)
#find_package(Threads REQUIRED)

message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")

#-------------------------------------------------------------------------------
# Vendor Targets
#-------------------------------------------------------------------------------

add_library(vendor-openssl INTERFACE)
target_link_libraries(vendor-openssl INTERFACE ${OPENSSL_LIBRARIES})
target_include_directories(vendor-openssl INTERFACE ${OPENSSL_INCLUDE_DIR})

configure_vendor_targets()

#-------------------------------------------------------------------------------
# Project Targets
#-------------------------------------------------------------------------------

configure_library_targets()

add_subdirectory(apps)
