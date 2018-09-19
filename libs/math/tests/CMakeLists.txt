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

add_fetch_test(exp_test fetch-math math/test_exp.cpp)
add_fetch_test(spline_test fetch-math math/test_spline.cpp)
add_fetch_test(rectangular_load_save_test fetch-math math/rectangular_load_save_test.cpp)
add_fetch_test(shared_rectangular_array_test fetch-math math/shared_rectangular_array_test.cpp)
#add_fetch_test(matrix_test fetch-math math/linalg/matrix.cpp)
add_fetch_gtest(matrix_test fetch-math math/linalg/matrix)
target_link_libraries(matrix_test PRIVATE pthread)

add_fetch_test(hamming_test fetch-math math/distance/hamming.cpp DISABLED)
add_fetch_test(manhattan_test fetch-math math/distance/manhattan.cpp)
add_fetch_test(correlation_test fetch-math math/distance/correlation.cpp)
add_fetch_test(distance_matrix_test fetch-math math/distance/distance_matrix.cpp DISABLED)

add_fetch_gtest(ndarray_test fetch-math math/ndarray)
add_fetch_gtest(ndarray_view_test fetch-math math/ndarray_view)
add_fetch_gtest(ndarray_iterator_test fetch-math math/ndarray_iterator)
add_fetch_gtest(ndarray_broadcast_test fetch-math math/ndarray_broadcast)
add_fetch_gtest(ndarray_squeeze_test fetch-math math/ndarray_squeeze)

add_fetch_gtest(relu_test fetch-math math/kernels/relu)
add_fetch_gtest(sign_test fetch-math math/kernels/sign)
add_fetch_gtest(l2loss_test fetch-math math/shape_less_array/l2loss)

# disabled / not working tests
add_fetch_test(load_image_read_exceptions_test fetch-math image/load_image_read_exceptions.cpp DISABLED)

