#
# F E T C H   O E F - S E A R C H
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-oef-core)

setup_compiler()

add_executable(oef-search MtSearch.cpp MtSearch.hpp main.cpp)
target_link_libraries(oef-search PRIVATE fetch-oef-search fetch-oef-messages)
