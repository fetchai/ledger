load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")


fetchai_cc_library(
  name = "variant",
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:core",
    "//libs/crypto:crypto",
    "//libs/meta:meta",
    "//libs/logging:logging",
    "//libs/vectorise:vectorise",
  ],
)