#
# F E T C H   O E F   M T   C O R E   E X A M P L E S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-oef-core)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Example Targets
# ------------------------------------------------------------------------------

setup_library_examples(fetch-oef-core)

if (FETCH_ENABLE_EXAMPLES)
  add_executable(example-standalone-core standalone_core/main.cpp)
  target_link_libraries(example-standalone-core PRIVATE fetch-oef-core fetch-oef-base)
  # target_include_directories(example-network-subscribe PRIVATE subscribe)
endif (FETCH_ENABLE_EXAMPLES)
