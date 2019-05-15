#
# F E T C H   N E T W O R K   E X A M P L E S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-network)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Example Targets
# ------------------------------------------------------------------------------

setup_library_examples(fetch-network)

if(FETCH_ENABLE_EXAMPLES)

  add_executable(example-network-subscribe subscribe/subscribe.cpp)
  target_link_libraries(example-network-subscribe PRIVATE fetch-network)
  target_include_directories(example-network-subscribe PRIVATE subscribe)

  add_executable(example-network-listener subscribe/listener.cpp)
  target_link_libraries(example-network-listener PRIVATE fetch-network)
  target_include_directories(example-network-listener PRIVATE subscribe)

  add_executable(example-network-rpc-server rpc/server.cpp)
  target_link_libraries(example-network-rpc-server PRIVATE fetch-network)

  add_executable(example-network-rpc-client rpc/client.cpp)
  target_link_libraries(example-network-rpc-client
                        PRIVATE fetch-network fetch-crypto)

  add_executable(example-network-mrpc-server muddle_rpc/server.cpp)
  target_link_libraries(example-network-mrpc-server
                        PRIVATE fetch-network fetch-crypto)

  add_executable(example-network-mrpc-client muddle_rpc/client.cpp)
  target_link_libraries(example-network-mrpc-client PRIVATE fetch-network)

  add_executable(example-network-reverserpc-server reverse_rpc/server.cpp)
  target_link_libraries(example-network-reverserpc-server PRIVATE fetch-network)

  add_executable(example-network-reverserpc-client reverse_rpc/client.cpp)
  target_link_libraries(example-network-reverserpc-client PRIVATE fetch-network)

endif(FETCH_ENABLE_EXAMPLES)
