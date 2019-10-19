#!/usr/bin/env python3
import os
import sys
import re
import argparse
import json
import time

from pprint import pprint

LIBRARY_DEPENDS_MATCH = re.compile(r'^(.*?)_LIB_DEPENDS:STATIC=(.*)$')
IGNORED_DEPS = {'general', '-framework CoreFoundation'}
RECURSION_LIMIT = 20


class CircularDependencyError(RuntimeError):
    def __init__(self):
        super().__init__('Circular depenency detected')


def _resolve_dependency_tree(print_tree, name, all_dependencies, index=None, root=None):
    index = index or 1
    root = root or name

    # simple check - do we look to have found ourselves in a loop
    if index > RECURSION_LIMIT:
        raise CircularDependencyError()

    if print_tree:
        prefix = ''.join('  ' * index) + '|-' + ''.join(['--'] * index) + '->'
        print('Resolve: {} {} @ {}'.format(prefix, name, index))

    if name == root and index != 1:
        raise CircularDependencyError()

    #tree = _update_tree(tree, name, index)
    for dep in all_dependencies.get(name, []):
        _resolve_dependency_tree(
            print_tree, dep, all_dependencies, index + 1, root)


def compute_dependency_graph(all_dependencies):

    # Creating list of libraries
    zero_deps = []
    libraries = {}
    for library_name, dependencies in all_dependencies.items():
        if not library_name.startswith('fetch-'):
            continue

        # Removing self-dependency
        #deps = [x for x in deps if x != library_name and not x.startswith("$")]
        #passed = [x for x in deps if x.startswith("fetch")]

        libraries[library_name] = {
            "name": library_name,
            "depends": dependencies,
            "passed": dependencies,
            "required_by": []
        }

        if len(dependencies) == 0:
            zero_deps.append(library_name)

    # Computing dependencies
    for lib in libraries.values():
        for d in lib["passed"]:
            if d not in libraries:
                print('Unable to find library:', d)
                continue

            libraries[d]["required_by"].append(lib["name"])

    pprint(zero_deps)
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

    pprint(libraries)
    pprint(order)


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'build_path', help='The path to the build output folder')
    parser.add_argument('target', nargs='?',
                        help='The specific target to evalute')
    return parser.parse_args()


def main():
    args = parse_commandline()

    cache_file_path = os.path.join(args.build_path, 'CMakeCache.txt')
    if not os.path.isfile(cache_file_path):
        print('Unable to locate the cmake cache file are you sure you have configured the project correctly?')
        sys.exit(1)

    all_dependencies = {}
    with open(cache_file_path, 'r') as cache_file:
        for line in cache_file:

            # find all the library depenencies
            match = LIBRARY_DEPENDS_MATCH.match(line)
            if match is None:
                continue

            library, dependency_list = match.groups()

            dependencies = list(
                filter(
                    lambda x: x not in IGNORED_DEPS and len(x) > 0,
                    dependency_list.split(';')
                )
            )

            if len(dependencies) > 0:
                all_dependencies[library] = dependencies

    # very basic dependency graph profiler
    exit_code = 0

    # limit targets to the main fetch libraries
    all_fetch_dependencies = filter(
        lambda x: x.startswith('fetch-'),
        all_dependencies.keys()
    )

    print_tree = args.target is not None
    targets = [args.target] if args.target else all_fetch_dependencies
    for root in targets:
        try:
            _resolve_dependency_tree(print_tree, root, all_dependencies)
        except CircularDependencyError as dep:
            print('Circular depenency in library path found for {}'.format(root))
            exit_code = 1

    sys.exit(exit_code)


if __name__ == '__main__':
    main()
