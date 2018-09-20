################################################################################
# F E T C H   H T T P   E X A M P L E S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-http)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

setup_library_examples(fetch-http)

