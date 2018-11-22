################################################################################
# F E T C H   M A T H   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-math)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_gtest(blas_gtest fetch-math math/linalg/blas/)

add_fetch_gtest(matrix_gtest fetch-math math/linalg/matrix)
target_link_libraries(matrix_gtest PRIVATE pthread)

add_fetch_gtest(ndarray_gtest fetch-math math/ndarray)
add_fetch_gtest(ndarray_view_gtest fetch-math math/ndarray_view)
add_fetch_gtest(ndarray_iterator_gtest fetch-math math/ndarray_iterator)
add_fetch_gtest(ndarray_broadcast_gtest fetch-math math/ndarray_broadcast)
add_fetch_gtest(ndarray_squeeze_gtest fetch-math math/ndarray_squeeze)
add_fetch_gtest(combinatorics_gtest fetch-math math/combinatorics)

add_fetch_gtest(free_functions_gtest fetch-math math/free_functions)

add_fetch_gtest(relu_gtest fetch-math math/kernels/relu)
add_fetch_gtest(sign_gtest fetch-math math/kernels/sign)
add_fetch_gtest(l2loss_gtest fetch-math math/shape_less_array/l2loss)

add_fetch_gtest(big_number_gtest fetch-math math/basic_math_tests)
add_fetch_gtest(distances_gtest fetch-math math/distance/gtest)
add_fetch_gtest(clustering_gtest fetch-math math/clustering)

add_fetch_gtest(math_gtest fetch-math gtest)
