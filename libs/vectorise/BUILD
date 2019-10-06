load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")

fetchai_cc_library(
  name = "vectorise",
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/meta:meta"
  ],
)
