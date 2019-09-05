#
# F E T C H   C O R E   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-bazel-test)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-bazel-test)

# ------------------------------------------------------------------------------
# Example Targets
# ------------------------------------------------------------------------------

add_subdirectory(examples)

# ------------------------------------------------------------------------------
# Test Targets
# ------------------------------------------------------------------------------

# add_test_target()

# ------------------------------------------------------------------------------
# Benchmark Targets
# ------------------------------------------------------------------------------

# add_subdirectory(benchmark)