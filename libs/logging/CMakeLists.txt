#
# F E T C H   L O G G I N G   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-logging)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-logging)
target_link_libraries(fetch-logging PUBLIC fetch-meta vendor-spdlog vendor-backward-cpp)
