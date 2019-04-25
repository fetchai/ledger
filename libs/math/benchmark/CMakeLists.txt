################################################################################
# F E T C H   M A T H   B E N C H M A R K S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-math)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_gbench(benchmark_basic_math fetch-math basic_math)
add_fetch_gbench(benchmark_tensor fetch-math tensor)
add_fetch_gbench(benchmark_matrix_ops fetch-math matrix_ops)
