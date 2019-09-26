#
# T X   G E N E R A T O R
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(chain-tool)

setup_compiler()

add_executable(chain-tool main.cpp)
target_link_libraries(chain-tool PRIVATE fetch-ledger)
