################################################################################
# F E T C H   C O R E   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
project(fetch-core)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(tokenizer_test fetch-core tokenizer/tokenizer.cpp)
add_fetch_test(encode_decode_test fetch-core encode_decode.cpp)
add_fetch_test(referenced_byte_array_test fetch-core referenced_byte_array.cpp)
add_fetch_test(lfg_test fetch-core test_lfg.cpp)
add_fetch_test(json_document_test fetch-core json/document.cpp)
add_fetch_test(variant_test fetch-core script/variant.cpp)

