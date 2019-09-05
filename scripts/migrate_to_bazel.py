#!/bin/python3.7

import glob
import copy


## Creating list of libraries
zero_deps = []
libraries = {}
for l in glob.glob("libs/*/CMakeLists.txt"):
  with open(l, "r") as fb:
    bf = fb.read()

  library_name = bf.split("setup_library(",1)[1].split(")")[0]
  deps = []

  if "target_link_libraries" in bf:
    deps = bf.split("target_link_libraries(",1)[1].split(")")[0]

    if "PUBLIC" in deps:
      deps = deps.split("PUBLIC")[1]
    elif "PRIVATE" in deps:
      deps = deps.split("PRIVATE")[1]
    elif "INTERFACE" in deps:
      deps = deps.split("INTERFACE")[1]
    deps = [y.strip() for y in deps.split(" ") if y != ""]

  # Removing self-dependency
  deps = [x for x in deps if x != library_name]

  libraries[library_name] = {
    "name": library_name,
    "depends": deps,
    "passed": [x for x in deps if x.startswith("fetch")],
    "required_by": []
  }

  if len(deps) == 0:
    zero_deps.append(library_name)

## Computing dependencies
for lib in libraries.values():
  for d in lib["passed"]:
    libraries[d]["required_by"].append(lib["name"])

while len(zero_deps) != 0:
  libname = zero_deps[0]
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
for lib in libraries.values():
  print(lib["name"]) 
  print(lib["passed"])
  print("")