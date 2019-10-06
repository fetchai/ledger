load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "openssl",
    url = "https://github.com/openssl/openssl/archive/OpenSSL_1_1_1c.tar.gz",
    sha256 = "640f3a3c26aef38293b4ab4017562aa7dccf787267382d27fc003538b405bbb5",
    strip_prefix = "openssl-OpenSSL_1_1_1c",
    build_file = "@//vendor:BUILD.openssl",
)
