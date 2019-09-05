cc_library(
  name = "fetch-http",
  visibility = [
      "//visibility:public",
  ],
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    "//libs/network:fetch-network",
    "//libs/logging:fetch-logging"
  ],
)