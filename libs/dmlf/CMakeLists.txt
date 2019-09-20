#
# F E T C H   D I S T R I B U T E D   M L  F R A M E W O R K   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-dmlf)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-dmlf)
target_link_libraries(fetch-dmlf
                      PUBLIC fetch-core
                             fetch-ml
                             fetch-network
                             fetch-muddle
                             fetch-crypto
                             fetch-math
                             fetch-vm
                             fetch-vm-modules
                             pthread)

# Test targets
add_test_target()

# Example targets
add_subdirectory(examples)
