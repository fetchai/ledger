################################################################################
# F E T C H   V E C T O R I S A T I O N   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-vectorisation-tests)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(fetch_vectorise_memory_tests fetch-vectorise memory)
include_directories(fetch_vectorise_memory_tests PRIVATE "../../core/include") # This should be handled with library imports

add_fetch_test(fetch_vectorise_exponent_tests fetch-vectorise vectorize)
add_fetch_test(fetch_vectorise_tests fetch-vectorise tests SLOW)
add_fetch_test(fetch_vectorise_exact_exponents_tests fetch-vectorise meta/tests)

