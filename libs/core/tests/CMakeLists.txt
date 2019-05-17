################################################################################
# F E T C H   C O R E   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-core)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(fetch-core-bitvector-tests fetch-core bitvector/)
add_fetch_test(serializers_gtest fetch-core serializers)
add_fetch_test(byte_array_gtest fetch-core byte_array)
add_fetch_test(random_generator_gtest fetch-core random)
add_fetch_test(json_gtest fetch-core json/gtest/)
add_fetch_test(core_encode_decode_gtest fetch-core tokenizer/gtest/)
add_fetch_test(containers-tests fetch-core containers/ SLOW)
add_fetch_test(sync_gtest fetch-core sync/ SLOW)
