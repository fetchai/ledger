#
# F E T C H   T E L E M E T R Y   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-telemetry)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-telemetry)
target_link_libraries(fetch-telemetry)

if (_is_clang_compiler)
  target_compile_options(fetch-telemetry PRIVATE -Wshadow)
endif ()

# ------------------------------------------------------------------------------
# Test Targets
# ------------------------------------------------------------------------------

add_test_target()
