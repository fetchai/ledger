#
# F E T C H   J S O N    T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-oef-base)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(oef_base_gtest fetch-oef-base utils/)
# fetch_add_test(yaml_gtest fetch-core yaml/gtest/)
