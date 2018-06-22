################################################################################
# F E T C H   C O R E   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1)
project(fetch-core)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-core)
target_link_libraries(fetch-core PUBLIC fetch-vectorise)