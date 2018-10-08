################################################################################
# F E T C H   M L   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ml)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()
add_fetch_gtest(ops_test fetch-ml ml/ops)
add_fetch_gtest(session_test fetch-ml ml/session)
add_fetch_gtest(computation_graph_test fetch-ml ml/computation_graph)
add_fetch_gtest(variable_test fetch-ml ml/variable)
add_fetch_gtest(layers_test fetch-ml ml/layers)