
include( CTest )
function(standard_test name file)
  list(REMOVE_AT ARGV 0) 
  
  add_executable(${name} "${name}.cpp")
  target_link_libraries(${name} ${PNG_LIBRARY})
  add_test(${name} ${name} ${ARGV})
endfunction()

function(standard_test name file)
  list(REMOVE_AT ARGV 0) 
  list(REMOVE_AT ARGV 0)
  
  add_executable(${name} ${file})
  target_link_libraries(${name} ${PNG_LIBRARY})
  add_test(${name} ${name} ${ARGV})
endfunction()

function(standard_test_defines name file defines)
  list(REMOVE_AT ARGV 0)    
  list(REMOVE_AT ARGV 0)    
  list(REMOVE_AT ARGV 0)
  
  add_executable(${name} "${name}.cpp")
  target_link_libraries(${name} ${PNG_LIBRARY})
  add_test(${name} ${name} ${ARGV})

endfunction()



### MISC
#standard_test( load_image_read_exceptions image/load_image_read_exceptions.cpp )

standard_test(exp math/test_exp.cpp)
standard_test(spline math/test_spline.cpp)
#standard_test(ndarray math/ndarray.cpp)
standard_test(rectangular_load_save_test math/rectangular_load_save_test.cpp)
standard_test(shared_rectangular_array_test math/shared_rectangular_array_test.cpp)

standard_test(matrix math/linalg/matrix.cpp)
target_link_libraries(matrix libfetchcore)

standard_test(hamming math/distance/hamming.cpp)
target_link_libraries(hamming libfetchcore)

standard_test(manhattan math/distance/manhattan.cpp)
target_link_libraries(manhattan libfetchcore)

standard_test(correlation math/distance/correlation.cpp)
target_link_libraries(correlation libfetchcore)


standard_test(distance_matrix math/distance/distance_matrix.cpp)
target_link_libraries(distance_matrix libfetchcore)

