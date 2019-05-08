################################################################################
# F E T C H   H T T P   L I B R A R Y  T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-http-tests)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Test Targets
#-------------------------------------------------------------------------------
add_fetch_test(fetch-http-tests fetch-http .)
