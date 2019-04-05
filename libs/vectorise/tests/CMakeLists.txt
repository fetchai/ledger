################################################################################
# F E T C H   V E C T O R I S A T I O N   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-vectorisation-tests)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(vectorise_memory_gtest fetch-vectorise memory)
include_directories(vectorise_memory_gtest PRIVATE "../../core/include") # This should be handled with library imports

add_fetch_test(vectorise_exponent_gtest fetch-vectorise vectorize)
add_fetch_test(vectorise_gtest fetch-vectorise gtest SLOW)
add_fetch_test(vectorise_exact_exponents_gtest fetch-vectorise meta/gtest)

