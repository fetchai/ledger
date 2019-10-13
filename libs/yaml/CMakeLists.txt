#
# F E T C H   Y A M L   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-yaml)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-yaml)
target_link_libraries(fetch-yaml PUBLIC fetch-core fetch-variant fetch-logging)

add_test_target()
