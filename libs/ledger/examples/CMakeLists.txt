#
# F E T C H   L E D G E R   E X A M P L E S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ledger)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

setup_library_examples(fetch-ledger)
