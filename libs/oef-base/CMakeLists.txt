#
# F E T C H   O E F - B A S E  L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-oef-base)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-oef-base)
target_link_libraries(fetch-oef-base PUBLIC
                                     fetch-core
#                                     fetch-oef-messages
                                     fetch-network)

# Test targets add_test_target()

# Example targets add_subdirectory(examples)

#add_test_target()
