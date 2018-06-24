################################################################################
# F E T C H   S T O R A G E   L I B R A R Y
################################################################################
cmake_minimum_required(VERSION 3.1)
project(fetch-storage)

# CMake Configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

#-------------------------------------------------------------------------------
# Main Library Target
#-------------------------------------------------------------------------------

setup_library(fetch-storage)
#target_link_libraries(fetch-storage INTERFACE fetch-core vendor-openssl)

#setup_library_examples(fetch-crypto)

#-------------------------------------------------------------------------------
# Unit Test Targets
#-------------------------------------------------------------------------------

add_fetch_test(file_object_test fetch-storage tests/file_object.cpp DISABLED)
add_fetch_test(random_access_stack_test fetch-storage tests/random_access_stack.cpp)
add_fetch_test(variant_stack_test fetch-storage tests/variant_stack.cpp)
add_fetch_test(versioned_random_access_stack_test fetch-storage tests/versioned_random_access_stack.cpp)