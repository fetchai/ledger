################################################################################
# F E T C H   N E T W O R K   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(fetch-network)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-network)
target_link_libraries(fetch-network PUBLIC fetch-core fetch-crypto vendor-asio pthread)

add_test_target()
add_subdirectory(examples)
add_subdirectory(benchmark)

