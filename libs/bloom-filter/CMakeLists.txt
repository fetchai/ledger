#
# F E T C H   B L O O M   F I L T E R   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-bloomfilter)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-bloomfilter)
target_link_libraries(fetch-bloomfilter PUBLIC fetch-core fetch-crypto fetch-logging)

add_test_target()
