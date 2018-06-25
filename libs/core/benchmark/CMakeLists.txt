################################################################################
# F E T C H   H T T P   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(fetch-core)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Benchmark Targets
#-------------------------------------------------------------------------------

add_executable(benchmark_json_document json/document.cpp)
target_link_libraries(benchmark_json_document PRIVATE fetch-core fetch-testing)

