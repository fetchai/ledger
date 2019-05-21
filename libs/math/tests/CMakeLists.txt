################################################################################
# F E T C H   M A T H   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-math)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(fetch_math_basic_math_tests fetch-math math/basic_math)
add_fetch_test(fetch_math_clustering_tests fetch-math math/clustering)
add_fetch_test(fetch_math_combinatorics_tests fetch-math math/combinatorics)
add_fetch_test(fetch_math_correlation_tests fetch-math math/correlation)
add_fetch_test(fetch_math_distance_tests fetch-math math/distance)
add_fetch_test(fetch_math_tests fetch-math math/tests SLOW)
add_fetch_test(fetch_math_matrix_tests fetch-math math/matrix_operations)
add_fetch_test(fetch_math_activation_tests fetch-math math/ml_activation_functions)
add_fetch_test(fetch_math_loss_tests fetch-math math/ml_loss_functions)
add_fetch_test(fetch_math_tensor_tests fetch-math math/tensor)
add_fetch_test(fetch_math_trigonometry_tests fetch-math math/trigonometry)
add_fetch_test(fetch_math_blas_tests fetch-math math/linalg/blas)
add_fetch_test(fetch_math_tensor_iterator_tests fetch-math math/tensor_iterator)
add_fetch_test(fetch_math_serializers_tests fetch-math math/serializers)
add_fetch_test(fetch_math_fixed_point_tests fetch-math math/fixed_point/)