#
# F E T C H   H T T P   T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-http-tests)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Test Targets
# ------------------------------------------------------------------------------

fetch_add_slow_test(http-unit-tests fetch-http unit/)
