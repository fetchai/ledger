cmake_minimum_required(VERSION 2.8.12)
project(libfetch)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(vendor/pybind11)
pybind11_add_module(libfetchcore src/main.cpp)
set_target_properties(libfetchcore
                      PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_RELEASE} -fPIC -Wno-inline -Wno-enum-compare -Wno-unknown-pragmas -O3  -DNDEBUG")
