load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")


fetchai_cc_library(
  name = "vm-modules",
  srcs = glob(["src/*.cpp", 'src/**/*.cpp']),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/vm:vm",
    "//libs/core:core",
    "//libs/math:math",
    "//libs/ml:ml",
    "//libs/json:json",
    "//libs/variant:variant",
    "//libs/logging:logging",
    "//libs/vectorise:vectorise",
    "//libs/crypto:crypto",
    "//libs/meta:meta",
  ],
)