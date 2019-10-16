#
# F E T C H   V M   M O D U L E S   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-vm-modules)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-vm-modules)
target_link_libraries(fetch-vm-modules
                      PUBLIC fetch-vm
                             fetch-core
                             fetch-math
                             fetch-ml
                             fetch-json
                             fetch-variant
                             fetch-ledger # TODO REMOVE ME
                             fetch-logging)

if (_is_clang_compiler)
  target_compile_options(fetch-vm-modules PRIVATE -Wshadow)
endif ()

add_test_target()

add_subdirectory(examples)
