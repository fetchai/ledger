################################################################################
# F E T C H   M A T H   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-math)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(math_basic_math_gtest fetch-math math/basic_math)
add_fetch_test(math_clustering_gtest fetch-math math/clustering)
add_fetch_test(math_combinatorics_gtest fetch-math math/combinatorics)
add_fetch_test(math_correlation_gtest fetch-math math/correlation)
add_fetch_test(math_distance_gtest fetch-math math/distance)
add_fetch_test(math_gtest_gtest fetch-math math/gtest SLOW)
add_fetch_test(math_kernels_gtest fetch-math math/kernels)
add_fetch_test(math_matrix_gtest fetch-math math/matrix_operations)
add_fetch_test(math_activation_gtest fetch-math math/ml_activation_functions)
add_fetch_test(math_loss_gtest fetch-math math/ml_loss_functions)
add_fetch_test(math_serializers_gtest fetch-math math/serializers)
add_fetch_test(math_tensor_gtest fetch-math math/tensor)
add_fetch_test(math_trigonometry_gtest fetch-math math/trigonometry)
