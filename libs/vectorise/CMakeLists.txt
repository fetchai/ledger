################################################################################
# F E T C H   V E C T O R I S A T I O N   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(fetch-vectorise)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-vectorise)

#-------------------------------------------------------------------------------
# Test Targets
#-------------------------------------------------------------------------------

add_fetch_test(vec_exp fetch-vectorise tests/vectorize/exp.cpp)
add_fetch_test(parallel_dispatcher_test fetch-vectorise tests/memory/parallel_dispatcher.cpp)
add_fetch_test(log2_test fetch-vectorise tests/meta/test_log2.cpp)
add_fetch_test(native_test fetch-vectorise tests/native.cpp)
add_fetch_test(sse_test fetch-vectorise tests/sse.cpp)

add_fetch_test(shared_array_tests fetch-vectorise tests/memory/shared_array_test.cpp DISABLED)
