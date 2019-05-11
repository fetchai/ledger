################################################################################
# F E T C H   N E T W O R K   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-network)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Test Targets
#-------------------------------------------------------------------------------

add_fetch_test(fetch_p2ptrust_tests fetch-network p2ptrust)
add_fetch_test(fetch_muddle_tests fetch-network muddle SLOW)
add_fetch_test(fetch_p2p_tests fetch-network p2p)
add_fetch_test(fetch_thread_pool_tests fetch-network thread_pool SLOW)
target_include_directories(fetch_muddle_tests PRIVATE include)
target_link_libraries(fetch_muddle_tests PRIVATE fetch-ledger)

add_fetch_test(fetch_network_peer_tests fetch-network tests)
add_fetch_test(fetch_network_tests fetch-network network SLOW)
target_include_directories(fetch_network_tests PRIVATE include)
target_link_libraries(fetch_network_tests PRIVATE fetch-ledger)

add_fetch_test(fetch_packet_tests fetch-network packet)
