#
# F E T C H   B L O O M   F I L T E R   T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-bloom-filter)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(fetch-bloom-filter-unit-tests fetch-bloom-filter unit/)
