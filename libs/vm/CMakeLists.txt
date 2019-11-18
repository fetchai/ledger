#
# F E T C H   V M   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-vm)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-vm)
target_link_libraries(fetch-vm
                      PUBLIC utf8cpp
                             fetch-math
                             fetch-core
                             fetch-variant
                             fetch-chain
                             fetch-logging)

if (_is_clang_compiler)
  target_compile_options(fetch-vm PRIVATE -Wshadow)
endif ()

add_test_target()
