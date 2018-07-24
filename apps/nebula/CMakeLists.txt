project(fetch-nebula)

setup_compiler()

add_executable(nebula
  main.cpp
)
target_link_libraries(nebula PRIVATE fetch-ledger)
#target_include_directories(constellation PRIVATE ${FETCH_ROOT_DIR}/libs/python/include)
