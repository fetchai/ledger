cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
project(pyfetch)

# Compiler Configuration
setup_compiler()

file(GLOB_RECURSE headers *.hpp)
file(GLOB_RECURSE srcs *.cpp)
add_executable(pyfetch ${headers} pyfetch.cpp)
target_link_libraries(pyfetch PRIVATE fetch-http fetch-network fetch-ledger fetch-python-lib pybind11::embed fetch-network vendor-openssl)
target_include_directories(pyfetch PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(pyfetch PRIVATE "-fvisibility=hidden")