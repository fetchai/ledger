################################################################################
# F E T C H   M L   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ml)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()
add_fetch_test(fetch_ml_ops_tests fetch-ml ml/ops)
add_fetch_test(fetch_ml_serializers_tests fetch-ml ml/serializers)
add_fetch_test(fetch_ml_dataloader_tests fetch-ml ml/dataloader)
add_fetch_test(fetch_ml_layers_tests fetch-ml ml/layers)
add_fetch_test(fetch_ml_training_tests fetch-ml ml/training SLOW)
add_fetch_test(fetch_ml_clustering_tests fetch-ml ml/clustering)

