################################################################################
# F E T C H   N E T W O R K   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
project(fetch-network)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Test Targets
#-------------------------------------------------------------------------------

add_fetch_test(tcp_client_stress_test fetch-network network/tcp_client_stress.cpp)
target_link_libraries(tcp_client_stress_test PRIVATE fetch-ledger)
target_include_directories(tcp_client_stress_test PRIVATE include)

add_fetch_test(thread_manager_stress_test fetch-network network/thread_manager_stress.cpp)
add_fetch_test(thread_pool_stress_test fetch-network network/thread_pool_stress.cpp)
add_fetch_test(callable_test fetch-network callable.cpp)

add_fetch_test(peer_test fetch-network peer_test.cpp)
target_link_libraries(peer_test PRIVATE gtest gtest_main)
