load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")


fetchai_cc_library(
  name = "shards",
  srcs = glob(["src/*.cpp", "src/**/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:core",
    "//libs/logging:logging",
    "//libs/muddle:muddle",
    "//libs/json:json",
    "//libs/network:network",
    "//libs/crypto:crypto",
    "//libs/vectorise:vectorise",
    "//libs/variant:variant",
    "//libs/meta:meta",
    "//vendor:asio",
  ],
)
