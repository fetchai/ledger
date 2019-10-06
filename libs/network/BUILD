load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")


fetchai_cc_library(
  name = "network",
  srcs = glob(["src/*.cpp", 'src/**/*.cpp']),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:core",
    "//libs/crypto:crypto",
    "//libs/math:math",
    "//libs/json:json",
    "//libs/logging:logging",
    "//libs/meta:meta",
    "//libs/vectorise:vectorise",
    "//libs/variant:variant", # p2p trust
    "//vendor:asio",
  ],
)
