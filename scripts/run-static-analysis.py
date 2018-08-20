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
# ./scripts/run-static-analysis.py build/
#
# (Assuming that the users output build directory is present at `build/`)
#
# By default the script will only warn the user about issues. In order for the script to
# apply the changes, the user must specify the `--fix` option.
#

import os
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
    parser.add_argument('--fix', action='store_true', help='Apply suggested fixes to files')
    parser.add_argument('-j', dest='jobs', type=int, default=multiprocessing.cpu_count(), help='The number of jobs to run in parallel')
    parser.add_argument('build_path', type=os.path.abspath, help='Path to build directory')
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
        '-warnings-as-errors=.*'
    ]

    if args.fix:
        cmd += ['-fix']
        cmd += ['-fix-errors']
        num_workers = 1
    else:
        num_workers = args.jobs

    def analyse_file(source_path):
        output('Analysing {} ...'.format(os.path.relpath(source_path, project_root)))
        exit_code = subprocess.call(cmd + [source_path])
        return exit_code != 0

    def project_source_files():
        for folder in PROJECT_FOLDERS:
            for root, _, files in os.walk(os.path.join(project_root, folder)):
                for path in fnmatch.filter(files, '*.cpp'):
                    source_path = os.path.join(root, path)
                    yield source_path

    with ThreadPoolExecutor(max_workers=num_workers) as e:
        results = e.map(analyse_file, project_source_files())
        if any(results):
            sys.exit(1)


if __name__ == '__main__':
    main()
