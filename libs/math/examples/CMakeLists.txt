################################################################################
# F E T C H   M A T H   E X A M P L E S
################################################################################
cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
project(fetch-math)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

setup_library_examples(fetch-math)

if (FETCH_ENABLE_EXAMPLES)
   target_link_libraries(example-math-blas-dgemm PRIVATE pthread)
endif(FETCH_ENABLE_EXAMPLES)
