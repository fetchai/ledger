#
# F E T C H   C O N S T E L L A T I O N   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-constellation)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-constellation)
target_link_libraries(fetch-constellation
                      PUBLIC fetch-core
                             fetch-http
                             fetch-ledger
                             fetch-network
                             fetch-crypto
                             fetch-math
                             fetch-storage
                             fetch-vm
                             fetch-vm-modules
                             fetch-moment
                             fetch-dkg
                             fetch-beacon
                             fetch-messenger
                             vendor-msgpack)

# TODO(HUT): Will do in subsequent PR add_test_target()
