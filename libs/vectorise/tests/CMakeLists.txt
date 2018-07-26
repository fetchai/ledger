################################################################################
# F E T C H   V E C T O R I S A T I O N   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
project(fetch-vectorisation-tests)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(vec_exp fetch-vectorise vectorize/exp.cpp)
add_fetch_test(parallel_dispatcher_test fetch-vectorise memory/parallel_dispatcher.cpp)
add_fetch_test(log2_test fetch-vectorise meta/test_log2.cpp)
add_fetch_test(native_test fetch-vectorise native.cpp)
add_fetch_test(sse_test fetch-vectorise sse.cpp)

add_fetch_test(shared_array_tests fetch-vectorise memory/shared_array_test.cpp DISABLED)

