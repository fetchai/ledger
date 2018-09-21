################################################################################
# F E T C H    N E T W O R K   B E N C H M A R K S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-network)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Benchmark Targets
#-------------------------------------------------------------------------------
add_executable(network_benchmark network/network_benchmark.cpp)
target_link_libraries(network_benchmark PRIVATE fetch-http fetch-ledger)
target_include_directories(network_benchmark PRIVATE "../tests/include")

add_executable(network_mine_test network/network_mine_test.cpp)
target_link_libraries(network_mine_test PRIVATE fetch-http fetch-ledger)
target_include_directories(network_mine_test PRIVATE "../tests/include")

add_executable(rpc rpc/main.cpp)
target_link_libraries(rpc PRIVATE fetch-network fetch-ledger)
target_include_directories(rpc PRIVATE "../tests/include")

