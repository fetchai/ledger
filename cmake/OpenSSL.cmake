function(_build_and_install_openssl openssl_install_dir openssl_vendor_dir)
  set(FETCH_MINIMUM_PERL_VERSION 5.13.4)

  find_package(Perl REQUIRED)
  if (PERL_VERSION_STRING VERSION_LESS FETCH_MINIMUM_PERL_VERSION)
    message(FATAL_ERROR "Perl version ${PERL_VERSION_STRING} found; minimum required version is ${FETCH_MINIMUM_PERL_VERSION}")
  endif()

  find_program(FETCH_MAKE make)
  if (${FETCH_MAKE} STREQUAL FETCH_MAKE-NOTFOUND)
    message(FATAL_ERROR "make not found")
  endif()

  set(FETCH_OPENSSL_BUILD_WORKDIR ${openssl_vendor_dir}/build)
  file(MAKE_DIRECTORY ${FETCH_OPENSSL_BUILD_WORKDIR})

  if(APPLE)
    set(FETCH_OPENSSL_BUILD_CONF fetchai_mac64)
  else()
    set(FETCH_OPENSSL_BUILD_CONF fetchai_linux64)
  endif()

  execute_process(
    COMMAND ${PERL_EXECUTABLE} ${openssl_vendor_dir}/src/Configure ${FETCH_OPENSSL_BUILD_CONF}
      --config=${FETCH_ROOT_VENDOR_AUX_DIR}/openssl/fetchai_openssl.conf
      --prefix=${openssl_install_dir}
      --openssldir=${openssl_vendor_dir}/dist/config
    WORKING_DIRECTORY ${FETCH_OPENSSL_BUILD_WORKDIR}
    RESULT_VARIABLE OPENSSL_BUILD_STATUS
  )
  execute_process(
    COMMAND ${PERL_EXECUTABLE} configdata.pm --dump
    WORKING_DIRECTORY ${FETCH_OPENSSL_BUILD_WORKDIR}
    RESULT_VARIABLE OPENSSL_BUILD_STATUS
  )

  execute_process(
    COMMAND make -j
    WORKING_DIRECTORY ${FETCH_OPENSSL_BUILD_WORKDIR}
    RESULT_VARIABLE OPENSSL_BUILD_STATUS
  )
  if(NOT OPENSSL_BUILD_STATUS EQUAL 0)
    message(FATAL_ERROR "OpenSSL build failure: make")
  endif()

  execute_process(
    COMMAND make test
    WORKING_DIRECTORY ${FETCH_OPENSSL_BUILD_WORKDIR}
    RESULT_VARIABLE OPENSSL_BUILD_STATUS
  )
  if(NOT OPENSSL_BUILD_STATUS EQUAL 0)
    message(FATAL_ERROR "OpenSSL build failure: make test")
  endif()

  execute_process(
    COMMAND make install
    WORKING_DIRECTORY ${FETCH_OPENSSL_BUILD_WORKDIR}
    RESULT_VARIABLE OPENSSL_BUILD_STATUS
  )
  if(NOT OPENSSL_BUILD_STATUS EQUAL 0)
    message(FATAL_ERROR "OpenSSL build failure: make install")
  endif()
endfunction(_build_and_install_openssl)

function(configure_openssl)
  set(FETCH_OPENSSL_VERSION 1.1.1b)

  # Configure OpenSSL library lookup
  set(OPENSSL_ROOT_DIR ${FETCH_ROOT_VENDOR_DIR}/openssl/dist/openssl)

  unset(OPENSSL_CRYPTO_LIBRARY CACHE)
  unset(OPENSSL_INCLUDE_DIR CACHE)
  unset(OPENSSL_SSL_LIBRARY CACHE)
  find_package(OpenSSL)
  if(NOT (OPENSSL_FOUND AND (OPENSSL_VERSION STREQUAL FETCH_OPENSSL_VERSION)))
    message("OpenSSL not found or unexpected version: building from source")
    _build_and_install_openssl(
      ${OPENSSL_ROOT_DIR}
      ${FETCH_ROOT_VENDOR_DIR}/openssl
    )
  endif()

  # Confirm successful installation and version correctness
  unset(OPENSSL_CRYPTO_LIBRARY CACHE)
  unset(OPENSSL_INCLUDE_DIR CACHE)
  unset(OPENSSL_SSL_LIBRARY CACHE)
  find_package(OpenSSL REQUIRED)
  if(NOT (OPENSSL_VERSION STREQUAL FETCH_OPENSSL_VERSION))
    message(FATAL_ERROR "Unexpected version of OpenSSL: ${OPENSSL_VERSION} instead of ${FETCH_OPENSSL_VERSION}")
  endif()

  if(FETCH_VERBOSE_CMAKE)
    message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")
    if(OPENSSL_USE_STATIC_LIBS)
      message(STATUS "OpenSSL linking: Static")
    else()
      message(STATUS "OpenSSL linking: Dynamic")
    endif()
  endif(FETCH_VERBOSE_CMAKE)

  add_library(vendor-openssl INTERFACE)
  target_link_libraries(vendor-openssl INTERFACE ${OPENSSL_LIBRARIES})

  if(OPENSSL_USE_STATIC_LIBS)
    target_link_libraries(vendor-openssl INTERFACE ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
  endif(OPENSSL_USE_STATIC_LIBS)

  target_include_directories(vendor-openssl INTERFACE ${OPENSSL_INCLUDE_DIR})
endfunction(configure_openssl)
