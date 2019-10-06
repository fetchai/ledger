
def fetchai_copts():
    posix_options = [
        "-std=c++14",
        "-Wall",
        "-Wextra",
        "-Wconversion",
        '-Wpedantic',
        '-mavx2', # needs to be conditional
        '-DFETCH_COMPILE_LOGGING_LEVEL=4',
    ]

    # if is GNU
    #posix_options.extend(['-Wno-pragmas', '-Wno-unknown-pragmas'])

    # if is Clang
    posix_options.extend(['-Wno-unknown-warning-option'])

    # warnings as errors
    #posix_options.append('-Werror')

    # coverage...
    #posix_options.extend(['-g', '-fprofile-instr-generate', '-fcoverage-mapping', '-O0'])

    # debug sanitisers
    #posix_options.append('-fsanitize=address')

    # DW

    items = select({
        "//conditions:default": posix_options,
    }) + select({
        "@bazel_tools//src/conditions:darwin": ["-DFETCH_PLATFORM_MACOS"],
        "//conditions:default": ["-DFETCH_PLATFORM_LINUX"],
    })

    return items

