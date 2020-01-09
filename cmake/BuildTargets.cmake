macro (fetch_info message)
  string(ASCII 27 ESC)
  message(STATUS "${ESC}[34mInfo${ESC}[0m: ${message}")
endmacro (fetch_info message)

macro (fetch_warning message)
  string(ASCII 27 ESC)
  message(STATUS "${ESC}[31mWARNING${ESC}[0m: ${message}")
endmacro (fetch_warning message)

macro (setup_compiler)

  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

  set(_is_clang_compiler FALSE)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(_is_clang_compiler TRUE)
  endif ()

  # ensure that only one architecture is enable
  set(_num_architectures_compiler 0)
  set(_list_architectures_compiler)
  if (FETCH_ARCH_SSE3)
    math(EXPR _num_architectures_compiler "${_num_architectures_compiler}+1")
    list(APPEND _list_architectures_compiler "SS3")
  endif (FETCH_ARCH_SSE3)
  if (FETCH_ARCH_SSE42)
    math(EXPR _num_architectures_compiler "${_num_architectures_compiler}+1")
    list(APPEND _list_architectures_compiler "SS4.2")
  endif (FETCH_ARCH_SSE42)
  if (FETCH_ARCH_AVX)
    math(EXPR _num_architectures_compiler "${_num_architectures_compiler}+1")
    list(APPEND _list_architectures_compiler "AVX")
  endif (FETCH_ARCH_AVX)
  if (FETCH_ARCH_FMA)
    math(EXPR _num_architectures_compiler "${_num_architectures_compiler}+1")
    list(APPEND _list_architectures_compiler "FMA")
  endif (FETCH_ARCH_FMA)
  if (FETCH_ARCH_AVX2)
    math(EXPR _num_architectures_compiler "${_num_architectures_compiler}+1")
    list(APPEND _list_architectures_compiler "AVX2")
  endif (FETCH_ARCH_AVX2)

  # platform configuration
  if (WIN32)
    message(FATAL_ERROR "Windows platform not currently supported")
  elseif (APPLE)
    add_definitions(-DFETCH_PLATFORM_MACOS)
  else () # assume linux flavour
    add_definitions(-DFETCH_PLATFORM_LINUX)
  endif ()

  if (${_num_architectures_compiler} GREATER 1)
    message(SEND_ERROR "Too many architectures configured: ${_list_architectures_compiler}")
  endif ()

  # correct update the flags with the target architecture
  set(_compiler_arch "sse3")
  if (FETCH_ARCH_SSE3)
    set(_compiler_arch "sse3")
  elseif (FETCH_ARCH_SSE42)
    set(_compiler_arch "sse4.2")
  elseif (FETCH_ARCH_AVX)
    set(_compiler_arch "avx")
  elseif (FETCH_ARCH_FMA)
    set(_compiler_arch "fma")
  elseif (FETCH_ARCH_AVX2)
    set(_compiler_arch "avx2")
  endif ()

  # update actual compiler configuration
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m${_compiler_arch}")

  # warnings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion -Wpedantic")

  if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pragmas -Wno-unknown-pragmas")
  elseif (_is_clang_compiler)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-warning-option -Wshadow")
  endif ()

  # ensuring that Ninja produces color output
  if (FETCH_FORCE_COLORED_OUTPUT)
    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
      add_compile_options(-fdiagnostics-color=always)
    elseif (_is_clang_compiler)
      add_compile_options(-fcolor-diagnostics)
    endif ()
  endif (FETCH_FORCE_COLORED_OUTPUT)

  if (FETCH_WARNINGS_AS_ERRORS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  endif (FETCH_WARNINGS_AS_ERRORS)

  # Temporary workaround for Apple Clang >= 11.0
  if (APPLE AND _is_clang_compiler AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 11.0))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-check")
  endif ()

  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

  if (FETCH_ENABLE_COVERAGE)
    if (_is_clang_compiler)
      set(CMAKE_CXX_FLAGS_DEBUG "-g -fprofile-instr-generate -fcoverage-mapping -O0")
    else ()
      message(SEND_ERROR "Coverage flag enabled but clang compiler not found, CMake will exit.")
    endif ()
  endif (FETCH_ENABLE_COVERAGE)

  # debug sanitizer configuration
  string(LENGTH "${FETCH_DEBUG_SANITIZER}" _debug_sanitizer_parameter_length)
  if (${_debug_sanitizer_parameter_length} GREATER 0)
    if (_is_clang_compiler)
      string(REGEX MATCH
                   "(thread|address|undefined)"
                   _debug_sanitizer_valid
                   "${FETCH_DEBUG_SANITIZER}")
      string(LENGTH "${_debug_sanitizer_valid}" _debug_sanitizer_match_length)

      if (${_debug_sanitizer_match_length} GREATER 0)
        set(CMAKE_CXX_FLAGS_DEBUG
            "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=${FETCH_DEBUG_SANITIZER}")
        set(
          CMAKE_EXE_LINKER_FLAGS_DEBUG
          "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=${FETCH_DEBUG_SANITIZER}"
          )
      else ()
        message(
          SEND_ERROR
            "Incorrect sanitizer configuration: ${FETCH_DEBUG_SANITIZER} Valid choices are thread, address or undefined"
          )
      endif ()
    else ()
      message(SEND_ERROR "Debug sanitizers are only available for clang based compilers")
    endif ()
  endif ()

  # add the backtrace flat
  if (FETCH_ENABLE_BACKTRACE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFETCH_ENABLE_BACKTRACE")

    find_library(DW_LIB dw)

    if (DW_LIB AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFETCH_ENABLE_BACKTRACE_WITH_DW")

      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldw -ldl")
    endif ()
  endif (FETCH_ENABLE_BACKTRACE)

  # allow disabling of colour log file
  if (FETCH_DISABLE_COLOUR_LOG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFETCH_DISABLE_COLOUR_LOG_OUTPUT")
  endif (FETCH_DISABLE_COLOUR_LOG)

  # needed for configuration files etc
  include_directories(${FETCH_ROOT_BINARY_DIR})

  # handle default logging level
  if ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "")
    set(FETCH_COMPILE_LOGGING_LEVEL "info")
  endif ()

  # debug mutex configuration
  if (FETCH_ENABLE_DEADLOCK_DETECTION)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFETCH_DEBUG_MUTEX")
  endif (FETCH_ENABLE_DEADLOCK_DETECTION)

  # based on the configued logging level
  if ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "trace")
    add_definitions(-DFETCH_COMPILE_LOGGING_LEVEL=6)
  elseif ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "debug")
    add_definitions(-DFETCH_COMPILE_LOGGING_LEVEL=5)
  elseif ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "info")
    add_definitions(-DFETCH_COMPILE_LOGGING_LEVEL=4)
  elseif ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "warn")
    add_definitions(-DFETCH_COMPILE_LOGGING_LEVEL=3)
  elseif ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "error")
    add_definitions(-DFETCH_COMPILE_LOGGING_LEVEL=2)
  elseif ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "critical")
    add_definitions(-DFETCH_COMPILE_LOGGING_LEVEL=1)
  elseif ("${FETCH_COMPILE_LOGGING_LEVEL}" STREQUAL "none")
    add_definitions(-DFETCH_DISABLE_COUT_LOGGING)
    add_definitions(-DFETCH_COMPILE_LOGGING_LEVEL=0)
  else ()
    message(
      SEND_ERROR
        "Invalid settings for FETCH_COMPILE_LOGGING_LEVEL. Please choose one of: debug,info,warn,error,none"
      )
  endif ()

  if (FETCH_FIXEDPOINT_DEBUG_HEX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFETCH_FIXEDPOINT_DEBUG_HEX")
  endif ()

endmacro (setup_compiler)

function (conditionally_enable_lto)
  if (FETCH_ENABLE_LTO)

    include(CheckIPOSupported)
    check_ipo_supported(RESULT
                        result
                        OUTPUT
                        output)

    if (result)

      fetch_info("[LTO] Link-time optimisation is supported")

      if (CMAKE_BUILD_TYPE STREQUAL Debug)
        fetch_info("[LTO] Debug build: link-time optimisation disabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
      else ()
        fetch_info("[LTO] Link-time optimisation enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
      endif ()

    else ()

      message(
        FATAL_ERROR
          "[LTO] CMake does not support link-time optimisation for this compiler. To proceed, explicitly disable the FETCH_ENABLE_LTO option"
        )

    endif ()

  endif ()
endfunction ()

function (configure_vendor_targets)

  find_package(Threads)

  # setup the testing
  if (FETCH_ENABLE_TESTS)
    include(CTest)
    enable_testing()
  endif (FETCH_ENABLE_TESTS)

  # memu vendor library
  add_library(vendor-memu INTERFACE)
  target_include_directories(vendor-memu INTERFACE ${FETCH_ROOT_VENDOR_DIR}/memu)

  # asio vendor library
  add_library(vendor-asio INTERFACE)
  target_include_directories(vendor-asio INTERFACE ${FETCH_ROOT_VENDOR_DIR}/asio/asio/include)
  target_compile_definitions(vendor-asio
                             INTERFACE
                             ASIO_STANDALONE
                             ASIO_HEADER_ONLY
                             ASIO_HAS_STD_SYSTEM_ERROR)

  # required for latest version of Xcode: Apple LLVM version 10.0.1 (clang-1001.0.46.3). In this
  # version the string view has been removed from: <experimental/string_view>. This will need to be
  # set across all compilers when C++17 support is standard.
  if (APPLE)
    target_compile_definitions(vendor-asio INTERFACE ASIO_HAS_STD_STRING_VIEW)
  endif (APPLE)

  # Local version of OpenSSL
  add_subdirectory(${FETCH_ROOT_VENDOR_DIR}/openssl)
  add_library(vendor-openssl INTERFACE)
  message(STATUS "OpenSSL include ${CMAKE_BINARY_DIR}/vendor/openssl/include")
  target_link_libraries(vendor-openssl INTERFACE ssl crypto)
  target_include_directories(vendor-openssl INTERFACE ${CMAKE_BINARY_DIR}/vendor/openssl/include)

  # Pybind11
  add_subdirectory(${FETCH_ROOT_VENDOR_DIR}/pybind11)

  # Google Test
  add_subdirectory(${FETCH_ROOT_VENDOR_DIR}/googletest)

  # MCL TODO: Work out how to get this to work with the already found version of OpenSSL
  set(USE_GMP OFF CACHE BOOL "use gmp" FORCE)
  set(USE_OPENSSL OFF CACHE BOOL "use openssl" FORCE)
  set(ONLY_LIB ON CACHE BOOL "only lib" FORCE)
  add_subdirectory(${FETCH_ROOT_VENDOR_DIR}/mcl)
  target_include_directories(mcl_st INTERFACE ${FETCH_ROOT_VENDOR_DIR}/mcl/include)
  target_compile_definitions(mcl_st
                             INTERFACE
                             -DMCL_USE_VINT
                             -DMCL_VINT_FIXED_BUFFER
                             -DMCLBN_FP_UNIT_SIZE=4)

  add_library(vendor-mcl INTERFACE)
  target_link_libraries(vendor-mcl INTERFACE mcl_st)

  # Google Benchmark Do not build the google benchmark library tests
  if (FETCH_ENABLE_BENCHMARKS)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppress google benchmark default tests" FORCE)
    add_subdirectory(${FETCH_ROOT_VENDOR_DIR}/benchmark)
  endif (FETCH_ENABLE_BENCHMARKS)

  # mio vendor library
  add_library(vendor-mio INTERFACE)
  target_include_directories(vendor-mio INTERFACE ${FETCH_ROOT_VENDOR_DIR}/mio/include)

  # backtrace stack vendor library
  add_library(vendor-backward-cpp INTERFACE)
  target_include_directories(vendor-backward-cpp INTERFACE ${FETCH_ROOT_VENDOR_DIR}/backward-cpp/)

  # MsgPack
  add_library(vendor-msgpack INTERFACE)
  target_include_directories(vendor-msgpack INTERFACE ${FETCH_ROOT_VENDOR_DIR}/msgpack/include)
  target_compile_definitions(vendor-msgpack INTERFACE -DMSGPACK_CXX11=ON)

  # Spdlog
  add_library(vendor-spdlog INTERFACE)
  target_include_directories(vendor-spdlog INTERFACE ${FETCH_ROOT_VENDOR_DIR}/spdlog/include)

  # utfcpp
  set(UTF8_TESTS OFF CACHE BOOL "Enable tests for UTF8-CPP" FORCE)
  set(UTF8_INSTALL OFF CACHE BOOL "Enable installation for UTF8-CPP" FORCE)
  set(UTF8_SAMPLES OFF CACHE BOOL "Enable building samples for UTF8-CPP" FORCE)
  add_subdirectory(${FETCH_ROOT_VENDOR_DIR}/utfcpp)

  # noisec
  set(USE_SODIUM OFF CACHE BOOL "Use Libsodium for crypto" FORCE)
  set(EXTERNAL_OPENSSL vendor-openssl CACHE STRING "Use vendor-openssl in NoiseC" FORCE)
  add_subdirectory(${FETCH_ROOT_VENDOR_DIR}/noisec)
  add_library(vendor-noisec INTERFACE)
  target_link_libraries(vendor-noisec INTERFACE noise_protocol)
  target_include_directories(vendor-noisec INTERFACE ${FETCH_ROOT_VENDOR_DIR}/noisec/include)

endfunction (configure_vendor_targets)

function (configure_library_targets)

  set(library_root ${FETCH_ROOT_DIR}/libs)

  file(GLOB children RELATIVE ${library_root} ${library_root}/*)

  if (APPLE)
    # Temporarily disable oef builds
    list(FILTER
         children
         EXCLUDE
         REGEX
         ".*oef-core.*")
  endif (APPLE)

  set(dirlist "")
  foreach (child ${children})
    set(element_path ${library_root}/${child})

    if (IS_DIRECTORY ${element_path})
      set(cmake_path ${element_path}/CMakeLists.txt)
      if (EXISTS ${cmake_path})
        if (FETCH_VERBOSE_CMAKE)
          message(STATUS "Configuring Component: ${child}")
        endif (FETCH_VERBOSE_CMAKE)
        add_subdirectory(${element_path})
      endif ()
    endif ()
  endforeach ()

endfunction (configure_library_targets)

macro (detect_environment)

  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

  # detect if we are running in a Jetbrains IDE
  set(FETCH_IS_JETBRAINS_IDE FALSE)
  if ($ENV{JETBRAINS_IDE})
    set(FETCH_IS_JETBRAINS_IDE TRUE)
  endif ()

  if (FETCH_ENABLE_CLANG_TIDY)

    if (CMAKE_VERSION VERSION_LESS_EQUAL 3.6)
      message(FATAL_ERROR "You need CMake 3.6 to use clang-tidy")
    endif ()

    find_program(CLANG_TIDY_EXE NAMES "clang-tidy" DOC "Path to clang-tidy executable")
    if (${CLANG_TIDY_EXE} STREQUAL CLANG_TIDY_EXE-NOTFOUND)
      message(STATUS "clang-tidy: not found.")
    elseif (FETCH_IS_JETBRAINS_IDE)
      message(STATUS "clang-tidy: disabled")
    else ()
      message(STATUS "clang-tidy: ${CLANG_TIDY_EXE}")

      if (FETCH_WARNINGS_AS_ERRORS)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}" "-checks=file -warnings-as-errors=*")
      else ()
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}" "-checks=file")
      endif ()

    endif ()

  endif (FETCH_ENABLE_CLANG_TIDY)

endmacro ()

function (generate_version_file)

  if (DEFINED ENV{FETCH_BUILD_VERSION})

    set(version "$ENV{FETCH_BUILD_VERSION}")
    message(STATUS "Using predefined version: ${version}")

  else ()

    # execute git to determine the version
    execute_process(COMMAND git describe --dirty=-wip  --always
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    OUTPUT_VARIABLE version
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

  endif ()

  # determine the format of the output from the above command
  string(REGEX MATCHALL
               "^v.*"
               is_normal_version
               "${version}")
  if (is_normal_version)

    # cmake regex processing to extract all meaningful elements
    string(REGEX
           REPLACE "v([0-9]+)\\..*"
                   "\\1"
                   FETCH_VERSION_MAJOR
                   "${version}")
    string(REGEX
           REPLACE "v[0-9]+\\.([0-9]+)\\..*"
                   "\\1"
                   FETCH_VERSION_MINOR
                   "${version}")
    string(REGEX
           REPLACE "v[0-9]+\\.[0-9]+\\.([0-9]+).*"
                   "\\1"
                   FETCH_VERSION_PATCH
                   "${version}")
    string(REGEX
           REPLACE "v[0-9]+\\.[0-9]+\\.[0-9]+-[0-9]+-g([0-9a-f]+).*"
                   "\\1"
                   FETCH_VERSION_COMMIT
                   "${version}")
    string(REGEX
           REPLACE "v[0-9]+\\.[0-9]+\\.[0-9]+-[0-9]+-g[0-9a-f]+-(wip)"
                   "\\1"
                   FETCH_VERSION_VALID
                   "${version}")

    set(FETCH_VERSION_STR "${version}")

    # handle the error conditions
    if (${FETCH_VERSION_COMMIT} STREQUAL ${version})
      set(FETCH_VERSION_COMMIT "")
    endif ()

    # correct the match
    if (${FETCH_VERSION_VALID} STREQUAL "wip")
      set(FETCH_VERSION_VALID "false")
    else ()
      set(FETCH_VERSION_VALID "true")
    endif ()

  else ()
    set(FETCH_VERSION_MAJOR 0)
    set(FETCH_VERSION_MINOR 0)
    set(FETCH_VERSION_PATCH 0)
    set(FETCH_VERSION_COMMIT "${version}")
    set(FETCH_VERSION_VALID "false")
    set(FETCH_VERSION_STR "Unknown version with hash: ${version}")
  endif ()

  # generate the version file
  configure_file(${CMAKE_SOURCE_DIR}/cmake/fetch_version.cpp.in
                 ${CMAKE_BINARY_DIR}/libs/version/src/fetch_version.cpp)

  message(STATUS "Project Version: ${FETCH_VERSION_STR}")

endfunction ()
