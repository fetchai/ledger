#
# F E T C H   J S O N    T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-json)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(json_gtest fetch-json json/gtest/)
