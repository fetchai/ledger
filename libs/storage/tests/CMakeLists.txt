################################################################################
# F E T C H   S T O R A G E   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-storage)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(fetch_storage_todothis_tests fetch-storage file_object SLOW)
add_fetch_test(fetch_storage_tests fetch-storage tests SLOW)
add_fetch_test(fetch_selective_storage_tests fetch-storage selective_test)
