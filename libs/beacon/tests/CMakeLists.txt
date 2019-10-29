#
# F E T C H   B E A C O N   T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-beacon)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_slow_test(beacon-unit-tests fetch-beacon unit/)
