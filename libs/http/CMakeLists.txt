################################################################################
# F E T C H   H T T P   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-http)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-http)
target_link_libraries(fetch-http PUBLIC fetch-network)

add_subdirectory(examples)
add_subdirectory(tests)
