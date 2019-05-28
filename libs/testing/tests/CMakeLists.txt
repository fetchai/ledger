#
# F E T C H   T E S T I N G   T E S T S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-testing)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(fetch-testing-tests fetch-testing .)
