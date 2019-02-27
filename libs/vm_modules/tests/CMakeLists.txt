################################################################################
# F E T C H   V M   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ledger)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(vm_modules_gtest fetch-vm_modules ./)
