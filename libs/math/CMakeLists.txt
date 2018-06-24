################################################################################
# F E T C H   M A T H   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1)
project(fetch-math)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-math)
target_link_libraries(fetch-math INTERFACE fetch-core)

#-------------------------------------------------------------------------------
# Test Targets
#-------------------------------------------------------------------------------

add_fetch_test(exp_test fetch-math tests/math/test_exp.cpp)
add_fetch_test(spline_test fetch-math tests/math/test_spline.cpp)
add_fetch_test(rectangular_load_save_test fetch-math tests/math/rectangular_load_save_test.cpp)
add_fetch_test(shared_rectangular_array_test fetch-math tests/math/shared_rectangular_array_test.cpp)
add_fetch_test(matrix_test fetch-math tests/math/linalg/matrix.cpp)
add_fetch_test(hamming_test fetch-math tests/math/distance/hamming.cpp)
add_fetch_test(manhattan_test fetch-math tests/math/distance/manhattan.cpp)
add_fetch_test(correlation_test fetch-math tests/math/distance/correlation.cpp)
add_fetch_test(distance_matrix_test fetch-math tests/math/distance/distance_matrix.cpp)

# disabled / not working tests
add_fetch_test(ndarray_test fetch-math tests/math/ndarray.cpp DISABLED)
add_fetch_test(load_image_read_exceptions_test fetch-math tests/image/load_image_read_exceptions.cpp DISABLED)

#-------------------------------------------------------------------------------
# Benchmark Targets
#-------------------------------------------------------------------------------
message(STATUS "Warning: Benchmark: benchmark_linalg_matrix does not compile")
#add_executable(benchmark_linalg_matrix benchmark/linalg/matrix.cpp)
#target_link_libraries(benchmark_linalg_matrix PRIVATE fetch-math)
