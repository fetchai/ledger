#
# F E T C H   M L   B E N C H M A R K S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-ml)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_gbench(benchmark_ml_activations fetch-ml activations)
add_fetch_gbench(benchmark_ml_ops fetch-ml ops)
add_fetch_gbench(benchmark_ml_embeddings fetch-ml embeddings)
add_fetch_gbench(benchmark_ml_training fetch-ml training)
add_fetch_gbench(benchmark_ml_serialization fetch-ml serialization)
add_fetch_gbench(benchmark_ml_metrics fetch-ml metrics)
