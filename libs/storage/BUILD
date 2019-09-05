cc_library(
  name = "fetch-storage",
  visibility = [
      "//visibility:public",
  ],
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:fetch-core",
    "//libs/crypto:fetch-crypto",
    "//libs/testing:fetch-testing",
    "//libs/logging:fetch-logging",
    "//libs/network:fetch-network"
  ],
)