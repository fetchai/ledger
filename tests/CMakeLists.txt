
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

standard_test(script_dictionary_test script/dictionary.cpp )
## chain
standard_test(chain_tx_serialization chain/transaction_serialization.cpp)
## Crypto
standard_test( crypto_sha256 crypto/sha256.cpp )

## Storage
# standard_test(storage_file_object storage/file_object.cpp)

standard_test(versioned_random_access_stack storage/versioned_random_access_stack.cpp)
# standard_test(variant_stack storage/variant_stack.cpp)
standard_test(random_access_stack storage/random_access_stack.cpp)


standard_test(crypto_ecdsa crypto/ecdsa.cpp )
standard_test(crypto_consensus_proof_of_work crypto/consensus/proof_of_work.cpp )
standard_test(math_bignumber math/bignumber.cpp )

### MISC
standard_test( load_image_read_exceptions image/load_image_read_exceptions.cpp )
standard_test(referenced_byte_array_thread_safety byte_array/referenced_byte_array_thread_safety.cpp)
standard_test(referenced_byte_array serializers/referenced_byte_array.cpp)

standard_test(script_ast script/ast.cpp)
standard_test(script_ast2 script/ast2.cpp)

standard_test(encode_decode_test byte_array/encode_decode.cpp)
standard_test(json_document_test byte_array/json/document.cpp)
standard_test(variant_test script/variant.cpp)
standard_test(tokenizer_test byte_array/tokenizer/tokenizer.cpp)
standard_test(referenced_byte_array_test byte_array/referenced_byte_array.cpp)


standard_test(shared_dictionary memory/shared_dictionary.cpp)

standard_test(shared_array_thread_safety memory/shared_array_thread_safety.cpp)
standard_test(shared_array memory/shared_array_test.cpp)
standard_test(shared_square_array memory/shared_square_array_test.cpp)
standard_test(exp math/test_exp.cpp)
standard_test(spline math/test_spline.cpp)
standard_test(lfg random/test_lfg.cpp)
standard_test(matrix math/linalg/matrix.cpp)
standard_test(brute_force optimisation/brute_force.cpp)
standard_test(load_txt optimisation/instance/load_txt.cpp)
standard_test(meta_log2 meta/test_log2.cpp)

standard_test(vectorize_native_register vectorize/native.cpp)
standard_test(vectorize_sse vectorize/sse.cpp)



