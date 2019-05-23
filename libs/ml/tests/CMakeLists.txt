################################################################################
# F E T C H   M L   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ml)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()
add_fetch_test(ml_ops_gtest fetch-ml ml/ops)
add_fetch_test(ml_serializers_gtest fetch-ml ml/serializers)
add_fetch_test(ml_dataloader_gtest fetch-ml ml/dataloader)
add_fetch_test(ml_layers_gtest fetch-ml ml/layers)
add_fetch_test(ml_training_gtest fetch-ml ml/training SLOW)
add_fetch_test(ml_optimization_gtest fetch-ml ml/optimization)
add_fetch_test(ml_clustering_gtest fetch-ml ml/clustering)

