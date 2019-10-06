load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")

fetchai_cc_library(
  name = "logging",
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:core",
    "//libs/telemetry:telemetry",
    "//vendor:spdlog",
    "//vendor:backward-cpp",
  ],
)