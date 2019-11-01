#
# F E T C H   C O N S T E L L A T I O N
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-oef-core)

setup_compiler()

add_executable(oef-core MtCore.cpp MtCore.hpp main.cpp)
target_link_libraries(oef-core PRIVATE fetch-oef-core fetch-oef-messages)
