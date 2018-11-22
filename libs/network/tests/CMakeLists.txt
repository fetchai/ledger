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

add_fetch_gtest(p2ptrust-tests fetch-network p2ptrust)
add_fetch_gtest(muddle-tests fetch-network muddle)
add_fetch_gtest(p2p-tests fetch-network p2p)
add_fetch_gtest(thread-pool-tests fetch-network thread_pool)
target_include_directories(muddle-tests PRIVATE include)
target_link_libraries(muddle-tests PRIVATE fetch-ledger)

add_fetch_gtest(network_gtest fetch-network gtest)
