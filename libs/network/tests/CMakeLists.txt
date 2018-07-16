################################################################################
# F E T C H   N E T W O R K   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(fetch-network)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Test Targets
#-------------------------------------------------------------------------------

add_fetch_test(tcp_client_stress_test fetch-network network/tcp_client_stress.cpp)
add_fetch_test(tcp_client_server_stress_test fetch-network network/tcp_client_server_stress.cpp)

target_link_libraries(tcp_client_stress_test PRIVATE fetch-ledger)
target_link_libraries(tcp_client_server_stress_test PRIVATE fetch-ledger)

add_fetch_test(thread_manager_stress_test fetch-network network/thread_manager_stress.cpp)
add_fetch_test(dummy_test fetch-network network/dummy_test.cpp)
add_fetch_test(thread_pool_stress_test fetch-network network/thread_pool_stress.cpp)
add_fetch_test(callable_test fetch-network callable.cpp)

# not working
add_fetch_test(promise_counter_test fetch-network rpc/promise_counter.cpp DISABLED)

