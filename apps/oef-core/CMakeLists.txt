#
# F E T C H   C O N S T E L L A T I O N
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-oef-core)

setup_compiler()

add_executable(oef-core
        main.hpp
        main.cpp
        MtCore.hpp
        MtCore.cpp)
target_link_libraries(oef-core
                      PRIVATE fetch-oef-core
                              fetch-oef-messages)

#target_include_directories(constellation PRIVATE ${FETCH_ROOT_DIR}/libs/python/include)
