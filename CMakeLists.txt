cmake_minimum_required (VERSION 2.8.0)
project (fetch)

OPTION(BUILD_TESTS "Build tests" ON)
OPTION(BUILD_EXAMPLES "Build examples" ON)
OPTION(BUILD_BENCHMARK "Build benchmark" ON)
OPTION(BUILD_PYTHON_BINDINGS "Build Python examples" ON)


## Check feature set on Apple with
# sysctl -a | grep machdep.cpu.features

OPTION(BUILD_MACH_SSE3 "Enable SSE3" ON)
OPTION(BUILD_MACH_SSE42 "Enable SSE4.2" ON)
OPTION(BUILD_MACH_AVX "Enable AVX" OFF)
OPTION(BUILD_MACH_FMA "Enable FMA" OFF)
OPTION(BUILD_MACH_AVX2 "Enable AVX2" OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(PNG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

message("OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message("OpenSSL libraries: ${OPENSSL_LIBRARIES}")


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/vendor/asio/asio/include")
include_directories("${OPENSSL_INCLUDE_DIR}")

add_definitions(-DASIO_STANDALONE -DASIO_HEADER_ONLY -DASIO_HAS_STD_SYSTEM_ERROR)

enable_testing()
#exec_program(
#    "git"
#    ${CMAKE_CURRENT_SOURCE_DIR}
#    ARGS "describe"
#    OUTPUT_VARIABLE ${GIT_DESCRIBE})


add_definitions( --std=c++11 )

if(BUILD_MACH_SSE3)
add_definitions( -msse3 ) 
endif()

if(BUILD_MACH_SSE42)
add_definitions( -msse4.2 ) 
endif()

if(BUILD_MACH_AVX)
add_definitions( -mavx ) 
endif()

if(BUILD_MACH_AVX2)
add_definitions( -mavx2 ) 
endif()

if(BUILD_MACH_FMA)
add_definitions( -mfma ) 
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/RemoveFlag.cmake")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
#
set(STRICT_FLAGS "-Wconversion -Wpedantic -Werror -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g -Wno-unused-function -Wno-unknown-pragmas -O3 ${STRICT_FLAGS}")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-inline -Wno-enum-compare -Wno-unknown-pragmas -O3 ${STRICT_FLAGS} -DNDEBUG -DFETCH_DISABLE_COUT_LOGGING")

#set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_BUILD_TYPE RELEASE)

include_directories(${PNG_INCLUDE_DIR})

if(BUILD_PYTHON_BINDINGS)
add_subdirectory( libfetchcore/ )
endif()


## Making compile flags strict for the rest of the library
set(STRICT_FLAGS "-Wconversion -Wpedantic -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wno-unused-function -Wno-unknown-pragmas -O3 ${STRICT_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")


link_libraries(crypto pthread)

#### LIB FETCH
add_library(libfetch_logger STATIC src/logger.cpp)
add_library(libfetch_vt100 STATIC src/commandline/vt100.cpp)
add_library(libfetch_variant STATIC src/script/variant.cpp)

add_library(libfetch_bytearray_encoders STATIC src/byte_array/encoders.cpp)
add_library(libfetch_bytearray_decoders STATIC src/byte_array/decoders.cpp)
add_library(libfetch_bytearray_detail_encode_decode STATIC src/byte_array/details/encode_decode.cpp)
add_library(libfetch_http_mime_types STATIC src/http/mime_types.cpp)


add_library(libfetch STATIC src/libfetch.cpp)
target_link_libraries(libfetch libfetch_logger libfetch_vt100 libfetch_variant libfetch_bytearray_encoders libfetch_bytearray_decoders libfetch_bytearray_detail_encode_decode libfetch_http_mime_types )



link_libraries(crypto pthread libfetch)

## Dependencies

# TESTS
if(BUILD_TESTS)
  add_subdirectory( tests )
endif()

# BENCHMARK
if(BUILD_EXAMPLES)
  add_subdirectory( benchmark )
endif()


# EXAMPLES
if(BUILD_EXAMPLES)
  add_subdirectory( examples )
endif()

## Main codes
# add_executable(xxx src/xxx.cpp)

# PYTHON

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

