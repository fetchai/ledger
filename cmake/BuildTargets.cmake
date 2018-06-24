macro (setup_compiler)

  set (CMAKE_CXX_STANDARD 11)
  set (CMAKE_CXX_STANDARD_REQUIRED TRUE)

endmacro(setup_compiler)

function (configure_vendor_targets)

  # setup the testing
  if (FETCH_ENABLE_TESTS)
    include(ctest)
    enable_testing()
  endif (FETCH_ENABLE_TESTS)

  # asio vendor library
  add_library(vendor-asio INTERFACE)
  target_include_directories(vendor-asio INTERFACE ${FETCH_ROOT_VENDOR_DIR}/asio/asio/include)
  target_compile_definitions(vendor-asio INTERFACE ASIO_STANDALONE ASIO_HEADER_ONLY ASIO_HAS_STD_SYSTEM_ERROR)

endfunction (configure_vendor_targets)

function (configure_library_targets)

  set (library_root ${FETCH_ROOT_DIR}/libs)

#  add_subdirectory(${library_root}/vectorise)
#  add_subdirectory(${library_root}/core)

  file(GLOB children RELATIVE ${library_root} ${library_root}/*)
  set(dirlist "")
  foreach(child ${children})
    set(element_path ${library_root}/${child})

    if(IS_DIRECTORY ${element_path})
      set(cmake_path ${element_path}/CMakeLists.txt)
        if  (EXISTS ${cmake_path})
          message(STATUS "Configuring Component: ${child}")
          add_subdirectory(${element_path})
        endif()
    endif()
  endforeach()

endfunction (configure_library_targets)
