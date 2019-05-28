#
# F E T C H   T I M E   L I B R A R Y
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-moment)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-moment)
target_link_libraries(fetch-moment PUBLIC fetch-core)

# Unit tests
add_subdirectory(tests)
