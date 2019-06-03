#
# F E T C H   C R Y P T O   E X A M P L E S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-crypto)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

setup_library_examples(fetch-crypto)
