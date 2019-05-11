################################################################################
# F E T C H   C O R E   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-core)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(fetch_serializers_tests        fetch-core serializers)
add_fetch_test(fetch_byte_array_tests         fetch-core byte_array)
add_fetch_test(fetch_random_generator_tests   fetch-core random)
add_fetch_test(fetch_json_tests               fetch-core json/test/)
add_fetch_test(fetch_core_encode_decode_tests fetch-core tokenizer/test/)
add_fetch_test(fetch_fixed_point_tests        fetch-core fixed_point/)
add_fetch_test(fetch_containers_tests        fetch-core containers/ SLOW)
add_fetch_test(fetch_sync_tests               fetch-core sync/ SLOW)
