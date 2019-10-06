load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")


fetchai_cc_library(
  name = "http",
  srcs = glob(["src/*.cpp", "src/**/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:core",
    "//libs/meta:meta",
    "//libs/network:network",
    "//libs/vectorise:vectorise",
    "//libs/variant:variant",
    "//libs/crypto:crypto",
    "//libs/logging:logging",
    "//libs/json:json",
    "//libs/telemetry:telemetry",
    "//vendor:asio",
  ],
)
