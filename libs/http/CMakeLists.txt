################################################################################
# F E T C H   H T T P   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1)
project(fetch-http)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-http)
target_link_libraries(fetch-http PUBLIC fetch-network)

#-------------------------------------------------------------------------------
# Test Targets
#-------------------------------------------------------------------------------

add_fetch_test(json_document_test fetch-http tests/json/document.cpp)
add_fetch_test(variant_test fetch-http tests/script/variant.cpp)

#-------------------------------------------------------------------------------
# Benchmark Targets
#-------------------------------------------------------------------------------

add_executable(benchmark_json_document benchmark/json/document.cpp)
target_link_libraries(benchmark_json_document PRIVATE fetch-http fetch-testing)

#-------------------------------------------------------------------------------
# Example Targets
#-------------------------------------------------------------------------------

setup_library_examples(fetch-http)
