#
# F E T C H   M L   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-ml)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-ml)

target_link_libraries(fetch-ml
                      PUBLIC fetch-core
                             fetch-crypto
                             fetch-math
                             fetch-dmlf)

# ------------------------------------------------------------------------------
# Example Targets
# ------------------------------------------------------------------------------

add_subdirectory(examples)

# ------------------------------------------------------------------------------
# Test Targets
# ------------------------------------------------------------------------------

add_test_target()

# ------------------------------------------------------------------------------
# Benchmark Targets
# ------------------------------------------------------------------------------

add_subdirectory(benchmark)
