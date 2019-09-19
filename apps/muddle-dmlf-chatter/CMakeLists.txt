#
# E T C H
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(muddle-dmlf-chatter)

setup_compiler()

add_executable(muddle-dmlf-chatter main.cpp)
target_link_libraries(muddle-dmlf-chatter PRIVATE fetch-version fetch-dmlf)
