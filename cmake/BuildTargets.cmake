macro(fetch_info message)
  string(ASCII 27 ESC)
  message(STATUS "${ESC}[34mInfo${ESC}[0m: ${message}")
endmacro(fetch_info message)

macro(fetch_warning message)
  string(ASCII 27 ESC)
  message(STATUS "${ESC}[31mWARNING${ESC}[0m: ${message}")
endmacro(fetch_warning message)

macro(setup_compiler)

  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

  # ensure that only one architecture is enable
  set(_num_architectures_compiler 0)
  set(_list_architectures_compiler)
  if(FETCH_ARCH_SSE3)
    math(EXPR _num_architectures_compiler "${_num_architectures_compiler}+1")
    list(APPEND _list_architectures_compiler "SS3")
  endif(FETCH_ARCH_SSE3)
  if(FETCH_ARCH_SSE42)
    math(EXPR _num_architectures_compiler "${_num_architectures_compiler}+1")
    list(APPEND _list_architectures_compiler "SS4.2")
  endif(FETCH_ARCH_SSE42)
  if(FETCH_ARCH_AVX)
    math(EXPR _num_architectures_compiler "${_num_architectures_compiler}+1")
    list(APPEND _list_architectures_compiler "AVX")
  endif(FETCH_ARCH_AVX)
  if(FETCH_ARCH_FMA)
    math(EXPR _num_architectures_compiler "${_num_architectures_compiler}+1")
    list(APPEND _list_architectures_compiler "FMA")
  endif(FETCH_ARCH_FMA)
  if(FETCH_ARCH_AVX2)
    math(EXPR _num_architectures_compiler "${_num_architectures_compiler}+1")
    list(APPEND _list_architectures_compiler "AVX2")
  endif(FETCH_ARCH_AVX2)

  # platform configuration
  if (WIN32)
    message(FATAL_ERROR "Windows platform not currently supported")
  elseif (APPLE)
    add_definitions(-DFETCH_PLATFORM_MACOS)
  else () # assume linux flavour
    add_definitions(-DFETCH_PLATFORM_LINUX)
  endif ()

  if(${_num_architectures_compiler} GREATER 1)
    message(SEND_ERROR "Too many architectures configured: ${_list_architectures_compiler}")
  endif()

  # correct update the flags with the target architecture
  set(_compiler_arch "sse3")
  if(FETCH_ARCH_SSE3)
    set(_compiler_arch "sse3")
  elseif(FETCH_ARCH_SSE42)
    set(_compiler_arch "sse4.2")
  elseif(FETCH_ARCH_AVX)
    set(_compiler_arch "avx")
  elseif(FETCH_ARCH_FMA)
    set(_compiler_arch "fma")
  elseif(FETCH_ARCH_AVX2)
    set(_compiler_arch "avx2")
  endif()

  # update actual compiler configuration
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m${_compiler_arch}")

  # warnings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion -Wpedantic")

  #fetch_info("Using compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pragmas")
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  if(FETCH_WARNINGS_AS_ERRORS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-unused-parameter")
  endif(FETCH_WARNINGS_AS_ERRORS)

  # prefer PIC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

  set(_is_clang_compiler FALSE)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(_is_clang_compiler TRUE)
  endif()

  if(FETCH_ENABLE_COVERAGE)
    if (_is_clang_compiler)
      set(CMAKE_CXX_FLAGS_DEBUG "-g -fprofile-instr-generate -fcoverage-mapping -O0")
    else()
      message(SEND_ERROR "Coverage flag enabled but clang compiler not found, CMake will exit.")
    endif()
  endif(FETCH_ENABLE_COVERAGE)

  # debug sanitizer configuration
  string(LENGTH "${FETCH_DEBUG_SANITIZER}" _debug_sanitizer_parameter_length)
  if(${_debug_sanitizer_parameter_length} GREATER 0)
    if (_is_clang_compiler)
      string(REGEX MATCH "(thread|address|undefined)" _debug_sanitizer_valid "${FETCH_DEBUG_SANITIZER}")
      string(LENGTH "${_debug_sanitizer_valid}" _debug_sanitizer_match_length)

      if(${_debug_sanitizer_match_length} GREATER 0)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=${FETCH_DEBUG_SANITIZER}")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=${FETCH_DEBUG_SANITIZER}")
      else()
        message(SEND_ERROR "Incorrect sanitizer configuration: ${FETCH_DEBUG_SANITIZER} Valid choices are thread, address or undefined")
      endif()
    else()
      message(SEND_ERROR "Debug sanitizers are only available for clang based compilers")
    endif()
  endif()

  # add a metric flag if needed
  if (FETCH_ENABLE_METRICS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFETCH_ENABLE_METRICS")
  endif (FETCH_ENABLE_METRICS)

  # allow disabling of colour log file
  if (FETCH_DISABLE_COLOUR_LOG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFETCH_DISABLE_COLOUR_LOG_OUTPUT")
  endif (FETCH_DISABLE_COLOUR_LOG)

  # needed for configuration files etc
  include_directories(${FETCH_ROOT_BINARY_DIR})

  # handle default logging level
  if ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "")
    set (FETCH_COMPILE_LOGGING_LEVEL "info")
  endif()

  # based on the configued logging level
  if ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "debug")
    add_definitions(-DFETCH_COMPILE_LOGGING_LEVEL=4)
  elseif ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "info")
    add_definitions(-DFETCH_COMPILE_LOGGING_LEVEL=3)
  elseif ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "warn")
    add_definitions(-DFETCH_COMPILE_LOGGING_LEVEL=2)
  elseif ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "error")
    add_definitions(-DFETCH_COMPILE_LOGGING_LEVEL=1)
  elseif ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "none")
    add_definitions(-DFETCH_DISABLE_COUT_LOGGING)
    add_definitions(-DFETCH_COMPILE_LOGGING_LEVEL=0)
  else()
    message(SEND_ERROR "Invalid settings for FETCH_COMPILE_LOGGING_LEVEL. Please choose one of: debug,info,warn,error,none")
  endif()

endmacro(setup_compiler)


function(configure_vendor_targets)

  # OpenSSL
  find_package(OpenSSL REQUIRED)
  find_package(Threads)

  if(FETCH_VERBOSE_CMAKE)
    message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")
    if(OPENSSL_USE_STATIC_LIBS)
      message(STATUS "OpenSSL linking: STATICALLY")
    else()
      message(STATUS "OpenSSL linking: DYNAMICALLY")
    endif()
  endif(FETCH_VERBOSE_CMAKE)

  add_library(vendor-openssl INTERFACE)
  target_link_libraries(vendor-openssl INTERFACE ${OPENSSL_LIBRARIES})

  if(OPENSSL_USE_STATIC_LIBS)
    target_link_libraries(vendor-openssl INTERFACE ${CMAKE_DL_LIBS})
    if(CMAKE_THREAD_LIBS_INIT)
      target_link_libraries(vendor-openssl INTERFACE ${CMAKE_THREAD_LIBS_INIT})
    endif(CMAKE_THREAD_LIBS_INIT)
  endif(OPENSSL_USE_STATIC_LIBS)

  target_include_directories(vendor-openssl INTERFACE ${OPENSSL_INCLUDE_DIR})

  # setup the testing
  if(FETCH_ENABLE_TESTS)
    include(CTest)
    enable_testing()
  endif(FETCH_ENABLE_TESTS)

  # asio vendor library
  add_library(vendor-asio INTERFACE)
  target_include_directories(vendor-asio INTERFACE ${FETCH_ROOT_VENDOR_DIR}/asio/asio/include)
  target_compile_definitions(vendor-asio INTERFACE ASIO_STANDALONE ASIO_HEADER_ONLY ASIO_HAS_STD_SYSTEM_ERROR)

  # Pybind11
  add_subdirectory(${FETCH_ROOT_VENDOR_DIR}/pybind11)

  # Google Test
  add_subdirectory(${FETCH_ROOT_VENDOR_DIR}/googletest)

  # Google Benchmark
  # Do not build the google benchmark library tests
  if(FETCH_ENABLE_BENCHMARKS)
      set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppress google benchmark default tests" FORCE)
      add_subdirectory(${FETCH_ROOT_VENDOR_DIR}/benchmark)
  endif(FETCH_ENABLE_BENCHMARKS)

  # mio vendor library
  add_library(vendor-mio INTERFACE)
  target_include_directories(vendor-mio INTERFACE ${FETCH_ROOT_VENDOR_DIR}/mio/include)

  # MsgPack
  add_library(vendor-msgpack INTERFACE)
  target_include_directories(vendor-msgpack INTERFACE ${FETCH_ROOT_VENDOR_DIR}/msgpack/include)
  target_compile_definitions(vendor-msgpack INTERFACE -DMSGPACK_CXX11=ON)

endfunction(configure_vendor_targets)

function(configure_library_targets)

  set(library_root ${FETCH_ROOT_DIR}/libs)

  file(GLOB children RELATIVE ${library_root} ${library_root}/*)
  set(dirlist "")
  foreach(child ${children})
    set(element_path ${library_root}/${child})

    if(IS_DIRECTORY ${element_path})
      set(cmake_path ${element_path}/CMakeLists.txt)
      if(EXISTS ${cmake_path})
        if(FETCH_VERBOSE_CMAKE)
          message(STATUS "Configuring Component: ${child}")
        endif(FETCH_VERBOSE_CMAKE)
        add_subdirectory(${element_path})
      endif()
    endif()
  endforeach()

endfunction(configure_library_targets)

macro(detect_environment)

  set (CMAKE_EXPORT_COMPILE_COMMANDS  ON)

  # detect if we are running in a Jetbrains IDE
  set(FETCH_IS_JETBRAINS_IDE FALSE)
  if ($ENV{JETBRAINS_IDE})
    set(FETCH_IS_JETBRAINS_IDE TRUE)
  endif()

  if (FETCH_ENABLE_CLANG_TIDY)

    if (VERSION LESSER_EQUAL 3.6)
      message( FATAL_ERROR "You need CMake 3.6 to use Clang tidy" )
    endif()

    find_program(
      CLANG_TIDY_EXE
      NAMES "clang-tidy"
      DOC "Path to clang-tidy executable"
    )
    if(NOT CLANG_TIDY_EXE)
      message(STATUS "clang-tidy: not found.")
    elseif(FETCH_IS_JETBRAINS_IDE)
      message(STATUS "clang-tidy: disabled")
    else()
      message(STATUS "clang-tidy: ${CLANG_TIDY_EXE}")

      if (FETCH_WARNINGS_AS_ERRORS)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}" "-checks=file -warnings-as-errors=*")
      else()
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}" "-checks=file")
      endif ()

    endif()

  endif(FETCH_ENABLE_CLANG_TIDY)

endmacro()

function (generate_configuration_file)

  if (DEFINED ENV{FETCH_BUILD_VERSION})

    set (version "$ENV{FETCH_BUILD_VERSION}")
    message (STATUS "Using predefined version: ${version}")

  else()

    # execute git to determine the version
    execute_process(
      COMMAND git describe --dirty=-wip --always
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE version
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  endif()

  # determine the format of the output from the above command
  string(REGEX MATCHALL "^v.*" is_normal_version "${version}")
  if (is_normal_version)

    # cmake regex processing to extract all meaningful elements
    string (REGEX REPLACE "v([0-9]+)\\..*" "\\1" FETCH_VERSION_MAJOR "${version}")
    string (REGEX REPLACE "v[0-9]+\\.([0-9]+)\\..*" "\\1" FETCH_VERSION_MINOR "${version}")
    string (REGEX REPLACE "v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" FETCH_VERSION_PATCH "${version}")
    string (REGEX REPLACE "v[0-9]+\\.[0-9]+\\.[0-9]+-[0-9]+-g([0-9a-f]+).*" "\\1" FETCH_VERSION_COMMIT "${version}")
    string (REGEX REPLACE "v[0-9]+\\.[0-9]+\\.[0-9]+-[0-9]+-g[0-9a-f]+-(wip)" "\\1" FETCH_VERSION_VALID "${version}")

    set (FETCH_VERSION_STR "${version}")

    # handle the error conditions
    if (${FETCH_VERSION_COMMIT} STREQUAL ${version})
      set (FETCH_VERSION_COMMIT "")
    endif ()

    # correct the match
    if (${FETCH_VERSION_VALID} STREQUAL "wip")
      set (FETCH_VERSION_VALID "false")
    else ()
      set (FETCH_VERSION_VALID "true")
    endif ()

  else()
    set (FETCH_VERSION_MAJOR 0)
    set (FETCH_VERSION_MINOR 0)
    set (FETCH_VERSION_PATCH 0)
    set (FETCH_VERSION_COMMIT "${version}")
    set (FETCH_VERSION_VALID "false")
    set (FETCH_VERSION_STR "Unknown version with hash: ${version}")
  endif()

  # generate the version file
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/fetch_version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/fetch_version.hpp
  )

  message(STATUS "Project Version: ${FETCH_VERSION_STR}")

endfunction()
