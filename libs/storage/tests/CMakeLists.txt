#
# F E T C H   S T O R A G E   T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-storage)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_slow_test(storage-unit-tests fetch-storage unit/)
