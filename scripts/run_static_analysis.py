#!/usr/bin/env python3

#
# CODE STATIC ANALYSIS CHECKER
#
# This script is used to run the clang-tidy based static analysis checks on the project
# code base.
#
# Due to the way that the clang-tidy works. It is required that the project has been
# completely built beforehand. After this has been completed the user can simply run
#
# ./scripts/run_static_analysis.py build/
#
# (Assuming that the users output build directory is present at `build/`)
#
# By default the script will only warn the user about issues. In order for the script to
# apply the changes, the user must specify the `--fix` option.
#

import os
import re
import sys
import fnmatch
import argparse
import subprocess
import multiprocessing
import shutil
from concurrent.futures import ThreadPoolExecutor

PROJECT_FOLDERS = ('libs', 'apps')


def find_clang_tidy():
    name = 'clang-tidy'

    # try and find the executable
    path = shutil.which(name)
    if path is not None:
        return path

    output('Unable to find clang-tidy using which attempting manual search...')

    # try and manually perform the search
    for prefix in ('/usr/bin', '/usr/local/bin'):
        potential_path = os.path.join(prefix, name)

        subprocess.check_call(['ls', '-l', prefix])

        if os.path.isfile(potential_path):
            output('Found potential candidate: {}'.format(potential_path))
            if os.access(potential_path, os.X_OK):
                output('Found candidate: {}'.format(potential_path))
                return potential_path


def output(text=None):
    if text is not None:
        sys.stdout.write(text)
    sys.stdout.write('\n')
    sys.stdout.flush()


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fix', action='store_true',
                        help='Apply suggested fixes to files')
    parser.add_argument(
        '-j',
        dest='jobs',
        type=int,
        default=multiprocessing.cpu_count(),
        help='The number of jobs to run in parallel')
    parser.add_argument('build_path', type=os.path.abspath,
                        help='Path to build directory')
    # stack overflow 26727314
    parser.add_argument('--only-these-files', nargs='+',
                        help="If this is the last switch provided right before final positional argument `build _path`, then it is necessary to separate them using ` -- ` separator.")
    return parser.parse_args()


def main():
    args = parse_commandline()

    project_root = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
    clang_tidy = find_clang_tidy()
    if clang_tidy is None:
        output('Failed to locate clang tidy tool')
        sys.exit(1)

    cmd = [
        clang_tidy,
        '-p', args.build_path,
        '-warnings-as-errors=*'
    ]

    if args.fix:
        cmd += ['-fix']
        cmd += ['-fix-errors']
        num_workers = 1
    else:
        num_workers = args.jobs

    excluded_file_list = ["vm/src/tokeniser.cpp"]

    def analyse_file(source_path):

        for excluded_file in excluded_file_list:
            if excluded_file in source_path:
                output("Skipping excluded file {}".format(source_path))
                return False

        output('Analysing {} ...'.format(
            os.path.relpath(source_path, project_root)))

        proc = subprocess.Popen(
            cmd + [source_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)

        while True:
            line = proc.stdout.readline().decode()
            if line == '':
                break
            elif line.startswith('Use -header-filter=.* to display errors'):
                continue
            elif re.match(r'\d+ warnings generated\.', line):
                continue
            elif re.match(r'Suppressed \d+ warnings \(\d+ in non-user code(, \d+ NOLINT)?\)\.', line):
                continue

            output(line.rstrip())

        return proc.wait() != 0

    def project_source_files(only_these_files=None):
        otf = [os.path.abspath(
            item) for item in only_these_files] if only_these_files else None

        for folder in PROJECT_FOLDERS:
            for root, _, files in os.walk(os.path.join(project_root, folder)):
                for path in fnmatch.filter(files, '*.cpp'):
                    source_path = os.path.join(root, path)
                    if otf and source_path not in otf:
                        continue
                    yield source_path

    with ThreadPoolExecutor(max_workers=num_workers) as e:
        results = e.map(analyse_file, project_source_files(
            args.only_these_files))
        if any(results):
            sys.exit(1)


if __name__ == '__main__':
    main()
