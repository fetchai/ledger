cmake_minimum_required(VERSION 3.1)
project(fetch-core)


include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

setup_compiler()

setup_library(fetch-core)
target_link_libraries(fetch-core PUBLIC fetch-vectorise)