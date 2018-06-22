function(setup_library name)

  file(GLOB_RECURSE headers include/*.hpp)
  file(GLOB_RECURSE srcs src/*.cpp)

  list(LENGTH headers headers_length)
  list(LENGTH srcs srcs_length)

  #message(STATUS "Headers: ${headers} ${headers_length}")
  #message(STATUS "Srcs: ${srcs}")

  # main library configuration
  if(srcs_length EQUAL 0)

    add_library(${name} INTERFACE)

    target_sources(${name} INTERFACE ${headers})

    target_include_directories(${name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

  else()

    add_library(${name} ${headers} ${srcs})

    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

  endif()
endfunction()


function(setup_library_examples library)

  if(FETCH_BUILD_EXAMPLES)

    # examples
    set(examples_root ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    if(IS_DIRECTORY ${examples_root})

      file(GLOB children RELATIVE ${examples_root} ${examples_root}/*)
      set(dirlist "")
      foreach(child ${children})
        set(example_path ${examples_root}/${child})
        if(IS_DIRECTORY ${example_path})
          set(example_name "example_${child}")

          file(GLOB_RECURSE example_headers ${example_path}/*.hpp)
          file(GLOB_RECURSE example_srcs ${example_path}/*.cpp)

          add_executable(${example_name} ${example_headers} ${example_srcs})
          target_link_libraries(${example_name} PRIVATE ${library})

          message(STATUS "Creating ${example_name} target linking to ${library}")

        endif()
      endforeach()
    endif()

  endif(FETCH_BUILD_EXAMPLES)
endfunction()

function(add_fetch_test name library file)
  if(FETCH_BUILD_TESTS)

    # remove all the arguments
    list(REMOVE_AT ARGV 0)
    list(REMOVE_AT ARGV 0)
    list(REMOVE_AT ARGV 0)

    # detect if the "DISABLED" flag has been passed to this test
    set(is_disabled FALSE)
    foreach(arg ${ARGV})
      if(arg STREQUAL "DISABLED")
        set(is_disabled TRUE)
      endif()
    endforeach()

    if(is_disabled)

      message(STATUS "WARNING: Disabled Test: ${name} - ${file}")

    else()

      include(CTest)

      add_executable(${name} ${file})
      target_link_libraries(${name} PRIVATE ${library})

      add_test(${name} ${name} ${ARGV})

    endif()

  endif(FETCH_BUILD_TESTS)
endfunction()


#function(add_fetch_dependency name dependency)
#
#  target_link_libraries(${name} PUBLIC ${dependency})
#
#endfunction(add_fetch_dependencies)
