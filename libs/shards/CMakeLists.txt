#
# F E T C H   S H A R D S   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-shards)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-shards)
target_link_libraries(fetch-shards PUBLIC fetch-core fetch-logging fetch-muddle)

add_test_target()
