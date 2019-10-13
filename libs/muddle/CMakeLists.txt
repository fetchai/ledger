#
# F E T C H   M U D D L E   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-muddle)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-muddle)
target_link_libraries(fetch-muddle
                      PUBLIC fetch-network
                             fetch-logging
                             fetch-moment
                             fetch-telemetry)

if (_is_clang_compiler)
  target_compile_options(fetch-muddle PRIVATE -Wshadow)
endif ()

# Unit tests
add_subdirectory(tests)
