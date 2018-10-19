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

add_fetch_test(tcp_client_stress_test fetch-network network/tcp_client_stress.cpp)
target_include_directories(tcp_client_stress_test PRIVATE include)
target_link_libraries(tcp_client_stress_test PRIVATE fetch-ledger)

add_fetch_test(tcp_client_server_stress_test fetch-network network/tcp_client_server_stress.cpp DISABLED)
#target_link_libraries(tcp_client_server_stress_test PRIVATE fetch-ledger)

add_fetch_test(thread_manager_stress_test fetch-network network/thread_manager_stress.cpp)
add_fetch_test(dummy_test fetch-network network/dummy_test.cpp)
add_fetch_test(callable_test fetch-network callable.cpp)

add_fetch_test(peer_test fetch-network peer_test.cpp)
target_link_libraries(peer_test PRIVATE gtest gtest_main)

add_fetch_gtest(p2ptrust-tests fetch-network p2ptrust)
add_fetch_gtest(muddle-tests fetch-network muddle)
add_fetch_gtest(p2p-tests fetch-network p2p)
add_fetch_gtest(thread-pool-tests fetch-network thread_pool)
