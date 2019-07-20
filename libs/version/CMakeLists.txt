#
# F E T C H   V E R S I O N   L I B R A R Y
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-version)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

setup_compiler()

generate_version_file()

setup_library(fetch-version)

# Add generated file to source set
target_sources(fetch-version PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src/fetch_version.cpp)
