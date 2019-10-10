#
# F E T C H   V A R I A N T   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-variant)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

setup_library(fetch-variant)
target_link_libraries(fetch-variant PUBLIC fetch-core fetch-crypto fetch-meta)

if (_is_clang_compiler)
  target_compile_options(fetch-variant PRIVATE -Wshadow)
endif ()

add_test_target()
