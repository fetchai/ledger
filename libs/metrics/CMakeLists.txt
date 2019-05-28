#
# F E T C H   M E T R I C S   L I B R A R Y
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-metrics)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-metrics)
target_link_libraries(fetch-metrics PUBLIC fetch-core)
