################################################################################
# F E T C H   N E T W O R K   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1)
project(fetch-network)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-network)
target_link_libraries(fetch-network INTERFACE fetch-core vendor-asio)

#-------------------------------------------------------------------------------
# Unit Test Targets
#-------------------------------------------------------------------------------

add_fetch_test(tcp_client_stress_test fetch-network tests/network/tcp_client_stress.cpp)
add_fetch_test(thread_manager_stress_test fetch-network tests/network/thread_manager_stress.cpp)
add_fetch_test(dummy_test fetch-network tests/network/dummy_test.cpp)
add_fetch_test(thread_pool_stress_test fetch-network tests/network/thread_pool_stress.cpp)

# not working
add_fetch_test(promise_counter_test fetch-network tests/rpc/promise_counter.cpp DISABLED)

# missing tests
#add_fetch_test(referenced_byte_array_test fetch-network tests/serializers/referenced_byte_array.cpp)
#standard_test(typed_byte_array_test serializers/typed_byte_array_test.cpp)
#standard_test(variant_test script/variant.cpp)

#-------------------------------------------------------------------------------
# Example Targets
#-------------------------------------------------------------------------------

setup_library_examples(fetch-network)

if(FETCH_BUILD_EXAMPLES)
  target_link_libraries(example_swarm PRIVATE fetch-http)
endif(FETCH_BUILD_EXAMPLES)