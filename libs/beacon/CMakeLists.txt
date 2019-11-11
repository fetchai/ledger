#
# F E T C H   B E A C O N   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-beacon)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-beacon)
target_link_libraries(fetch-beacon
                      PUBLIC fetch-core
                             fetch-shards
                             fetch-crypto
                             fetch-network
                             fetch-dkg)

# Test targets
add_test_target()
add_subdirectory(benchmark)
