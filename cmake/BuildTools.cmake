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

    # CoreFoundation Support on MacOS
    if (APPLE)
      target_link_libraries(${name} PUBLIC "-framework CoreFoundation")
    endif ()

  endif()
endfunction()

function(fetch_add_executable name)
  list(REMOVE_AT ARGV 0)

  # add the executable
  add_executable(${name} "${ARGV}")

  # CoreFoundation Support on MacOS
  if (APPLE)
    target_link_libraries(${name} PRIVATE "-framework CoreFoundation")
  endif ()

endfunction()

# Function defines additional example target
function(setup_library_examples library)

  if(FETCH_ENABLE_EXAMPLES)
    list(REMOVE_AT ARGV 0)

    # build up the library suffix i.e. with fetch-http the suffix would be http
    string(REGEX REPLACE "fetch-(.*)" "\\1" library_suffix ${library})
    if (library_suffix STREQUAL library)
      set (library_suffix "")
    else ()
      set (library_suffix "-${library_suffix}")
    endif ()

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

          # build the example name - replace "_" in favour of "-" to keep target names uniform
          set(example_name "example${library_suffix}-${child}")
          string(REPLACE "_" "-" example_name "${example_name}")

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

            # CoreFoundation Support on MacOS
            if (APPLE)
              target_link_libraries(${example_name} PRIVATE "-framework CoreFoundation")
            endif ()

            if(FETCH_VERBOSE_CMAKE)
              message(STATUS "Creating ${example_name} target linking to ${library}")
            endif(FETCH_VERBOSE_CMAKE)
          endif()
        endif()
      endforeach()
    endif()

  endif(FETCH_ENABLE_EXAMPLES)
endfunction()

function(add_fetch_test name library directory)

  if(NOT name MATCHES "^fetch_[a-zA-Z0-9_]+_tests$")
      message(FATAL_ERROR "fetch tests are required to follow the format fetch_xxx_tests. Found: ${name}")
  endif()

  if(FETCH_ENABLE_TESTS)

    # remove all the arguments
    list(REMOVE_AT ARGV 0)
    list(REMOVE_AT ARGV 0)
    list(REMOVE_AT ARGV 0)

    # define the label for the test
    if ("SLOW" IN_LIST ARGV)
      set(test_label "Slow")
      fetch_warning("Slow Test: ${name}")
    elseif("INTEGRATION" IN_LIST ARGV)
      set(test_label "Integration")
      fetch_warning("Integration Test: ${name}")
    else()
      set(test_label "Normal")
    endif()

    # detect if the "DISABLED" flag has been passed to this test
    set(is_disabled FALSE)
    if ("DISABLED" IN_LIST ARGV)
        set(is_disabled TRUE)
    endif()

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

      # CoreFoundation Support on MacOS
      if (APPLE)
        target_link_libraries(${name} PRIVATE "-framework CoreFoundation")
      endif ()

      # define the test
      add_test(${name} ${name} ${ARGV})
      set_tests_properties(${name} PROPERTIES TIMEOUT 300)
      set_tests_properties(${name} PROPERTIES LABELS "${test_label}")

    endif()

  endif(FETCH_ENABLE_TESTS)
endfunction()

function(add_fetch_gbench name library directory)

  if(FETCH_ENABLE_BENCHMARKS)

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
      fetch_warning("Disabled benchmark: ${name} - ${file}")
    else()

      # locate the headers for the benchmark
      file(GLOB_RECURSE headers ${directory}/*.hpp)
      file(GLOB_RECURSE srcs ${directory}/*.cpp)

      # define the target
      add_executable(${name} ${headers} ${srcs})

      target_link_libraries(${name} PRIVATE ${library} gmock gmock_main)
      target_link_libraries(${name} PRIVATE ${library} benchmark)

      # CoreFoundation Support on MacOS
      if (APPLE)
        target_link_libraries(${name} PRIVATE "-framework CoreFoundation")
      endif ()

      #Google bench requires google test
      target_include_directories(${name} PRIVATE ${FETCH_ROOT_VENDOR_DIR}/googletest/googletest/include)
      target_include_directories(${name} PRIVATE ${FETCH_ROOT_VENDOR_DIR}/googletest/googlemock/include)

      target_include_directories(${name} PRIVATE ${FETCH_ROOT_VENDOR_DIR}/benchmark/include)

    endif()

  endif(FETCH_ENABLE_BENCHMARKS)
endfunction()

macro(add_test_target)
  if (FETCH_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
  endif (FETCH_ENABLE_TESTS)
endMacro()

macro(add_benchmark_target)
  if (FETCH_ENABLE_BENCHMARKS)
    enable_testing()
    add_subdirectory(benchmarks)
  endif (FETCH_ENABLE_BENCHMARKS)
endMacro()

