#
# F E T C H   N E T W O R K   T E S T S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-network)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Test Targets
# ------------------------------------------------------------------------------

fetch_add_test(p2ptrust_gtest fetch-network p2ptrust)
fetch_add_test(p2p_gtest fetch-network p2p)
fetch_add_test(network_peer_gtest fetch-network gtest)
fetch_add_test(packet_gtest fetch-network packet)

fetch_add_slow_test(thread_pool_gtest fetch-network thread_pool)

fetch_add_slow_test(network_gtest
                    fetch-network
                    network
                    DISABLED)
# target_include_directories(network_gtest PRIVATE include) target_link_libraries(network_gtest
# PRIVATE fetch-ledger)

fetch_add_integration_test(muddle_gtest
                           fetch-network
                           muddle
                           DISABLED)
# target_include_directories(muddle_gtest PRIVATE include) target_link_libraries(muddle_gtest
# PRIVATE fetch-ledger)
