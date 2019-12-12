#
# F E T C H   M U D D L E   T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-muddle)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# define the test target
fetch_add_slow_test(muddle-unit-tests fetch-muddle unit/)
fetch_add_integration_test(muddle-integration-tests fetch-muddle integration/)
