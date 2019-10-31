#
# CHAIN TOOL
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(chain-tool)

setup_compiler()

add_executable(chain-tool main.cpp)
target_link_libraries(chain-tool PRIVATE fetch-ledger)
