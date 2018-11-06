################################################################################
# F E T C H   V E C T O R I S A T I O N   T E S T S
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-vectorisation-tests)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()


add_fetch_gtest(vectorise_exact_exponents_gtest fetch-vectorise meta/gtest)
add_fetch_gtest(Vectorise_memory_gtest fetch-vectorise memory)
add_fetch_gtest(Vectorise_approx_exp_gtest fetch-vectorise vectorize)
include_directories(Vectorise_approx_exp_gtest PRIVATE ../../core/include)

