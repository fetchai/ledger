################################################################################
# F E T C H   S T O R A G E   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(fetch-storage)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-storage)

target_link_libraries(fetch-storage INTERFACE fetch-core fetch-crypto fetch-testing fetch-network vendor-openssl)

#-------------------------------------------------------------------------------
# Example Targets
#-------------------------------------------------------------------------------

if(FETCH_ENABLE_EXAMPLES) 

  add_executable(example_state_shard_server examples/state_shard_server.cpp)
  target_link_libraries(example_state_shard_server PRIVATE fetch-storage)

  add_executable(example_state_shard_client examples/state_shard_client.cpp)
  target_link_libraries(example_state_shard_client PRIVATE fetch-storage)


endif(FETCH_ENABLE_EXAMPLES)

#-------------------------------------------------------------------------------
# Test Targets
#-------------------------------------------------------------------------------

add_fetch_test(file_object_test fetch-storage tests/file_object.cpp)
add_fetch_test(random_access_stack_test fetch-storage tests/random_access_stack.cpp)
add_fetch_test(versioned_random_access_stack_test fetch-storage tests/versioned_random_access_stack.cpp)

add_fetch_test(key_value_index_test fetch-storage tests/key_value_index.cpp)
add_fetch_test(indexed_document_store_test fetch-storage tests/indexed_document_store.cpp)
add_fetch_test(variant_stack_test fetch-storage tests/variant_stack.cpp)
add_fetch_test(versioned_file_object_test fetch-storage tests/versioned_file_object.cpp)
add_fetch_test(versioned_kvi_test fetch-storage tests/versioned_kvi.cpp)

