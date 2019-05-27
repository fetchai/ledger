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

add_fetch_test(p2ptrust_gtest fetch-network p2ptrust)
add_fetch_test(muddle_gtest
               fetch-network
               muddle
               SLOW)
add_fetch_test(p2p_gtest fetch-network p2p)
add_fetch_test(thread_pool_gtest
               fetch-network
               thread_pool
               SLOW)
target_include_directories(muddle_gtest PRIVATE include)
target_link_libraries(muddle_gtest PRIVATE fetch-ledger)

add_fetch_test(network_peer_gtest fetch-network gtest)
add_fetch_test(network_gtest
               fetch-network
               network
               SLOW)
target_include_directories(network_gtest PRIVATE include)
target_link_libraries(network_gtest PRIVATE fetch-ledger)

add_fetch_test(packet_gtest fetch-network packet)
