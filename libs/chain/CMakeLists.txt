################################################################################
# F E T C H   C H A I N   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(fetch-chain)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-chain)
target_link_libraries(fetch-chain INTERFACE fetch-core vendor-openssl)

#-------------------------------------------------------------------------------
# Test Targets
#-------------------------------------------------------------------------------
