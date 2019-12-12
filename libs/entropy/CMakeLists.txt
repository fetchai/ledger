#
# F E T C H   A D D R E S S   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-entropy)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-entropy)
target_link_libraries(fetch-entropy INTERFACE fetch-core)
