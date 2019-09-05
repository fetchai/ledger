cc_library(
  name = "fetch-crypto",
  visibility = [
      "//visibility:public",
  ],
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:fetch-core",
    "//libs/meta:fetch-meta",
    "//libs/math:fetch-math",
    "//libs/vectorise:fetch-vectorise",
    "//libs/logging:fetch-logging"
  ],
)