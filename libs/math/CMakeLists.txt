cmake_minimum_required(VERSION 3.1)
project(fetch-math)

include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

setup_compiler()

setup_library(fetch-math)


### OLD

#
#cmake_minimum_required (VERSION 2.8.0)
#project (fetch-vectorisation)
#
##include_directories("${CMAKE_CURRENT_SOURCE_DIR}/core/vectorisation/include")
##include_directories("${CMAKE_CURRENT_SOURCE_DIR}/core/include")
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
#
#
#if(NOT FETCH_CORE_LIB)
#  add_subdirectory(core)
#endif()
#
#
#OPTION(BUILD_MACH_SSE3 "Enable SSE3" ON)
#OPTION(BUILD_MACH_SSE42 "Enable SSE4.2" ON)
#OPTION(BUILD_MACH_AVX "Enable AVX" OFF)
#OPTION(BUILD_MACH_FMA "Enable FMA" OFF)
#OPTION(BUILD_MACH_AVX2 "Enable AVX2" OFF)
#
#OPTION(BUILD_STICT_COMPILE_FLAGS "Use strict compile flags" ON)
#
#set(THREADS_PREFER_PTHREAD_FLAG ON)
#link_libraries(crypto pthread)
#
#enable_testing()
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
#
### Making compile flags strict for the rest of the library
#if(BUILD_STICT_COMPILE_FLAGS)
#  set(STRICT_FLAGS "-Wconversion -Wpedantic -Wall -Werror")
#else()
#  set(STRICT_FLAGS "")
#endif()
#
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wno-unused-function -Wno-unknown-pragmas -O3 ${STRICT_FLAGS}")
#
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g -Wno-unused-function -Wno-unknown-pragmas -O3 ${STRICT_FLAGS}")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-inline -Wno-enum-compare -Wno-unknown-pragmas -O3 ${STRICT_FLAGS} -DNDEBUG -DFETCH_DISABLE_COUT_LOGGING")
#
### Use BUILD_DEBUG_MODE to change mode
#if( BUILD_DEBUG_MODE )
#    message(STATUS "Building in DEBUG MODE")
#    set(CMAKE_BUILD_TYPE DEBUG)
#else()
#    message(STATUS "Building in RELEASE MODE")
#    set(CMAKE_BUILD_TYPE RELEASE)
#endif()
#
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
#
#add_subdirectory( tests )


