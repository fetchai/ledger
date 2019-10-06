load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")


fetchai_cc_library(
  name = "beacon",
  srcs = glob(["src/*.cpp", 'src/**/*.cpp']),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:core",
    "//libs/crypto:crypto",
    "//libs/logging:logging",
    "//libs/vectorise:vectorise",
    "//libs/meta:meta",
    "//libs/network:network",
    "//libs/muddle:muddle",
    "//libs/entropy:entropy",
    "//libs/moment:moment",
    "//libs/telemetry:telemetry",
    "//libs/dkg:dkg",
    "//libs/shards:shards",
    "//vendor:mcl",
    "//vendor:asio",
  ],
)