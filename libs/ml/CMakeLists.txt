#
# F E T C H   M L   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-ml)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-ml)
<<<<<<< HEAD
target_link_libraries(fetch-ml INTERFACE fetch-core fetch-math fetch-dmlf)
=======
target_link_libraries(fetch-ml PUBLIC fetch-core fetch-crypto fetch-math)
>>>>>>> bb5c24ef2ef32742f7626039058151b6186770d9

# ------------------------------------------------------------------------------
# Example Targets
# ------------------------------------------------------------------------------

add_subdirectory(examples)

# ------------------------------------------------------------------------------
# Test Targets
# ------------------------------------------------------------------------------

add_test_target()

# ------------------------------------------------------------------------------
# Benchmark Targets
# ------------------------------------------------------------------------------

add_subdirectory(benchmark)
