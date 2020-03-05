#
# F E T C H   B L O O M   F I L T E R   L I B R A R Y
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-bloom-filter)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Main Library Target
# ------------------------------------------------------------------------------

setup_library(fetch-bloom-filter)
target_link_libraries(fetch-bloom-filter PUBLIC fetch-core fetch-crypto fetch-logging)

add_test_target()
