cmake_minimum_required(VERSION 3.1)
project(fetch-core)


file(GLOB_RECURSE SRCS include/*.hpp src/*.cpp)

add_library(fetch-core ${SRCS})
