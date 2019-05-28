#
# F E T C H   V M   M O D U L E S   E X A M P L E S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-vm-modules)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

setup_library_examples(fetch-vm-modules)
