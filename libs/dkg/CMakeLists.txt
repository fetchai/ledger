#
# F E T C H   D K G   L I B R A R Y
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-dkg)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()
set(CMAKE_CXX_FLAGS -Wno-error=deprecated)

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-dkg)
target_link_libraries(fetch-dkg
                      PUBLIC fetch-core
                             fetch-crypto
                             fetch-network
                             fetch-ledger)

add_test_target()
