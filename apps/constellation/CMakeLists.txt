#
# F E T C H   C O N S T E L L A T I O N
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-constellation)

setup_compiler()

add_executable(constellation
               bootstrap_monitor.cpp
               bootstrap_monitor.hpp
               config_builder.cpp
               config_builder.hpp
               main.cpp
               manifest_builder.cpp
               manifest_builder.hpp
               settings.cpp
               settings.hpp)
target_link_libraries(constellation
                      PRIVATE fetch-ledger
                              fetch-constellation
                              fetch-http
                              fetch-beacon
                              fetch-settings
                              fetch-version)
target_include_directories(constellation PRIVATE ${FETCH_ROOT_DIR}/libs/python/include)
