cmake_minimum_required(VERSION 3.1)
project(fetch-vectorise)

add_library(fetch-vectorise INTERFACE)

target_sources(fetch-vectorise
  include/vectorise/info_sse.hpp
  include/vectorise/info.hpp
  include/vectorise/memory/array.hpp
  include/vectorise/memory/shared_array.hpp
  include/vectorise/memory/iterator.hpp
  include/vectorise/sse.hpp
  include/vectorise/register.hpp
  include/vectorise/meta/log2.hpp
  include/vectorise/platform.hpp
  include/vectorise/avx.hpp
  include/vectorise/iterator.hpp
  include/vectorise/info_avx.hpp
)

add_executable(vectorise-tests
  tests/memory/shared_array_test.cpp
)

target_link_libraries(vectorise-tests INTERFACE fetch-vectorise)


