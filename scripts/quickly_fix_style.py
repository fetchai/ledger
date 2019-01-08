#!/usr/bin/env python3

#
# CODE STATIC ANALYSIS AND CODE STYLE QUICK FIXER
#
# This script is used to run the apply_style.py and run-statis-analysis.py tools
# after trying to determine the files that actually need changing (NOT GUARANTEED)
# IT WILL AUTOMATICALLY TRY TO FIX which may break the code. Commit your changes first.
#
# Normal usage
# ./scripts/run_static_analysis.py
#
# Extended usage
# ./scripts/run_static_analysis.py --build-path ./build --branch develop
#
# The script will ask git for the changes between this commit and the last common commit to [develop]
# to determine the files that need changing.

import run_static_analysis
import apply_style
import check_licence_header
import subprocess
import sys
import os
import argparse

def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-path', action='store_true', help='non default build path', default="./build")
    parser.add_argument('--branch', action='store_true', help='branch that is being merged against', default="develop")
    parser.add_argument('-v', '--verbose', action='store_true', help='verbose mode', default=False)
    return parser.parse_args()

def main():

    args = parse_commandline()

    compare_branch = args.branch
    build_path     = args.build_path

    # Firstly, check licence headers
    print("Checking licence headers")
    if not check_licence_header.main():
        print("Failed to check header. quit.")
        sys.exit(1)

    changed_files = []
    # Stack overflow 34279322
    proc = subprocess.Popen(["git", "diff", "{}...".format(compare_branch), "--name-only"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    while True:
        line = proc.stdout.readline().decode()
        if line == '':
            break
        line = line.split('\n')[0]
        changed_files.append(line)

    tmp = [x for x in changed_files if '.cpp' in x or '.hpp' in x]
    changed_files = tmp

    if len(changed_files) == 0:
        print("No cpp or hpp files appear to have changed.")
        sys.exit(1)

    changed_files_fullpath = [os.path.abspath(x) for x in changed_files]
    if args.verbose:
        print("Changed files:")
        print(changed_files_fullpath)

    # static analysis uses cpp files
    changed_files_fullpath_cpp_only = [x for x in changed_files_fullpath if '.cpp' in x]

    print("Running static analysis")
    sys.argv = ['_', build_path, '--fix', '--only-these-files']
    sys.argv.extend(changed_files_fullpath_cpp_only)
    if args.verbose:
        print(sys.argv)
    run_static_analysis.main()

    print("Applying style")
    sys.argv = ['_']
    sys.argv.extend(changed_files_fullpath)
    if args.verbose:
        print(sys.argv)
    apply_style.main()

if __name__ == '__main__':
    main()
