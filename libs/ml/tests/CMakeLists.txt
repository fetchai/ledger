################################################################################
# F E T C H   M L   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ml)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()
add_fetch_test(ops_gtest fetch-ml ml/ops)
add_fetch_test(ml_serializers_gtest fetch-ml ml/serializers)


