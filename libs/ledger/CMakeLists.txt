#
# F E T C H   L E D G E R   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-ledger)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-ledger)
target_link_libraries(fetch-ledger
                      PUBLIC fetch-beacon
                             fetch-bloomfilter
			     fetch-constellation
		             fetch-core
                             fetch-crypto
                             fetch-entropy
                             fetch-http
                             fetch-logging
                             fetch-math
                             fetch-moment
                             fetch-network
                             fetch-shards
                             fetch-storage
                             fetch-vm
                             fetch-vm-modules
                             vendor-msgpack)

add_test_target()
add_subdirectory(examples)
add_subdirectory(benchmark)
