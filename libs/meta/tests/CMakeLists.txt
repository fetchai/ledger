#
# F E T C H   M E T A   L I B R A R Y   T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-meta)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(fetch-meta-unit-tests fetch-meta unit/)
target_link_libraries(fetch-meta-unit-tests PRIVATE fetch-core)
