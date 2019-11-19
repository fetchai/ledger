#
# F E T C H   N E T W O R K   T E S T S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-network)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Test Targets
# ------------------------------------------------------------------------------

# define the test targets
fetch_add_test(network-unit-tests fetch-network unit/)
fetch_add_integration_test(network-integration-tests fetch-network integration/)
target_link_libraries(network-integration-tests PRIVATE fetch-ledger)
