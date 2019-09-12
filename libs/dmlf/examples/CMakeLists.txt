#
# F E T C H   D I S T R I B U T E D   M L  F R A M E W O R K   E X A M P L E S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-dmlf)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Example Targets
# ------------------------------------------------------------------------------

setup_library_examples(fetch-dmlf)
