cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
add_executable(bloom-tool main.cpp)
target_link_libraries(bloom-tool PRIVATE fetch-bloomfilter)
