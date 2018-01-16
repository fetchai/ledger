cmake_minimum_required (VERSION 2.8.0)
project (fetch)

OPTION(BUILD_TESTS "Build tests" ON)
OPTION(BUILD_EXAMPLES "Build examples" ON)
OPTION(BUILD_PYTHON_BINDINGS "Build Python examples" ON)

find_package(PNG REQUIRED)
find_package(OpenSSL REQUIRED)

message("OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message("OpenSSL libraries: ${OPENSSL_LIBRARIES}")

link_libraries(crypto)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/vendor/asio/asio/include")
include_directories("${OPENSSL_INCLUDE_DIR}")

add_definitions(-DASIO_STANDALONE -DASIO_HEADER_ONLY -DASIO_HAS_STD_SYSTEM_ERROR)

enable_testing()
exec_program(
    "git"
    ${CMAKE_CURRENT_SOURCE_DIR}
    ARGS "describe"
    OUTPUT_VARIABLE ${GIT_DESCRIBE})

add_definitions( --std=c++11 )
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_definitions( -msse4.2 )
#set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_BUILD_TYPE DEBUG)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g -Wno-unused-function")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-inline -Wno-enum-compare -O3 -DNDEBUG -g -Wall  -Werror -Wno-unused-function")


## Dependencies

include_directories(${PNG_INCLUDE_DIR})

# TESTS
if(BUILD_TESTS)
  add_subdirectory( tests )
endif()

# EXAMPLES
if(BUILD_EXAMPLES)
  add_subdirectory( examples )
endif()

## Main codes
# add_executable(xxx src/xxx.cpp)

# PYTHON
if(BUILD_PYTHON_BINDINGS)
  find_program(PYTHON "python")
  FIND_PACKAGE(PythonLibs)
  set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
  set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
  set(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/pybingen_timestamp")

  configure_file(${SETUP_PY_IN} ${SETUP_PY})

  file( MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/site-packages")
  file( COPY "${CMAKE_CURRENT_SOURCE_DIR}/libfetch" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/site-packages"
  FILES_MATCHING PATTERN "*.py" )  

  set(BINDINGS_DIR "${CMAKE_CURRENT_BINARY_DIR}/site-packages/libfetch")
  set(TEMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/tempfetchcore")  
  
  file(MAKE_DIRECTORY "${TEMP_DIR}")  
  add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND ${PYTHON} ${SETUP_PY} build_ext -b ${BINDINGS_DIR} -t ${TEMP_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
    DEPENDS ${DEPS})

  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

  add_custom_target(python_bindings ALL DEPENDS ${OUTPUT})

  install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install)")
endif()

