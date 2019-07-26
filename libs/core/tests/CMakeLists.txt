#
# F E T C H   C O R E   T E S T S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-core)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(fetch-core-unit-tests fetch-core unit)
fetch_add_test(fetch-core-bitvector-tests fetch-core bitvector)
fetch_add_test(serializers_gtest fetch-core serializers)
fetch_add_test(byte_array_gtest fetch-core byte_array)
fetch_add_test(random_generator_gtest fetch-core random)
fetch_add_test(json_gtest fetch-core json/gtest)
fetch_add_test(core_encode_decode_gtest fetch-core tokenizer/gtest)

fetch_add_slow_test(containers-tests fetch-core containers)
fetch_add_slow_test(sync_gtest fetch-core sync)
