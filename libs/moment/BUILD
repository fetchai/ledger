load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")


fetchai_cc_library(
  name = "moment",
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:core",
    "//libs/logging:logging"
  ],
)
