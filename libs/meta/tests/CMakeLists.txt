#
# F E T C H   M E T A   T E S T S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-meta)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(pack_gtest fetch-meta meta/pack)
fetch_add_test(type_util_gtest fetch-meta meta/type_util)

target_include_directories(pack_gtest PRIVATE meta)
