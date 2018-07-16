macro(fetch_info message)
  string(ASCII 27 ESC)
  message(STATUS "${ESC}[34mInfo${ESC}[0m: ${message}")
endmacro(fetch_info message)

macro(fetch_warning message)
  string(ASCII 27 ESC)
  message(STATUS "${ESC}[31mWARNING${ESC}[0m: ${message}")
endmacro(fetch_warning message)

macro(setup_compiler)

  set(CMAKE_CXX_STANDARD 11)
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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -Wextra -Wconversion -Wpedantic")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
  if(FETCH_WARNINGS_AS_ERRORS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  endif(FETCH_WARNINGS_AS_ERRORS)

  # prefer PIC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

  # fetch logging defines
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -DFETCH_DISABLE_COUT_LOGGING")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DFETCH_DISABLE_COUT_LOGGING")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DFETCH_DISABLE_COUT_LOGGING")

  # debug sanitizer configuration
  string(LENGTH "${FETCH_DEBUG_SANITIZER}" _debug_sanitizer_parameter_length)
  if(${_debug_sanitizer_parameter_length} GREATER 0)
    string(REGEX MATCH "(thread|address)" _debug_sanitizer_valid "${FETCH_DEBUG_SANITIZER}")
    string(LENGTH "${_debug_sanitizer_valid}" _debug_sanitizer_match_length)

    if(${_debug_sanitizer_match_length} GREATER 0)
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=${FETCH_DEBUG_SANITIZER}")
      set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=${FETCH_DEBUG_SANITIZER}")
    else()
      message(SEND_ERROR "Incorrect sanitizer configuration: ${FETCH_DEBUG_SANITIZER} Valid choices are thread or address")
    endif()
  endif()

endmacro(setup_compiler)

function(configure_vendor_targets)

  #find_package(PNG REQUIRED)
  #set(THREADS_PREFER_PTHREAD_FLAG ON)
  #find_package(Threads REQUIRED)

  find_package(OpenSSL REQUIRED)

  if(FETCH_VERBOSE_CMAKE)
    message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")
  endif(FETCH_VERBOSE_CMAKE)

  # OpenSSL
  add_library(vendor-openssl INTERFACE)
  target_link_libraries(vendor-openssl INTERFACE ${OPENSSL_LIBRARIES})
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
