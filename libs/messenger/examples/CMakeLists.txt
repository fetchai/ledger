#
# F E T C H   M E S S E N G E R   E X A M P L E S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-messenger)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

setup_library_examples(fetch-messenger)
