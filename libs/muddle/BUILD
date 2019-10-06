load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")


fetchai_cc_library(
  name = "muddle",
  srcs = glob([
      'src/*.cpp',
      'src/**/*.cpp'
  ]),
  hdrs = glob([
      'include/*.hpp',
      'include/**/*.hpp',
      'internal/*.hpp',
  ]),
  deps = [
    "//libs/core:core",
    "//libs/network:network",
    "//libs/meta:meta",
    "//libs/crypto:crypto",
    "//libs/logging:logging",
    "//libs/telemetry:telemetry",
    "//libs/vectorise:vectorise",
    "//libs/moment:moment",
    "//libs/variant:variant",
    "//vendor:asio",
  ],
  copts = [
      '-Ilibs/muddle/internal',
  ]
)
