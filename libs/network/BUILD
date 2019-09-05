cc_library(
  name = "fetch-network",
  visibility = [
      "//visibility:public",
  ],
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:fetch-core",
    "//libs/crypto:fetch-crypto",
    "//libs/math:fetch-math",
    "//libs/json:fetch-json",
    "//libs/logging:fetch-logging"
  ],
)