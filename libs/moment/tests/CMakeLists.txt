################################################################################
# F E T C H   T I M E   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-moment)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# define the test target
add_fetch_test(fetch-moment-unit-tests fetch-moment unit/)
#add_fetch_test(fetch-moment-integration-tests fetch-moment integration/)
