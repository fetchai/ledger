################################################################################
# F E T C H   M A T H   E X A M P L E S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-math)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

setup_library_examples(fetch-math)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)

if (FETCH_ENABLE_EXAMPLES)
  add_executable(perfom-test perform_test/main.cpp)
  target_link_libraries(perfom-test PUBLIC fetch-math Eigen3::Eigen)
endif(FETCH_ENABLE_EXAMPLES)
