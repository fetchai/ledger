load("//bazel:fetchai_build_system.bzl", "fetchai_cc_library")


fetchai_cc_library(
  name = "miner",
  srcs = glob(["src/*.cpp", "src/**/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:core",
    "//libs/meta:meta",
    "//libs/address:address",
    "//libs/ledger:ledger",
    "//libs/storage:storage",
    "//libs/beacon:beacon",
    "//libs/crypto:crypto",
    "//libs/vectorise:vectorise",
    "//libs/network:network",
    "//libs/telemetry:telemetry",
    "//libs/muddle:muddle",
    "//libs/bloom-filter:bloom-filter",
    "//libs/logging:logging",
    "//libs/dkg:dkg",
    "//vendor:mcl",
  ],
)
