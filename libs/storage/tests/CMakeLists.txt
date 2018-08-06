################################################################################
# F E T C H   S T O R A G E   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
project(fetch-storage)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()



add_fetch_test(file_object_test fetch-storage file_object.cpp)
add_fetch_test(random_access_stack_test fetch-storage random_access_stack.cpp)
add_fetch_test(versioned_random_access_stack_test fetch-storage versioned_random_access_stack.cpp)

add_fetch_test(key_value_index_test fetch-storage key_value_index.cpp)
add_fetch_test(document_store_test fetch-storage document_store.cpp)
add_fetch_test(variant_stack_test fetch-storage variant_stack.cpp)
add_fetch_test(versioned_file_object_test fetch-storage versioned_file_object.cpp)
add_fetch_test(versioned_kvi_test fetch-storage versioned_kvi.cpp)

add_fetch_test(object_store_test fetch-storage object_store.cpp)
