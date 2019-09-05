cc_library(
  name = "fetch-ml",
  visibility = [
      "//visibility:public",
  ],
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/core:fetch-core",
    "//libs/math:fetch-math",
    "//libs/logging:fetch-logging"
  ],
)