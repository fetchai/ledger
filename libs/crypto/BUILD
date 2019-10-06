load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")

fetchai_cc_library(
  name = "crypto",
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:core",
    "//libs/meta:meta",
    "//libs/math:math",
    "//libs/vectorise:vectorise",
    "//libs/logging:logging",
    "//vendor:mcl",
    "@openssl//:ssl",
    "@openssl//:crypto",
  ],
)