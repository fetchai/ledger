#!/usr/bin/env python3

#
# Generate coverage html for unit tests. Default usage will build all tests and a global coverage
# file in build/coverage. Typical usage:
# Build in debug mode with coverage flag enabled (FETCH_ENABLE_COVERAGE).
# ./scripts/generate_coverage.py ./build
#
# Best done in the docker container.
#
# On successful completion, you should be able to view ./build/coverage/coverage_XXX/index.html in
# a browser

import os
import sys
import subprocess
import re
import pprint
import argparse
import shutil

pp = pprint.PrettyPrinter(indent=4)
project_root = 'unknown'
build_directory = 'unknown'






def create_dir(dirname, remove):
    dirname = os.path.join(build_directory, dirname)

    if os.path.exists(dirname):
        shutil.rmtree(dirname)

    os.makedirs(dirname)


def remove_blacklisted(targets: map) -> map:
    """Certain test targets may take a long time or segfault"""

    blacklist = {'executors_test',
                 'main_chain_test',
                 'tcp_client_stress_test',
                 'thread_manager_stress_test',
                 'thread_pool_stress_test',
                 'document_store_test',
                 'object_store_test',
                 'object_sync_test'}

    unwanted = set(targets.keys()) & blacklist
    for unwanted_key in unwanted:
        del targets[unwanted_key]

    return targets


def get_targets_only_tests(tests: list, targets: map) -> map:
    """Given we want coverage for tests, and we know their targets from cmake, return"""

    ret = targets
    if not set(tests).issubset(ret.keys()):
        print("Failed to find one or more tests in cmake target list.")
        print("Tests:")
        pp.pprint(tests)
        print("Targets:")
        pp.pprint(targets)
        sys.exit(1)

    unwanted = set(ret.keys()) - set(tests)

    for unwanted_key in unwanted:
        del ret[unwanted_key]
    return ret


def get_coverage_reports(targets: map):
    """Get coverage reports by iterating through the targets and running them

    If the targets were compiled with coverage flags in clang, a profraw file will be created.
    This is used in calling llvm according to  https://clang.llvm.org/docs/SourceBasedCodeCoverage.html
    """

    for key, val in targets.items():
        try:
            os.remove(os.path.join(build_directory, "default.profraw"))
        except:
            pass

        print("Executing: " + key)
        print(val)

        # Execute the binary target, this will generate a default.profraw
        try:
            subprocess.check_output([val], cwd=build_directory, timeout=3*60)
        except subprocess.TimeoutExpired:
            print(
                "\nWARNING: Timed out. This may not provide an accurate coverage report")
        except subprocess.CalledProcessError:
            print(
                "\nWARNING: Returned non-zero error code. Skipping as coverage will not be valid")
            continue

        raw_file_name = key + ".profraw"
        indexed_file_name = key + ".profdata"
        directory_name = os.path.join("coverage", key + "_coverage")
        raw_file_name_dst = ''
        raw_indexed_file_dst = os.path.join(build_directory, indexed_file_name)

        # Rename default.profraw to target.profraw
        try:
            src = os.path.join(build_directory, "default.profraw")
            dst = os.path.join(build_directory, raw_file_name)
            os.rename(src, dst)
        except:
            print("Failed to find output file default.profraw")
            print("Check you haven't set LLVM_PROFILE_FILE")
            print("Did you build in debug mode with FETCH_ENABLE_COVERAGE enabled?")
            sys.exit(1)

        # Generate an indexed file target.profdata
        subprocess.check_output(
            ["llvm-profdata", "merge", "-sparse", raw_file_name, "-o",
             indexed_file_name],
            cwd=build_directory)

        # Generate the coverage in coverate/target_coverate
        subprocess.check_output(
            ["llvm-cov", "show", "-Xdemangler", "c++filt", val,
             "-instr-profile=" + indexed_file_name, "-format=html", "-o",
             directory_name],
            cwd=build_directory)


def get_all_coverage_reports(targets: map):
    """Once coverage reports for other binaries have been created, this function just creates a report
    for all binaries
    """

    all_profs = []
    all_binaries = []
    for key, val in targets.items():
        raw_file_name = key + ".profraw"
        indexed_file_name = key + ".profdata"
        if not os.path.isfile(build_directory+raw_file_name) or not os.path.isfile(build_directory+indexed_file_name):
            print("\nWARNING: Failed to find coverage output files " +
                  raw_file_name + " and " + indexed_file_name)
            continue

        all_profs += [indexed_file_name]
        all_binaries += ["-object", val]

    args = ["llvm-profdata", "merge", "-sparse"]
    args.extend(all_profs)
    args.extend(["-o", "final.profdata"])

    subprocess.check_output(args, cwd=build_directory)

    args = ["llvm-cov", "show", "-Xdemangler", "c++filt"]
    args.extend(all_binaries)
    args.extend(["-instr-profile=final.profdata",
                 "-format=html", "-o", "coverage/all_coverage"])
    subprocess.check_output(args, cwd=build_directory)


def parse_commandline():
    parser = argparse.ArgumentParser(
        description='Generate coverage for fetch-ledger,\
            usually in build/coverage/coverage_TEST. Each run will clear build/coverage.')

    # Required argument
    parser.add_argument(
        'build_directory', type=str,
        help='Location of the build directory relative to current path')

    parser.add_argument(
        '--tests', type=str, nargs='+',
        help='Name of unit tests to generate coverage for. Defaults to all')

    parser.add_argument(
        '--binaries', type=str, nargs='+',
        help='Location of a binary file to generate coverage for. Ensure the binary terminates.')

    parser.add_argument('--no_clear_coverage', action='store_true',
                        help='Switch to not clear the coverage directory')

    return parser.parse_args()


def get_targets_binaries(binaries: list) -> map:
    targets = {}
    for binary in binaries:
        binary_path = os.path.join(project_root, binary)

        if not os.path.isfile(binary_path):
            print(binary_path + " file not found.")
            sys.exit(1)

        # pdb.set_trace()
        targets[os.path.split(binary_path)[1]] = binary_path

    return targets


def get_targets_cmake() -> map:
    targets = {}

    print("Checking ctest for targets at: " + build_directory)
    output = subprocess.check_output(
        ["ctest", "-N", "--debug"], cwd=build_directory)

    output = output.decode("utf-8")

    for line in output.splitlines():
        match = re.match(r'\d+: Test command: ([\w/-]+)( \"\w+\")?', line)
        if match is not None:

            # extract the full command
            command = match.group(1)
            target = os.path.basename(command)

            targets[target] = command

    if len(targets) == 0:
        print("failed to find targets in output of cmake command - check targets have been built")
        sys.exit(1)

    return targets


def main():
    args = parse_commandline()

    global project_root
    global build_directory

    project_root = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
    build_directory = os.path.join(project_root, args.build_directory)

    if not os.path.isdir(build_directory):
        print("Supplied build path " + build_directory + " not found.")
        sys.exit(1)

    # based on command line args build up targets as a dict of target_name,
    # binary_location
    targets = {}

    if args.binaries:
        targets = get_targets_binaries(args.binaries)

    # Call cmake to get all test names and their exe locations
    cmake_targets = get_targets_cmake()
    cmake_targets = remove_blacklisted(cmake_targets)

    # if we specified test target(s), only get those from the cmake targets
    if args.tests:
        cmake_targets = get_targets_only_tests(args.tests, cmake_targets)
        targets = {**targets, **cmake_targets}

    # If no tests or binaries specified, go with all test targets
    if not args.tests and not args.binaries:
        targets = cmake_targets

    print("Creating coverage for target(s):")
    pp.pprint(targets)

    create_dir("coverage", not args.no_clear_coverage)

    get_coverage_reports(targets)
    get_all_coverage_reports(targets)

    print("Finished.")


if __name__ == '__main__':
    main()
