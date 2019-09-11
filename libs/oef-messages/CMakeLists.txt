#
# F E T C H   O E F - C O R E   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-oef-messages)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-oef-messages)

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

## TODO: Work out how to add dependencies between each other
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS protos/search_message.proto protos/search_query.proto protos/search_remove.proto protos/search_update.proto protos/agent.proto protos/dap_interface.proto protos/fipa.proto protos/query.proto protos/config.proto protos/search_config.proto protos/transport.proto protos/search_transport.proto protos/search_response.proto)



add_library(protos ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(protos ${Protobuf_LIBRARIES})
target_include_directories(protos INTERFACE ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(fetch-oef-messages
                      INTERFACE protos)
target_include_directories(fetch-oef-messages INTERFACE ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

# Test targets
# add_test_target()

# Example targets
# add_subdirectory(examples)
