#
# F E T C H   O E F - S E A R C H  L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-oef-search)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-oef-search)
target_link_libraries(fetch-oef-search
                      PUBLIC fetch-core
                             fetch-oef-messages
                             fetch-oef-base
                             fetch-network)

# Test targets add_test_target()

# Example targets add_subdirectory(examples)
