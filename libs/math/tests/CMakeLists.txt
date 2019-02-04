################################################################################
# F E T C H   M A T H   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-math)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(blas_gtest fetch-math math/linalg/blas/)

add_fetch_test(matrix_gtest fetch-math math/linalg/matrix)
target_link_libraries(matrix_gtest PRIVATE pthread)

add_fetch_test(ndarray_gtest fetch-math math/ndarray)
add_fetch_test(ndarray_view_gtest fetch-math math/ndarray_view)
add_fetch_test(ndarray_iterator_gtest fetch-math math/ndarray_iterator)
add_fetch_test(ndarray_broadcast_gtest fetch-math math/ndarray_broadcast)
add_fetch_test(ndarray_squeeze_gtest fetch-math math/ndarray_squeeze)
add_fetch_test(combinatorics_gtest fetch-math math/combinatorics)

add_fetch_test(free_functions_gtest fetch-math math/free_functions)

add_fetch_test(relu_gtest fetch-math math/kernels/relu)
add_fetch_test(sign_gtest fetch-math math/kernels/sign)

add_fetch_test(l2loss_gtest fetch-math math/shapeless_array/l2loss)
add_fetch_test(shapeless_array_basic_tests fetch-math math/shapeless_array/basic_tests)

add_fetch_test(big_number_gtest fetch-math math/basic_math_tests)
add_fetch_test(distances_gtest fetch-math math/distance/gtest)
add_fetch_test(clustering_gtest fetch-math math/clustering)

add_fetch_test(math_gtest fetch-math math/gtest SLOW)

add_fetch_test(tensor_gtest fetch-math math/tensor)
