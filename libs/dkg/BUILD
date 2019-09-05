cc_library(
  name = "fetch-dkg",
  visibility = [
      "//visibility:public",
  ],
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:fetch-core",
    "//libs/crypto:fetch-crypto",
    "//libs/network:fetch-network",
    "//libs/address:fetch-address",
    "//libs/entropy:fetch-entropy",
    "//libs/logging:fetch-logging"
  ],
)