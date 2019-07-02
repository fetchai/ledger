#
# F E T C H   V M   L I B R A R Y
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-vm)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-vm)
target_link_libraries(fetch-vm PUBLIC fetch-math fetch-core fetch-ledger)

