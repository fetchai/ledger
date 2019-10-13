#
# F E T C H   V M   M O D U L E S   T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-vm-modules)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(vm-modules-unit-tests fetch-vm-modules unit/)
