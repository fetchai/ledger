#
# F E T C H   D M L F   T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-dmlf)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_slow_test(dmlf_deprecated_gtest fetch-dmlf deprecated)
fetch_add_test(dmlf_execution_gtest fetch-dmlf execution)
fetch_add_test(dmlf_colearn_gtest fetch-dmlf colearn)
fetch_add_slow_test(dmlf_dmlfmuddle_gtest fetch-dmlf dmlf_muddle)
fetch_add_test(dmlf_var_converter_test fetch-dmlf var_converter)
fetch_add_slow_test(dmlf_colearn_muddle_gtest fetch-dmlf colearn_muddle)
