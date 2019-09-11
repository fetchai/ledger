#
# F E T C H   N E T W O R K   E X A M P L E S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-network)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Example Targets
# ------------------------------------------------------------------------------

setup_library_examples(fetch-network)
