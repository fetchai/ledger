cc_library(
  name = "fetch-vm-modules",
  visibility = [
      "//visibility:public",
  ],
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/vm:fetch-vm",
    "//libs/core:fetch-core",
    "//libs/math:fetch-math",
    "//libs/ml:fetch-ml",
    "//libs/json:fetch-json",
    "//libs/variant:fetch-variant",
    "//libs/logging:fetch-logging"
  ],
)