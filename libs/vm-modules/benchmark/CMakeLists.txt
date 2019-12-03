#
# F E T C H   V M   M O D U L E S   B E N C H M A R K S
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(fetch-vm-modules)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

add_fetch_gbench(benchmark_vm_modules_model fetch-vm-modules ../../vm-modules/benchmark/model)
add_fetch_gbench(benchmark_vm_modules_tensor fetch-vm-modules ../../vm-modules/benchmark/tensor)

add_fetch_gbench(benchmark_vm_modules_model_charge fetch-vm-modules ../../vm-modules/benchmark/model_charge)
