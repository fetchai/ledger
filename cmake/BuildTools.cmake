function(setup_library name)

  file(GLOB_RECURSE headers include/*.hpp)
  file(GLOB_RECURSE srcs src/*.cpp)

  list(LENGTH headers headers_length)
  list(LENGTH srcs srcs_length)

  #message(STATUS "Headers: ${headers} ${headers_length}")
  #message(STATUS "Srcs: ${srcs}")

  if (srcs_length EQUAL 0)

    message(STATUS "Header Only")
    add_library(${name} INTERFACE)

    target_sources(${name} INTERFACE ${headers})

    target_include_directories(${name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

  else()

    message(STATUS "Normal library")
    add_library(${name} ${headers} ${srcs})

    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

  endif()

endfunction()


#function(add_fetch_dependency name dependency)
#
#  target_link_libraries(${name} PUBLIC ${dependency})
#
#endfunction(add_fetch_dependencies)
