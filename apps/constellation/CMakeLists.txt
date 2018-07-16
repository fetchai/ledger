project(fetch-constellation)

setup_compiler()

add_executable(constellation
  main.cpp
  constellation.cpp
  constellation.hpp
)
target_link_libraries(constellation PRIVATE fetch-ledger)
target_include_directories(constellation PRIVATE ${FETCH_ROOT_DIR}/libs/python/include)
