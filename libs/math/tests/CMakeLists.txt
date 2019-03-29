################################################################################
# F E T C H   M A T H   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-math)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()


add_fetch_test(combinatorics_gtest fetch-math math/combinatorics)

add_fetch_test(free_functions_gtest fetch-math math/free_functions)
add_fetch_test(activation_functions fetch-math math/ml_activation_functions)
add_fetch_test(loss_functions fetch-math math/ml_loss_functions)

add_fetch_test(sign_gtest fetch-math math/kernels/sign)

add_fetch_test(big_number_gtest fetch-math math/basic_math_tests)
add_fetch_test(distances_gtest fetch-math math/distance/gtest)
add_fetch_test(clustering_gtest fetch-math math/clustering)

add_fetch_test(math_gtest fetch-math math/gtest SLOW)

add_fetch_test(tensor_gtest fetch-math math/tensor)
add_fetch_test(math_serializers_gtest fetch-math math/serializers)
