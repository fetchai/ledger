#
# F E T C H   C O N S T E L L A T I O N
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-constellation)

setup_compiler()

add_executable(constellation
               bootstrap_monitor.cpp
               bootstrap_monitor.hpp
               config_builder.cpp
               config_builder.hpp
               constellation.cpp
               constellation.hpp
               open_api_http_module.hpp               
               open_api_http_module.cpp               
               health_check_http_module.hpp
               key_generator.cpp
               key_generator.hpp
               logging_http_module.hpp
               main.cpp
               manifest_builder.cpp
               manifest_builder.hpp
               settings.cpp
               settings.hpp
               telemetry_http_module.hpp)
target_link_libraries(constellation
                      PRIVATE fetch-ledger
                              fetch-miner
                              fetch-http
                              fetch-dkg
                              fetch-settings
                              fetch-version)
target_include_directories(constellation PRIVATE ${FETCH_ROOT_DIR}/libs/python/include)
