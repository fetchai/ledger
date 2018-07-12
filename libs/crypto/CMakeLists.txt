################################################################################
# F E T C H   C R Y P T O   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(fetch-crypto)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-crypto)
target_link_libraries(fetch-crypto INTERFACE fetch-core vendor-openssl)

add_test_target()

add_subdirectory(examples)

