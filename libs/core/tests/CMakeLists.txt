#
# F E T C H   C O R E   T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-core)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(core-unit-tests fetch-core unit/)
fetch_add_integration_test(core-integration-tests fetch-core integration/)
