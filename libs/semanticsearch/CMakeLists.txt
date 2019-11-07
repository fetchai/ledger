# ##################################################################################################
# F E T C H   S E M A N T I C   S E A R C H   L I B R A R Y
# ##################################################################################################
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-semanticsearch)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# -------------------------------------------------------------------------------
# Main Library Target
# -------------------------------------------------------------------------------
setup_library(fetch-semanticsearch)
target_link_libraries(fetch-semanticsearch
                      PUBLIC fetch-network
                             fetch-muddle
                             fetch-core
                             fetch-crypto
                             fetch-math)


add_test_target()
add_subdirectory(examples)
