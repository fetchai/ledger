################################################################################
# F E T C H   M L   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-ml)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-ml)
target_link_libraries(fetch-ml INTERFACE fetch-math)

add_test_target()

<<<<<<< HEAD
#add_subdirectory(benchmark)
=======
add_subdirectory(benchmark)
>>>>>>> 0d9f5a8842a1e8f8cd1cf8fc23164d04ddd5a87f
#add_subdirectory(examples)

