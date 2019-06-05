#!/usr/bin/env python3

#
# CODE STATIC ANALYSIS AND CODE STYLE QUICK FIXER
#
# This script is used to run the apply_style.py and run-statis-analysis.py tools
# after trying to determine the files that actually need changing (NOT GUARANTEED)
# IT WILL AUTOMATICALLY TRY TO FIX which may break the code. Commit your changes first.
#
# *** Requires you to have build the targets in your build directory ***
#
# Normal usage
# ./scripts/run_static_analysis.py
#
# Extended usage
# ./scripts/run_static_analysis.py --build-path ./build --branch master
#
# The script will ask git for the changes between this commit and the last common commit to [master]
# to determine the files that need changing.

import run_static_analysis
import apply_style
import subprocess
import sys
import os
import argparse
import glob
import re


def print_warning(warning):
    try:
        from colors import red, green, blue
        print(red(warning))
    except:
        print("*** " + warning)


def convert_to_dependencies(
        changed_files_fullpath: set, build_path, verbose=False):
    """For each changed hpp file, find a cpp file that depends on it (clang-tidy requires cpp files only) and replace it in the set.

    To do this, find cmake generated depend.make files
    Example depend.make format:
    libs/metrics/CMakeFiles/fetch-metrics.dir/src/metrics.cpp.o: ../libs/vectorise/include/vectorise/math/exp.hpp
    """

    targets_to_substitute = 0
    for filename in changed_files_fullpath:
        if '.hpp' in filename:
            targets_to_substitute = targets_to_substitute + 1

    if targets_to_substitute == 0:
        return changed_files_fullpath

    globbed_files = glob.glob(
        build_path + '/libs/**/depend.make', recursive=True)
    globbed_files.extend(
        glob.glob(build_path + '/apps/**/depend.make', recursive=True))

    # files with no content have this as their first line
    globbed_files = [
        x for x in globbed_files
        if 'Empty dependencies file' not in open(x).readline()]

    if len(globbed_files) == 0:
        print_warning(
            "\nWARNING: No non-empty depend.make files found in {} . Did you make sure to build in this directory?".format(build_path))

    for filename in globbed_files:
        if verbose:
            print("reading file: {}".format(filename))

        for line in open(filename):

            dependency_file = "{}{}{}".format(
                build_path, '/', line.split(' ')[-1])

            if '.hpp' not in dependency_file:
                continue

            dependency_file = os.path.abspath(dependency_file).rstrip()

            target_cpp_file = line.split('.o:')[0]
            target_cpp_file = re.sub(
                'CMakeFiles.*\.dir', '**', target_cpp_file.rstrip())

            if os.path.exists(
                    dependency_file) and '.cpp' in target_cpp_file and dependency_file in changed_files_fullpath:

                if verbose:
                    print("globbing for:")
                    print(target_cpp_file)

                target_cpp_file_save = target_cpp_file
                target_cpp_file = glob.glob(
                    "**" + target_cpp_file, recursive=True)

                if target_cpp_file is None or len(target_cpp_file) == 0:
                    print_warning("Failed to find file/dependency: ")
                    print_warning("File: {}".format(target_cpp_file_save))
                    print_warning("Dependency: {}".format(dependency_file))
                    print_warning("Your build directory may be old")
                    break

                if target_cpp_file is None or len(target_cpp_file) > 1:
                    print("Too many files found matching {}".format(
                        target_cpp_file))

                target_cpp_file = os.path.abspath(target_cpp_file[0])

                if verbose:
                    print("source file: {}".format(dependency_file))
                    print("target cpp file: {}".format(target_cpp_file))

                changed_files_fullpath.add(target_cpp_file)
                changed_files_fullpath.remove(dependency_file)

                targets_to_substitute = targets_to_substitute - 1
                if targets_to_substitute == 0:
                    return changed_files_fullpath

    filenames_to_remove = []
    for filename in changed_files_fullpath:
        if '.hpp' in filename:
            print_warning(
                "\nWARNING: Failed to find a file dependent on {}".format(
                    filename))
            filenames_to_remove.append(filename)

    for filename in filenames_to_remove:
        changed_files_fullpath.remove(filename)

    return changed_files_fullpath


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-path', type=str,
                        help='non default build path', default="./build")
    parser.add_argument(
        '--branch',
        type=str,
        help='branch that is being merged against',
        default="origin/master")
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='verbose mode', default=False)
    return parser.parse_args()


def main():

    args = parse_commandline()

    compare_branch = args.branch
    build_path = args.build_path

    changed_files = []
    # Stack overflow 34279322
    proc = subprocess.Popen(
        ["git", "diff", "{}...".format(compare_branch),
         "--name-only"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    while True:
        line = proc.stdout.readline().decode()
        if line == '':
            break
        line = line.split('\n')[0]
        changed_files.append(line)

    changed_files = set(changed_files)
    tmp = [x for x in changed_files if '.cpp' in x or '.hpp' in x]
    changed_files = tmp

    if len(changed_files) == 0:
        print("No cpp or hpp files appear to have changed.")
        sys.exit(1)

    changed_files_fullpath = set([os.path.abspath(x) for x in changed_files])
    changed_files_fullpath = set(
        [x for x in changed_files_fullpath if os.path.exists(x)])
    if args.verbose:
        print("Changed files:")
        print(changed_files_fullpath)

    print("Applying style")
    sys.argv = ['_', '--commit', compare_branch]
    if args.verbose:
        print(sys.argv)
    apply_style.main()

    print("Running static analysis")
    # static analysis uses cpp files only
    changed_files_fullpath_cpp_only = convert_to_dependencies(
        changed_files_fullpath, build_path, args.verbose)
    sys.argv = ['_', build_path, '--fix', '--only-these-files']
    sys.argv.extend(changed_files_fullpath_cpp_only)
    if args.verbose:
        print(sys.argv)
    run_static_analysis.main()


if __name__ == '__main__':
    main()
