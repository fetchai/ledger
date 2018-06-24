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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion -Wpedantic")
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

endmacro(setup_compiler)

function(configure_vendor_targets)

  #find_package(PNG REQUIRED)
  #set(THREADS_PREFER_PTHREAD_FLAG ON)
  #find_package(Threads REQUIRED)

  find_package(OpenSSL REQUIRED)

  message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
  message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")

  # OpenSSL
  add_library(vendor-openssl INTERFACE)
  target_link_libraries(vendor-openssl INTERFACE ${OPENSSL_LIBRARIES})
  target_include_directories(vendor-openssl INTERFACE ${OPENSSL_INCLUDE_DIR})

  # setup the testing
  if(FETCH_ENABLE_TESTS)
    include(ctest)
    enable_testing()
  endif(FETCH_ENABLE_TESTS)

  # asio vendor library
  add_library(vendor-asio INTERFACE)
  target_include_directories(vendor-asio INTERFACE ${FETCH_ROOT_VENDOR_DIR}/asio/asio/include)
  target_compile_definitions(vendor-asio INTERFACE ASIO_STANDALONE ASIO_HEADER_ONLY ASIO_HAS_STD_SYSTEM_ERROR)

  # Pybind11
  add_subdirectory(${FETCH_ROOT_VENDOR_DIR}/pybind11)

endfunction(configure_vendor_targets)

function(configure_library_targets)

  set(library_root ${FETCH_ROOT_DIR}/libs)

  #  add_subdirectory(${library_root}/vectorise)
  #  add_subdirectory(${library_root}/core)

  file(GLOB children RELATIVE ${library_root} ${library_root}/*)
  set(dirlist "")
  foreach(child ${children})
    set(element_path ${library_root}/${child})

    if(IS_DIRECTORY ${element_path})
      set(cmake_path ${element_path}/CMakeLists.txt)
      if(EXISTS ${cmake_path})
        message(STATUS "Configuring Component: ${child}")
        add_subdirectory(${element_path})
      endif()
    endif()
  endforeach()

endfunction(configure_library_targets)
