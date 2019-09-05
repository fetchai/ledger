cc_library(
  name = "fetch-json",
  visibility = [
      "//visibility:public",
  ],
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:fetch-core",
    "//libs/variant:fetch-variant",
    "//libs/logging:fetch-logging"
  ],
)