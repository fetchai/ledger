################################################################################
# F E T C H   M I N E R   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-miner)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# define the test target
add_fetch_test(fetch-miner-tests fetch-miner .)
