################################################################################
# F E T C H   L E D G E R   P R O J  E C T
################################################################################
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project (fetch)

#-------------------------------------------------------------------------------
# Configuration Options
#-------------------------------------------------------------------------------

option(FETCH_ENABLE_TESTS           "Enable test targets"         ON)
option(FETCH_ENABLE_EXAMPLES        "Enable example targets"      ON)
option(FETCH_ENABLE_BENCHMARKS      "Enable benchmark targets"    ON)
option(FETCH_ENABLE_PYTHON_BINDINGS "Build Python examples"       ON)
option(FETCH_WARNINGS_AS_ERRORS     "Enable warnings as errors"   ON)
option(FETCH_VERBOSE_CMAKE          "Enable verbose cmake output" OFF)

# target architecture (to be replaced with automatic detection)
## sysctl -a | grep machdep.cpu.features
option(FETCH_ARCH_SSE3  "Architecture maximally supports SSE3"    OFF)
option(FETCH_ARCH_SSE42 "Architecture maximally supports SSE4.2"  ON)
option(FETCH_ARCH_AVX   "Architecture maximally supports AVX"     OFF)
option(FETCH_ARCH_FMA   "Architecture maximally supports FMA"     OFF)
option(FETCH_ARCH_AVX2  "Architecture maximally supports AVX2"    OFF)

#-------------------------------------------------------------------------------
# Top Level Setup
#-------------------------------------------------------------------------------

set (FETCH_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (FETCH_ROOT_CMAKE_DIR ${FETCH_ROOT_DIR}/cmake)
set (FETCH_ROOT_VENDOR_DIR ${FETCH_ROOT_DIR}/vendor)

# Default to a release build
if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "Defaulting to Release build")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "The build configuration" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# Global CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)
include(${FETCH_ROOT_CMAKE_DIR}/BuildTargets.cmake)

# Enable CTest support
if (FETCH_ENABLE_TESTS)
  enable_testing()
endif (FETCH_ENABLE_TESTS)

#-------------------------------------------------------------------------------
# System / Vendor Targets
#-------------------------------------------------------------------------------

configure_vendor_targets()

#-------------------------------------------------------------------------------
# Project Targets
#-------------------------------------------------------------------------------

configure_library_targets()

add_subdirectory(apps)
