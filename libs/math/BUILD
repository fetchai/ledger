load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")

fetchai_cc_library(
  name = "math",
  srcs = glob(["src/*.cpp", "src/**/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:core",
    "//libs/logging:logging",
    "//libs/vectorise:vectorise",
    "//libs/meta:meta",
  ],
)