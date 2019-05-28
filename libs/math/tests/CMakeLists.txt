#
# F E T C H   M A T H   T E S T S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-math)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(math_basic_math_gtest fetch-math math/basic_math)
fetch_add_test(math_clustering_gtest fetch-math math/clustering)
fetch_add_test(math_combinatorics_gtest fetch-math math/combinatorics)
fetch_add_test(math_correlation_gtest fetch-math math/correlation)
fetch_add_test(math_distance_gtest fetch-math math/distance)
fetch_add_test(math_fixed_point_gtest fetch-math math/fixed_point/)
fetch_add_test(math_matrix_gtest fetch-math math/matrix_operations)
fetch_add_test(math_activation_gtest fetch-math math/ml_activation_functions)
fetch_add_test(math_loss_gtest fetch-math math/ml_loss_functions)
fetch_add_test(math_serializers_gtest fetch-math math/serializers)
fetch_add_test(math_tensor_gtest fetch-math math/tensor)
fetch_add_test(math_trigonometry_gtest fetch-math math/trigonometry)
fetch_add_test(math_blas_gtest fetch-math math/linalg/blas)
fetch_add_test(math_tensor_iterator fetch-math math/tensor_iterator)

fetch_add_slow_test(math_gtest_gtest fetch-math math/gtest)
