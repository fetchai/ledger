#
# F E T C H   M A T H   B E N C H M A R K S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-math)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_gbench(benchmark_math_activation_functions fetch-math activation_functions)
add_fetch_gbench(benchmark_math_basic fetch-math ../../math/benchmark/basic_math)
add_fetch_gbench(benchmark_math_tensor fetch-math tensor)
add_fetch_gbench(benchmark_math_matrix_ops fetch-math matrix_ops)
add_fetch_gbench(benchmark_math_trigonometry fetch-math trigonometry)
