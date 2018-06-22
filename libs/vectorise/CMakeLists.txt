cmake_minimum_required(VERSION 3.1)
project(fetch-vectorise)

include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

setup_compiler()

setup_library(fetch-vectorise)

add_fetch_test(vec_exp fetch-vectorise tests/vectorize/exp.cpp)
add_fetch_test(parallel_dispatcher_test fetch-vectorise tests/memory/parallel_dispatcher.cpp)
#add_fetch_test(shared_array_tests fetch-vectorise tests/memory/shared_array_test.cpp)


