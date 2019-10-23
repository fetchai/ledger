#
# F E T C H   O E F - C O R E   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-oef-core)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-oef-core)
target_link_libraries(fetch-oef-core
                      PUBLIC fetch-core
                             fetch-oef-messages
                             fetch-oef-base
                             fetch-network
                             vendor-openssl)

# Test targets add_test_target()

# Example targets
add_subdirectory(examples)
