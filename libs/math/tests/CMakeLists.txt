################################################################################
# F E T C H   M A T H   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-math)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_gtest(blas fetch-math math/linalg/blas/)



add_fetch_gtest(matrix_test fetch-math math/linalg/matrix)
target_link_libraries(matrix_test PRIVATE pthread)

add_fetch_gtest(ndarray_test fetch-math math/ndarray)
add_fetch_gtest(ndarray_view_test fetch-math math/ndarray_view)
add_fetch_gtest(ndarray_iterator_test fetch-math math/ndarray_iterator)
add_fetch_gtest(ndarray_broadcast_test fetch-math math/ndarray_broadcast)
add_fetch_gtest(ndarray_squeeze_test fetch-math math/ndarray_squeeze)

add_fetch_gtest(relu_test fetch-math math/kernels/relu)
add_fetch_gtest(sign_test fetch-math math/kernels/sign)
add_fetch_gtest(l2loss_test fetch-math math/shape_less_array/l2loss)

add_fetch_gtest(big_number_gtest fetch-math math/basic_math_tests)
add_fetch_gtest(manhattan_gtest fetch-math math/distance/gtest)


