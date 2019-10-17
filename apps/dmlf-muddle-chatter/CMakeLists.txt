#
# F E T C H  C P P  D M L F
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(dmlf-muddle-chatter)

setup_compiler()

add_executable(dmlf-muddle-chatter main.cpp)
target_link_libraries(dmlf-muddle-chatter PRIVATE fetch-version fetch-dmlf)
