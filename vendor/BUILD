config_setting(
    name = "darwin",
    values = {
        "cpu": "darwin_x86_64",
    },
)

cc_library(
  name = "spdlog",
  visibility = [
      "//visibility:public",
  ],
  srcs = [],
  hdrs = glob(["spdlog/include/**/*.h"]),
  deps = [],
  copts = [],
)

cc_library(
  name = "backward-cpp",
  visibility = [
      "//visibility:public",
  ],
  srcs = ["backward-cpp/backward.cpp"],
  hdrs = ["backward-cpp/backward.hpp"],
  deps = [],
  copts = [],
)


#   add_library(vendor-bls-internal STATIC ${FETCH_ROOT_VENDOR_DIR}/bls/src/bls_c256.cpp
  #                                         ${FETCH_ROOT_VENDOR_DIR}/bls/src/bls_c384.cpp)
  #  target_link_libraries(vendor-bls-internal PUBLIC vendor-mcl)
  #  target_include_directories(vendor-bls-internal PUBLIC ${FETCH_ROOT_VENDOR_DIR}/bls/include)
  #  target_compile_definitions(vendor-bls-internal
  #                             PUBLIC
  #                             -DMCL_USE_VINT
  #                             -DMCL_VINT_FIXED_BUFFER)
  #
  #  add_library(vendor-bls INTERFACE)
  #  target_link_libraries(vendor-bls INTERFACE vendor-bls-internal)
  #  target_compile_definitions(vendor-bls INTERFACE -DMCLBN_FP_UNIT_SIZE=4)

#  # MCL TODO: Work out how to get this to work with the already found version of OpenSSL
 #  set(USE_GMP OFF CACHE BOOL "use gmp" FORCE)
 #  set(USE_OPENSSL OFF CACHE BOOL "use openssl" FORCE)
 #  set(ONLY_LIB ON CACHE BOOL "only lib" FORCE)
 #  add_subdirectory(${FETCH_ROOT_VENDOR_DIR}/mcl)
 #  target_include_directories(mcl_st INTERFACE ${FETCH_ROOT_VENDOR_DIR}/mcl/include)
 #  target_compile_definitions(mcl_st
 #                             INTERFACE
 #                             -DMCL_USE_VINT
 #                             -DMCL_VINT_FIXED_BUFFER
 #                             -DMCLBN_FP_UNIT_SIZE=4)
 #
 #  add_library(vendor-mcl INTERFACE)
 #  target_link_libraries(vendor-mcl INTERFACE mcl_st)

cc_library(
  name = "mcl",
  visibility = [
      "//visibility:public",
  ],
  srcs = [
      "mcl/src/fp.cpp",
  ]
#  + select({
#       ':darwin': [
#           'mcl/src/asm/x86-64mac.S',
#           'mcl/src/asm/x86-64mac.bmi2.S',
#       ],
#       '//conditions:default': [
#           'mcl/src/asm/x86-64.S',
#           'mcl/src/asm/x86-64.bmi2.S',
#       ],
#  })
  ,
  hdrs = glob([
      "mcl/include/**/*.hpp",
      "mcl/include/**/*.h",
      "mcl/src/*.hpp",
      "mcl/src/**/*.h",
  ]),
  deps = [],
  copts = [
      '-std=c++11',
      '-Ivendor/mcl/include',
      '-Wall',
      '-Wextra',
      '-Wformat=2',
      '-Wcast-qual',
      '-Wcast-align',
      '-Wwrite-strings',
      '-Wfloat-equal',
      '-Wpointer-arith',
      '-DMCL_USE_VINT',
      '-DMCL_VINT_FIXED_BUFFER',
      '-DMCLBN_FP_UNIT_SIZE=4',
      '-DMCL_USE_LLVM=1',
      '-DMCL_DONT_USE_OPENSSL',
  ],
)

#cc_library(
#  name = "bls",
#  visibility = [
#      "//visibility:public",
#  ],
#  srcs = ["bls/src/bls_c256.cpp", "bls/src/bls_c384.cpp"],
#  hdrs = ["backward-cpp/backward.hpp"],
#  deps = [],
#  copts = [],
#)

