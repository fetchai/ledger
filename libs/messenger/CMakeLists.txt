#
# F E T C H   M E S S E N G E R   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-messenger)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-messenger)
target_link_libraries(fetch-messenger
                      PUBLIC fetch-core
                             fetch-network
                             fetch-semanticsearch
                             fetch-muddle
                             fetch-http)

add_test_target()

add_subdirectory(examples)
