################################################################################
# F E T C H   M L   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ml)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()
add_fetch_gtest(ops fetch-ml ml/ops)
add_fetch_gtest(computation_graph fetch-ml ml/computation_graph)






add_fetch_gtest(variable fetch-ml ml/variable)

add_fetch_gtest(layers fetch-ml ml/layers)