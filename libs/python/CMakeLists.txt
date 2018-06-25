################################################################################
# F E T C H   P Y T H O N   B I N D I N G S   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(fetch-python)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

if(FETCH_ENABLE_PYTHON_BINDINGS)

  # define the python module
  pybind11_add_module(fetch-python src/main.cpp)
  target_include_directories(fetch-python PRIVATE
    include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FETCH_ROOT_DIR}/apps/swarm
  )
  target_link_libraries(fetch-python PRIVATE
    fetch-math
    fetch-network
    fetch-http
    vendor-openssl
  )

  # make the name match the bindings
  set_target_properties(fetch-python PROPERTIES OUTPUT_NAME "fetch")

  add_executable(pyfetch src/pyfetch.cpp)
  target_include_directories(pyfetch PRIVATE
    include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FETCH_ROOT_DIR}/apps/swarm
    )
  target_link_libraries(pyfetch PRIVATE
    fetch-math
    fetch-network
    fetch-http
    pybind11::embed
    vendor-openssl
  )

endif(FETCH_ENABLE_PYTHON_BINDINGS)
