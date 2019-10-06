load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")


fetchai_cc_library(
  name = "dkg",
  srcs = glob(["src/*.cpp", "src/**/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:core",
    "//libs/crypto:crypto",
    "//libs/network:network",
    "//libs/address:address",
    "//libs/entropy:entropy",
    "//libs/logging:logging",
    "//libs/vectorise:vectorise",
    "//libs/muddle:muddle",
    "//libs/meta:meta",
  ],
)