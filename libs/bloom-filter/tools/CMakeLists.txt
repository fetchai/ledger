#
# F E T C H   B L O O M   F I L T E R   T O O L S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-bloomfilter)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

# ------------------------------------------------------------------------------
# Bloom Tool Target
# ------------------------------------------------------------------------------

add_executable(bloom-tool main.cpp)
target_link_libraries(bloom-tool PRIVATE fetch-bloomfilter)
