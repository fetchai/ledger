#
# F E T C H   V E C T O R I S E   T E S T S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-vectorisation-tests)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(vectorise_exponent_gtest fetch-vectorise vectorize)
fetch_add_test(vectorise_exact_exponents_gtest fetch-vectorise meta/gtest)

fetch_add_test(vectorise_memory_gtest fetch-vectorise memory)
target_link_libraries(vectorise_memory_gtest PRIVATE fetch-core)

fetch_add_slow_test(vectorise_gtest fetch-vectorise gtest)
target_link_libraries(vectorise_gtest PRIVATE fetch-core)
