#!/usr/bin/env python3
import os
import sys
import re
import argparse
from fetchai_code_quality.internal.graph import Graph, OrientedPath
from typing import Set, Dict, List


LIBRARY_DEPENDS_MATCH = re.compile(r'^(.*?)_LIB_DEPENDS:STATIC=(.*)$')
IGNORED_DEPS = {'general', '-framework CoreFoundation'}
RECURSION_LIMIT = 20

SubpathsSet = Set[OrientedPath[str]]
SubpathsDict = Dict[str, List[OrientedPath[str]]]


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

    dep_graph = Graph()
    all_dependencies = {}
    with open(cache_file_path, 'r') as cache_file:
        for line in cache_file:

            # find all the library dependencies
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
            for dependency in dependencies:
                dep_graph.add(dependency, library)

    # limit targets to the main fetch libraries
    all_fetch_dependencies = filter(
        lambda x: x.startswith('fetch-'),
        dep_graph.nodes.keys()
    )

    exit_code = 0
    print_tree = args.target is not None
    targets = [args.target] if args.target else all_fetch_dependencies
    dependency_paths: SubpathsDict = dep_graph.recurse(targets)
    cyclic_dependencies: SubpathsSet = OrientedPath.extract_canonical_cyclic_subpaths(
        dependency_paths)
    if cyclic_dependencies:
        for path in cyclic_dependencies:
            print("Cyclic dependency found: {}".format(path))
        exit_code = 1

    sys.exit(exit_code)


if __name__ == '__main__':
    main()
