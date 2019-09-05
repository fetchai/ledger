#
# F E T C H   A D D R E S S   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-address)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-address)
target_link_libraries(fetch-address
                      PUBLIC fetch-core
                      fetch-crypto
                      fetch-logging)
# ------------------------------------------------------------------------------
# Example Targets
# ------------------------------------------------------------------------------

# add_subdirectory(examples)

# ------------------------------------------------------------------------------
# Test Targets
# ------------------------------------------------------------------------------

# add_test_target()

# ------------------------------------------------------------------------------
# Benchmark Targets
# ------------------------------------------------------------------------------

# add_subdirectory(benchmark)
