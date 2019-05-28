#
# F E T C H   V E C T O R I S E   T E S T S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-vectorisation-tests)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_test(vectorise_memory_gtest fetch-vectorise memory)
add_fetch_test(vectorise_exponent_gtest fetch-vectorise vectorize)
add_fetch_test(vectorise_gtest
               fetch-vectorise
               gtest
               SLOW)
add_fetch_test(vectorise_exact_exponents_gtest fetch-vectorise meta/gtest)

target_link_libraries(vectorise_gtest PRIVATE fetch-core)
target_link_libraries(vectorise_memory_gtest PRIVATE fetch-core)
