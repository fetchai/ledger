################################################################################
# F E T C H   C O R E   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-core)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(lfg_test fetch-core test_lfg.cpp)
add_fetch_gtest(serializers-gtests fetch-core serializers)
add_fetch_gtest(byte-array-gtests fetch-core byte_array)
add_fetch_gtest(random_generator_test fetch-core random)
add_fetch_gtest(json-tests fetch-core json/gtest/)
add_fetch_gtest(core_encode_decode_gtest fetch-core tokenizer/gtest/)
add_fetch_gtest(containers-tests fetch-core containers/)
