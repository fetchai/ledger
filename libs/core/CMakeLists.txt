################################################################################
# F E T C H   C O R E   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-core)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

setup_library(fetch-core)
target_link_libraries(fetch-core PUBLIC fetch-vectorise fetch-variant vendor-spdlog)

add_test_target()

add_subdirectory(benchmark)
add_subdirectory(examples)
