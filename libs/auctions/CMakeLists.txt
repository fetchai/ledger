################################################################################
# F E T C H   A U C T I O N S   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-auctions)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-auctions)
target_link_libraries(fetch-auctions INTERFACE fetch-core fetch-math)

add_test_target()
