#
# F E T C H   C O R E   E X A M P L E S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-core)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()


if (FETCH_ENABLE_EXAMPLES)

#  add_executable(example-msg-pack msg_pack/main.cpp)
#  target_link_libraries(example-msg-pack PRIVATE fetch-core)
endif()