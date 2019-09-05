cc_library(
  name = "fetch-variant",
  visibility = [
      "//visibility:public",
  ],
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:fetch-core",
    "//libs/crypto:fetch-crypto",
    "//libs/meta:fetch-meta",
    "//libs/logging:fetch-logging"
  ],
)