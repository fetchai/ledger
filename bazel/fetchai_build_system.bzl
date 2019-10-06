load(
    ":fetchai_copts.bzl",
    "fetchai_copts",
)


def fetchai_cc_deps(deps):
    include_ops = []

    # since we are emulating the current structure we need to provide additional include
    # paths based on the dependencies
    for dep in deps:
        if dep.startswith('//libs/'):
            _, _, dep_name = dep.rpartition(':')
            include_ops.append("-Ilibs/" + dep_name + "/include")
        elif dep == '//vendor:spdlog':
            include_ops.append("-Ivendor/spdlog/include")
        elif dep == '//vendor:backward-cpp':
            include_ops.append("-Ivendor/backward-cpp")
        elif dep == '//vendor:mcl':
            include_ops.extend([
                "-Ivendor/mcl/include",
                '-DMCL_USE_VINT',
                '-DMCL_VINT_FIXED_BUFFER',
                '-DMCLBN_FP_UNIT_SIZE=4',
                '-DMCL_USE_LLVM=1',
                '-DMCL_DONT_USE_OPENSSL',
            ])
        elif dep == '//vendor:asio':
            include_ops.extend([
                "-Ivendor/asio/asio/include",
                "-DASIO_STANDALONE",
                "-DASIO_HEADER_ONLY",
                "-DASIO_HAS_STD_SYSTEM_ERROR",
                '-DASIO_DISABLE_STRING_VIEW',
            ])
        elif dep == '//vendor:utfcpp':
            include_ops.append('-Ivendor/utfcpp/source')
        elif dep == '//vendor:msgpack':
            include_ops.extend([
                '-Ivendor/msgpack/include',
                '-DMSGPACK_CXX11=ON',
            ])
    return include_ops


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

    library_copts = []
    library_copts.extend(['-Ilibs/' + name + '/include'])
    library_copts.extend(fetchai_cc_deps(deps))
    library_copts.extend(copts)

    native.cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        copts = fetchai_copts() + select({"//conditions:default": library_copts}),
        visibility = visibility,
        tags = tags,
        textual_hdrs = textual_hdrs,
        deps = deps,
        include_prefix = None,
        alwayslink = 1,
        nocopts = "(?:-DDEBUG|-std=c\+\+11)",
        strip_include_prefix = strip_include_prefix,
    )


def fetchai_cc_application(
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

    application_copts = []
    application_copts.extend(['-iquote apps/' + name, '-iquote .'])
    application_copts.extend(fetchai_cc_deps(deps))
    application_copts.extend(copts)

    native.cc_binary(
        name = name,
        srcs = srcs,
        data = data,
        copts = fetchai_copts() + select({"//conditions:default": application_copts}),
        linkopts = linkopts,
        testonly = testonly,
        linkstatic = 1,
        visibility = visibility,
        stamp = 1,
        deps = deps,
        tags = tags,
        nocopts = "(?:-DDEBUG|-std=c\+\+11)",
    )