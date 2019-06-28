#
# F E T C H   M L   T E S T S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ml)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(ml_dataloader_gtest fetch-ml ml/dataloader)
fetch_add_test(ml_layers_gtest fetch-ml ml/layers)
fetch_add_test(ml_ops_gtest fetch-ml ml/ops)
fetch_add_test(ml_optimisation_gtest fetch-ml ml/optimisation)
fetch_add_test(ml_serializers_gtest fetch-ml ml/serializers)

fetch_add_slow_test(ml_clustering_gtest fetch-ml ml/clustering)
fetch_add_slow_test(ml_estimator_gtest fetch-ml ml/estimators)
fetch_add_slow_test(ml_training_gtest fetch-ml ml/training)
