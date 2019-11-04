#
# F E T C H   M A T H   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-math)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-math)
target_link_libraries(fetch-math PUBLIC fetch-core fetch-logging)

add_test_target()

add_subdirectory(benchmark)
add_subdirectory(examples)
