################################################################################
# F E T C H   C O R E   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-core)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_gtest(serializers-gtest fetch-core serializers)
add_fetch_gtest(byte-array-gtest fetch-core byte_array)
add_fetch_gtest(random_generator_gtest fetch-core random)
add_fetch_gtest(json-gtest fetch-core json/gtest/)
add_fetch_gtest(core_encode_decode_gtest fetch-core tokenizer/gtest/)
