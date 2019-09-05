cc_library(
  name = "fetch-vm",
  visibility = [
      "//visibility:public",
  ],
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/math:fetch-math",
    "//libs/core:fetch-core",
    "//libs/variant:fetch-variant",
    "//libs/address:fetch-address",
    "//libs/logging:fetch-logging"
  ],
)