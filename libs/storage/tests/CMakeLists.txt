#
# F E T C H   S T O R A G E   T E S T S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-storage)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_slow_test(storage_gtest
               fetch-storage
               gtest)

fetch_add_slow_test(selective_storage_gtest
               fetch-storage
               selective_test)
