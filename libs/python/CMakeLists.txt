################################################################################
# F E T C H   P Y T H O N   B I N D I N G S   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-python)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

if(FETCH_ENABLE_PYTHON_BINDINGS)

  # define the python module
  pybind11_add_module(fetch-python src/main.cpp)
  target_include_directories(fetch-python PRIVATE include)
  target_link_libraries(fetch-python PRIVATE fetch-math fetch-ml fetch-crypto fetch-ledger fetch-auctions)

  # make the name match the bindings
  set_target_properties(fetch-python PROPERTIES OUTPUT_NAME "fetch")

  add_library(fetch-python-lib INTERFACE)
  target_include_directories(fetch-python-lib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

  # define the python module
  pybind11_add_module(fetch-python-benchmarking src/fetch_benchmarking.cpp)
  target_include_directories(fetch-python-benchmarking PRIVATE include)
  target_link_libraries(fetch-python-benchmarking PRIVATE fetch-ledger)

  # make the name match the bindings
  set_target_properties(fetch-python-benchmarking PROPERTIES OUTPUT_NAME "fetch_benchmarking")

  add_library(fetch-python-benchmarking-lib INTERFACE)
  target_include_directories(fetch-python-benchmarking-lib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

endif(FETCH_ENABLE_PYTHON_BINDINGS)
