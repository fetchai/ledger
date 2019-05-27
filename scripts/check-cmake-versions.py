#!/usr/bin/env python3

import argparse
import os
import sys

TARGET_FILES = ('CMakeLists.txt',)
EXPECTED_CMAKE_VERSION = 'cmake_minimum_required(VERSION 3.5 FATAL_ERROR)'


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('path', nargs='?', default='.',
                        type=os.path.abspath, help='The path to search')
    parser.add_argument('--verbose', action='store_true',
                        help='If enabled print more information out')
    return parser.parse_args()


def create_status_printer():
    def basic_formatter(x):
        return 'Passed' if x else 'FAILED'

    try:
        import crayons

        def colour_formatter(x):
            colour = crayons.green if x else crayons.red
            return colour(basic_formatter(x))

        formatter = colour_formatter
    except ImportError:
        formatter = basic_formatter

    return lambda x: '[ ' + formatter(x) + ' ]'


def locate_files(path):
    for root, _, files in os.walk(path):
        if 'vendor' in root:
            continue

        for target in TARGET_FILES:
            if target in files:
                full_path = os.path.join(root, target)
                yield full_path


def check_cmake_file(path):
    with open(path, 'r') as cmake_file:
        for line in cmake_file:
            if line.startswith('#'):
                continue

            # we expect the first non comment line to declare the cmake version
            return line.strip() == EXPECTED_CMAKE_VERSION


def main():
    args = parse_commandline()

    # collect up the file list and remove the unwanted ones
    file_list = list(locate_files(args.path))

    # determine the status of each file
    statuses = list(map(check_cmake_file, file_list))

    # determine if all of the tests passed
    failures_present = not all(statuses)

    # in the failure case or if we request it, print the log
    if failures_present or args.verbose:
        status_printer = create_status_printer()

        for path, status_text in zip(file_list, map(status_printer, statuses)):
            print('{} - {}'.format(status_text, path))

    # if there are any failures then ensure exit code reflects this
    if failures_present:
        sys.exit(1)


if __name__ == '__main__':
    main()
