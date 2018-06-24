################################################################################
# F E T C H   C O R E   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1)
project(fetch-core)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-core)
target_link_libraries(fetch-core PUBLIC fetch-vectorise)

#-------------------------------------------------------------------------------
# Test Targets
#-------------------------------------------------------------------------------

add_fetch_test(tokenizer_test fetch-core tests/tokenizer/tokenizer.cpp)
add_fetch_test(encode_decode_test fetch-core tests/encode_decode.cpp)
add_fetch_test(referenced_byte_array_test fetch-core tests/referenced_byte_array.cpp)
add_fetch_test(lfg_test fetch-core tests/test_lfg.cpp)
