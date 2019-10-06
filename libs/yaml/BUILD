load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")


fetchai_cc_library(
  name = "yaml",
  srcs = glob(["src/*.cpp", 'src/**/*.cpp']),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:core",
    "//libs/variant:variant",
    "//libs/logging:logging",
    "//libs/crypto:crypto",
    "//libs/vectorise:vectorise",
    "//libs/meta:meta",
  ],
)