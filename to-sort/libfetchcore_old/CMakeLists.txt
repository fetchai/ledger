cmake_minimum_required(VERSION 2.8.12)
project(libfetch)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(vendor/pybind11)
pybind11_add_module(libfetchcore src/main.cpp)
