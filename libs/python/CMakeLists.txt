#cmake_minimum_required(VERSION 2.8.12)
#project(fetch-python)
#
#
##include_directories("${CMAKE_CURRENT_SOURCE_DIR}/math/core/vectorisation/include")
##include_directories("${CMAKE_CURRENT_SOURCE_DIR}/math/core/include")
##include_directories("${CMAKE_CURRENT_SOURCE_DIR}/math/include")
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
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
#if(NOT FETCH_CORE_LIB)
#  add_subdirectory(math/core)
#endif()
#
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
#
#
#add_subdirectory(vendor/pybind11)
#pybind11_add_module(fetch src/main.cpp)
#target_link_libraries(fetch PRIVATE libfetchcore)
#
#if(BUILD_PYTHON_BINDINGS)
#  find_program(PYTHON "python")
#  FIND_PACKAGE(PythonLibs)
#  set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
#  set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
#  set(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/pybingen_timestamp")
#
#  configure_file(${SETUP_PY_IN} ${SETUP_PY})
#
#  file( MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/site-packages")
#  file( COPY "${CMAKE_CURRENT_SOURCE_DIR}/libfetch" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/site-packages"
#  FILES_MATCHING PATTERN "*.py" )
#
#  set(BINDINGS_DIR "${CMAKE_CURRENT_BINARY_DIR}/site-packages/libfetch")
#  set(TEMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/tempfetchcore")
#
#  file(MAKE_DIRECTORY "${TEMP_DIR}")
#  add_custom_command(
#    OUTPUT ${OUTPUT}
#    COMMAND ${PYTHON} ${SETUP_PY} build_ext -b ${BINDINGS_DIR} -t ${TEMP_DIR}
#    COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
#    DEPENDS ${DEPS})
#
#  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
#  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
#
#  add_custom_target(python_bindings ALL DEPENDS ${OUTPUT})
#
#  install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install)")
#endif()