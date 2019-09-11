#!/bin/python3.7

import glob
import copy


# Creating list of libraries
zero_deps = []
libraries = {}
for l in glob.glob("libs/*/CMakeLists.txt"):
    with open(l, "r") as fb:
        bf = fb.read()

    _, dirname, _ = l.split("/")

    library_name = bf.split("project(", 1)[1].split(")")[0]
    deps = []
    if "target_link_libraries" in bf:
        deps = bf.rsplit("target_link_libraries(", 1)[1].split(")")[0]

        if "PUBLIC" in deps:
            deps = deps.split("PUBLIC")[1]
        elif "PRIVATE" in deps:
            deps = deps.split("PRIVATE")[1]
        elif "INTERFACE" in deps:
            deps = deps.split("INTERFACE")[1]
        deps = [y.strip() for y in deps.split(" ") if y != ""]

    # Removing self-dependency
    deps = [x for x in deps if x != library_name and not x.startswith("$")]

    passed = [x for x in deps if x.startswith("fetch")]
    libraries[library_name] = {
        "name": library_name,
        "dirname": dirname,
        "depends": deps,
        "passed": passed,
        "required_by": []
    }

    if len(passed) == 0:
        zero_deps.append(library_name)

# Computing dependencies
for lib in libraries.values():
    for d in lib["passed"]:
        libraries[d]["required_by"].append(lib["name"])

order = []
while len(zero_deps) != 0:
    libname = zero_deps[0]
    order.append(libname)
    zero_deps = zero_deps[1:]
    lib = libraries[libname]

    print(lib["name"])
    print(lib["depends"])

    for r in lib["required_by"]:
        passed = libraries[r]["passed"]
        passed = [x for x in passed if not x == libname]
        libraries[r]["passed"] = passed

        if len(passed) == 0:
            zero_deps.append(r)

print("")
print("---")

copts = []
for lib in libraries.values():
    if len(lib["passed"]) > 0:
        print(lib["name"])
        print(lib["passed"])
        print("")
    d = lib["dirname"]
    copts.append("\"-Ilibs/%s/include\"" % d)

print("")
print("---")

for libname in order:
    lib = libraries[libname]
    dirname = lib["dirname"]

    depends = []

    for dep in lib["depends"]:
        if not dep.startswith("fetch"):
            continue
        x = libraries[dep]
        d = x["dirname"]
        depends.append("\"//libs/%s:%s\"" % (d, dep))

    with open("libs/%s/BUILD" % dirname, "w") as fb:
        fb.write(
            """cc_library(
  name = "%s",
  visibility = [
      "//visibility:public",
  ],
  srcs = glob(["src/*.cpp"]),
  hdrs = glob(["include/*.hpp", "include/**/*.hpp"]),
  deps = [
    %s
  ],
  copts = [
        %s
  ],
)""" % (libname, ",\n    ".join(depends), ",\n    ".join(copts))
        )
