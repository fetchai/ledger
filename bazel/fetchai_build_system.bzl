load(
    ":fetchai_copts.bzl",
    "fetchai_copts",
)


def fetchai_cc_library(
        name,
        srcs = [],
        hdrs = [],
        copts = [],
        visibility = None,
        external_deps = [],
        tcmalloc_dep = None,
        repository = "",
        linkstamp = None,
        tags = [],
        deps = [],
        strip_include_prefix = None,
        textual_hdrs = None):

    if not visibility:
        visibility = ["//visibility:public"]

    native.cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        copts = fetchai_copts() + copts,
        visibility = visibility,
        tags = tags,
        textual_hdrs = textual_hdrs,
        deps = deps,
        include_prefix = None,
        alwayslink = 1,
        nocopts = "(?:-DDEBUG|-std=c\+\+11)",
        strip_include_prefix = strip_include_prefix,
    )


def fetchai_cc_binary(
        name,
        srcs = [],
        data = [],
        copts = [],
        testonly = 0,
        visibility = None,
        external_deps = [],
        repository = "",
        stamped = False,
        deps = [],
        linkopts = [],
        tags = []):

    if not visibility:
        visibility = ["//visibility:public"]

    native.cc_binary(
        name = name,
        srcs = srcs,
        data = data,
        copts = fetchai_copts() + copts,
        linkopts = linkopts,
        testonly = testonly,
        linkstatic = 1,
        visibility = visibility,
        #malloc = tcmalloc_external_dep(repository),
        stamp = 1,
        deps = deps,
        tags = tags,
        nocopts = "(?:-DDEBUG|-std=c\+\+11)",
    )