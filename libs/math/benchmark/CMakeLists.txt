################################################################################
# F E T C H   M A T H   B E N C H M A R K S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-math)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_gbench(benchmark_shapeless_array fetch-math shapeless_array)
add_fetch_gbench(benchmark_basic_math fetch-math basic_math)
add_fetch_gbench(benchmark_tensor fetch-math tensor)

if(TRUE)
  fetch_warning("Disabled Bencmark: benchmark_linalg_matrix (does not compile)")
else()
  add_executable(benchmark_linalg_matrix benchmark/linalg/matrix.cpp)
  target_link_libraries(benchmark_linalg_matrix PRIVATE fetch-math)
endif()
