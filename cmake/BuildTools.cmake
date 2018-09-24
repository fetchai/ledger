# Function defines the main library component
#
# By looking at the folder structure it is possible to determine what type of
# librray it is i.e. header only or static library.
function(setup_library name)

  # lookup the files for the library
  file(GLOB_RECURSE headers include/*.hpp)
  file(GLOB_RECURSE srcs src/*.cpp)
  list(LENGTH headers headers_length)
  list(LENGTH srcs srcs_length)

  #message(STATUS "Headers: ${headers} ${headers_length}")
  #message(STATUS "Srcs: ${srcs}")

  # main library configuration
  if(srcs_length EQUAL 0)

    # define header only library
    add_library(${name} INTERFACE)
    target_sources(${name} INTERFACE ${headers})
    target_include_directories(${name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

  else()

    # define the normal library
    add_library(${name} ${headers} ${srcs})
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

  endif()
endfunction()

function(fetch_add_executable name)
  list(REMOVE_AT ARGV 0)

  # add the executable
  add_executable(${name} "${ARGV}")

endfunction()

# Function defines additional example target
function(setup_library_examples library)

  if(FETCH_ENABLE_EXAMPLES)
    list(REMOVE_AT ARGV 0)

    # examples
    set(examples_root ${CMAKE_CURRENT_SOURCE_DIR})
    if(IS_DIRECTORY ${examples_root})

      file(GLOB children RELATIVE ${examples_root} ${examples_root}/*)
      set(dirlist "")
      foreach(child ${children})
        set(example_path ${examples_root}/${child})
        if(IS_DIRECTORY ${example_path})
          set(disabled_path ${example_path}/.disabled)
          set(exclusion_path ${example_path}/.manual-config)
          set(example_name "example_${child}")

          if(EXISTS ${exclusion_path})
            # do nothing the target will be manually configured
          elseif(EXISTS ${disabled_path})
            fetch_warning("Disabled Example: ${example_name} - ${example_path}")
          else()
            file(GLOB_RECURSE example_headers ${example_path}/*.hpp)
            file(GLOB_RECURSE example_srcs ${example_path}/*.cpp)

            add_executable(${example_name} ${example_headers} ${example_srcs})
            target_link_libraries(${example_name} PRIVATE ${library})
            target_include_directories(${example_name} PRIVATE ${example_path})
            target_include_directories(${example_name} PRIVATE ${examples_root})

            if(FETCH_VERBOSE_CMAKE)
              message(STATUS "Creating ${example_name} target linking to ${library}")
            endif(FETCH_VERBOSE_CMAKE)
          endif()
        endif()
      endforeach()
    endif()

  endif(FETCH_ENABLE_EXAMPLES)
endfunction()

function(add_fetch_test name library file)
  if(FETCH_ENABLE_TESTS)

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
      fetch_warning("Disabled Test: ${name} - ${file}")
    else()

      include(CTest)

      add_executable(${name} ${file})
      target_link_libraries(${name} PRIVATE ${library} fetch-testing)

      add_test(${name} ${name} ${ARGV})
      set_tests_properties(${name} PROPERTIES TIMEOUT 120)

    endif()

  endif(FETCH_ENABLE_TESTS)
endfunction()

function(add_fetch_gtest name library directory)
  if(FETCH_ENABLE_TESTS)

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
      fetch_warning("Disabled Test: ${name} - ${file}")
    else()

      include(CTest)

      # locate the headers for the test project
      file(GLOB_RECURSE headers ${directory}/*.hpp)
      file(GLOB_RECURSE srcs ${directory}/*.cpp)

      # define the target
      add_executable(${name} ${headers} ${srcs})
      target_link_libraries(${name} PRIVATE ${library} gmock gmock_main)
      target_include_directories(${name} PRIVATE ${FETCH_ROOT_VENDOR_DIR}/googletest/googletest/include)
      target_include_directories(${name} PRIVATE ${FETCH_ROOT_VENDOR_DIR}/googletest/googlemock/include)

      # define the test
      add_test(${name} ${name} ${ARGV})
      set_tests_properties(${name} PROPERTIES TIMEOUT 120)

    endif()

  endif(FETCH_ENABLE_TESTS)
endfunction()


macro(add_test_target)
  if (FETCH_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
  endif (FETCH_ENABLE_TESTS)
endMacro()

