################################################################################
# F E T C H   M L   B E N C H M A R K S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ml)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_executable(benchmark_layers layers/layers.cpp)
target_link_libraries(benchmark_layers fetch-ml)

