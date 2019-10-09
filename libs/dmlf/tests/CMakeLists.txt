#
# F E T C H   D M L F   T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-dmlf)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(dmlf_local_gtest fetch-dmlf local)
fetch_add_test(dmlf_muddle_gtest fetch-dmlf muddle)
fetch_add_test(dmlf_vm_wrapper_gtest fetch-dmlf vm_wrapper)
