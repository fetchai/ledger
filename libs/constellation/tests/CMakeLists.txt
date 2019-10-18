#
# F E T C H   C O N S T E L L A T I O N   T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-constellation)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_slow_test(ledger-multiple-constellation-test fetch-constellation multiple_constellation)
