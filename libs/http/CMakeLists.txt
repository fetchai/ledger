################################################################################
# F E T C H   H T T P   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1)
project(fetch-http)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()



#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-http)
target_link_libraries(fetch-http PUBLIC fetch-network)

#-------------------------------------------------------------------------------
# Example Targets
#-------------------------------------------------------------------------------

setup_library_examples(fetch-http)

#-------------------------------------------------------------------------------
# Benchmark Targets
#-------------------------------------------------------------------------------

add_executable(benchmark_json_document benchmark/json/document.cpp)
target_link_libraries(benchmark_json_document PRIVATE fetch-http fetch-testing)

add_fetch_test(json_document_test fetch-http tests/json/document.cpp)
add_fetch_test(variant_test fetch-http tests/script/variant.cpp)

# http-server
#add_executable(example-http-server examples/http/server.cpp)
#target_link_libraries(example-http-server PRIVATE fetch-http)

#cmake_minimum_required (VERSION 2.8.0)
#project (fetch)
#
#OPTION(BUILD_TESTS "Build tests" ON)
#OPTION(BUILD_EXAMPLES "Build examples" ON)
#OPTION(BUILD_BENCHMARK "Build benchmark" ON)
#
### Check feature set on Apple with
## sysctl -a | grep machdep.cpu.features
#OPTION(BUILD_DEBUG_MODE "Debug mode" OFF)
#
#OPTION(BUILD_MACH_SSE3 "Enable SSE3" ON)
#OPTION(BUILD_MACH_SSE42 "Enable SSE4.2" ON)
#OPTION(BUILD_MACH_AVX "Enable AVX" OFF)
#OPTION(BUILD_MACH_FMA "Enable FMA" OFF)
#OPTION(BUILD_MACH_AVX2 "Enable AVX2" OFF)
#
#OPTION(BUILD_STRICT_COMPILE_FLAGS "Use strict compile flags" ON)
#
#find_package(Threads REQUIRED)
#find_package(OpenSSL REQUIRED)
#
#message("OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
#message("OpenSSL libraries: ${OPENSSL_LIBRARIES}")
#
#include_directories("${OPENSSL_INCLUDE_DIR}")
#
#add_definitions( --std=c++11 )
#
#if(BUILD_MACH_SSE3)
#add_definitions( -msse3 )
#endif()
#
#if(BUILD_MACH_SSE42)
#add_definitions( -msse4.2 )
#endif()
#
#if(BUILD_MACH_AVX)
#add_definitions( -mavx )
#endif()
#
#if(BUILD_MACH_AVX2)
#add_definitions( -mavx2 )
#endif()
#
#if(BUILD_MACH_FMA)
#add_definitions( -mfma )
#endif()
#
#
######################################################
## Compile flags
#set(STRICT_FLAGS "-Wconversion -Wpedantic -Wall -Werror")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wno-unused-function -Wno-unknown-pragmas -O3 ${STRICT_FLAGS}")
#
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g -Wno-unused-function -Wno-unknown-pragmas -O3 ${STRICT_FLAGS}")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-inline -Wno-enum-compare -Wno-unknown-pragmas -O3 ${STRICT_FLAGS} -DNDEBUG -DFETCH_DISABLE_COUT_LOGGING")
#
##set(CMAKE_BUILD_TYPE DEBUG)
#set(CMAKE_BUILD_TYPE RELEASE)
#
######################################################
## ASIO specific options
#add_definitions(-DASIO_STANDALONE -DASIO_HEADER_ONLY -DASIO_HAS_STD_SYSTEM_ERROR)
#
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/vendor/asio/asio/include")
#
######################################################
## Includes
#set(FETCH_CORE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/include")
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
#
######################################################
## Fetch library
#add_library(libfetch_logger STATIC src/logger.cpp)
#add_library(libfetch_vt100 STATIC src/commandline/vt100.cpp)
#add_library(libfetch_variant STATIC src/script/variant.cpp)
#
#add_library(libfetch_bytearray_encoders STATIC src/byte_array/encoders.cpp)
#add_library(libfetch_bytearray_decoders STATIC src/byte_array/decoders.cpp)
#add_library(libfetch_bytearray_detail_encode_decode STATIC src/byte_array/details/encode_decode.cpp)
#add_library(libfetch_http_mime_types STATIC src/http/mime_types.cpp)
#
#add_library(libfetch STATIC src/libfetch.cpp)
#target_link_libraries(libfetch libfetch_logger libfetch_vt100 libfetch_variant libfetch_bytearray_encoders libfetch_bytearray_decoders libfetch_bytearray_detail_encode_decode libfetch_http_mime_types )
#
#link_libraries(pthread libfetch crypto)
#
### TESTS
##if(BUILD_TESTS)
##  add_subdirectory( tests )
##endif()
#
### BENCHMARK
##if(BUILD_EXAMPLES)
##  add_subdirectory( benchmark )
##endif()
#
## EXAMPLES
#if(BUILD_EXAMPLES)
#  add_subdirectory( examples )
#endif()
