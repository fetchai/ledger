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
  name = "asio",
  visibility = [
      "//visibility:public",
  ],
  srcs = [],
  hdrs = glob([
      "asio/asio/include/*.hpp",
      "asio/asio/include/**/*.hpp",
      "asio/asio/include/**/*.ipp",
  ]),
  deps = [],
  copts = [],
)

cc_library(
  name = "utfcpp",
  visibility = [
      "//visibility:public",
  ],
  srcs = [],
  hdrs = glob([
      "utfcpp/source/*.h",
      "utfcpp/source/utf8/*.h",
  ]),
  deps = [],
  copts = [],
)

cc_library(
  name = "msgpack",
  visibility = [
      "//visibility:public",
  ],
  hdrs = glob([
      "msgpack/include/*.hpp",
      "msgpack/include/*.h",
      "msgpack/include/**/*.hpp",
      "msgpack/include/**/*.h",
  ]),
  srcs = [],
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

def generate_mcl_src_list():
    print(':darwin')

    darwin_srcs = [
        'mcl/src/fp.cpp',
        'mcl/src/asm/x86-64mac.S',
        'mcl/src/asm/x86-64mac.bmi2.S',
    ]

    default_srcs = [
        'mcl/src/fp.cpp',
        'mcl/src/asm/x86-64.S',
        'mcl/src/asm/x86-64.bmi2.S',
    ]

    return darwin_srcs


cc_library(
  name = "mcl",
  visibility = [
      "//visibility:public",
  ],
  srcs = generate_mcl_src_list(),
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
