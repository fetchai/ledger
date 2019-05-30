#
# F E T C H   T I M E   T E S T S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-moment)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# define the test target
fetch_add_test(fetch-moment-unit-tests fetch-moment unit/)
# fetch_add_integration_test(fetch-moment-integration-tests fetch-moment integration/)
