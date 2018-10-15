################################################################################
# F E T C H   S T O R A G E   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-storage)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-storage)
target_link_libraries(fetch-storage INTERFACE fetch-core fetch-crypto fetch-ledger fetch-testing fetch-network vendor-openssl)

#-------------------------------------------------------------------------------
# Example Targets
#-------------------------------------------------------------------------------

if(FETCH_ENABLE_EXAMPLES)

  add_executable(example_state_shard_server examples/state_shard_server.cpp)
  target_link_libraries(example_state_shard_server PRIVATE fetch-storage)

  add_executable(example_state_shard_client examples/state_shard_client.cpp)
  target_link_libraries(example_state_shard_client PRIVATE fetch-storage)

  add_executable(example_object_store examples/object_store.cpp)
  target_link_libraries(example_object_store PRIVATE fetch-storage)

endif(FETCH_ENABLE_EXAMPLES)

add_test_target()
