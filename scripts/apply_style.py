#!/usr/bin/env python3

#
# CODE STYLE SCRIPT
#
# This script is used to run check and enforce a consistent code style in the
# project files.
#
# For usage instructions, run
#
#   ./scripts/apply_style.py --help
#

import argparse
import autopep8
import fnmatch
import multiprocessing
import os
import re
import subprocess
import sys
import threading
import time
import traceback
from concurrent.futures import ThreadPoolExecutor
from functools import wraps
from os.path import abspath, basename, dirname, isdir, isfile, join

PROJECT_ROOT = abspath(dirname(dirname(__file__)))
CHECKED_IN_BINARY_FILE_PATTERNS = ('*.png', '*.npy')
INCLUDE_GUARD = '#pragma once'
DISALLOWED_HEADER_FILE_EXTENSIONS = ('*.h', '*.hxx', '*.hh')
CMAKE_VERSION_REQUIREMENT = 'cmake_minimum_required(VERSION 3.5 FATAL_ERROR)'


def find_excluded_dirs():
    def get_abspath(name):
        return abspath(join(PROJECT_ROOT, name))

    def cmake_build_tree_roots():
        direct_subdirectories = os.listdir(PROJECT_ROOT)

        return [name for name in direct_subdirectories
                if isfile(join(get_abspath(name), 'CMakeCache.txt'))]

    directories_to_exclude = ['.git', 'vendor'] + \
        cmake_build_tree_roots()

    return [get_abspath(name)
            for name in directories_to_exclude
            if isdir(get_abspath(name))]


EXCLUDED_DIRS = find_excluded_dirs()

CURRENT_YEAR = int(time.localtime().tm_year)

RE_LICENSE = """//------------------------------------------------------------------------------
//
//   Copyright 2018(-\\d+)? Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 \\(the "License"\\);
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------([\\s\\n]*)
"""

LICENSE = """//------------------------------------------------------------------------------
//
//   Copyright 2018-{} Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

""".format(CURRENT_YEAR)

SUPPORTED_LANGUAGES = {
    'cpp': {
        'cmd_prefix': [
            'clang-format',
            '-style=file'
        ],
        'filename_patterns': ('*.cpp', '*.hpp')
    },
    'cmake': {
        'cmd_prefix': [
            'python3',
            '-m',
            'cmake_format',
            '--separate-ctrl-name-with-space',
            '--line-width', '100',
            '-'
        ],
        'filename_patterns': ('*.cmake', 'CMakeLists.txt')
    }
}

NUM_FILES_PROCESSEFD_SO_FAR = 0  # Global variable
TOTAL_FILES_TO_PROCESS = 0  # Global variable

output_lock = threading.Lock()
file_count_lock = threading.Lock()


def output(text):
    with output_lock:
        sys.stdout.write('{}\n'.format(text))
        sys.stdout.flush()


def increment_file_count():
    global NUM_FILES_PROCESSEFD_SO_FAR

    with file_count_lock:
        NUM_FILES_PROCESSEFD_SO_FAR += 1
        if NUM_FILES_PROCESSEFD_SO_FAR % 100 == 0:
            output('Processed {}\t of {} files'.format(
                NUM_FILES_PROCESSEFD_SO_FAR, TOTAL_FILES_TO_PROCESS))


def include_patterns(*filename_patterns):
    def decorator(func):
        @wraps(func)
        def wrapper(text, path_to_file):
            if any([fnmatch.fnmatch(basename(path_to_file), pattern) for pattern in filename_patterns]):
                return func(text, path_to_file)
            else:
                return text

        return wrapper

    return decorator


def has_include_guard(text):
    return text.startswith(INCLUDE_GUARD)


def walk_source_directories(project_root, excluded_dirs):
    """Walk directory tree, skip excluded subtrees"""
    for root, _, files in os.walk(project_root):
        if any([os.path.commonpath([root, excluded_dir]) == excluded_dir for excluded_dir in excluded_dirs]):
            continue

        yield root, _, files


def external_formatter(cmd_prefix, filename_patterns):

    @include_patterns(*filename_patterns)
    def reformat_as_pipe(text, path_to_file):
        input_bytes = text.encode()

        # Process twice for changes to 'settle'
        for _ in range(2):
            p = subprocess.Popen(
                cmd_prefix,
                stdout=subprocess.PIPE,
                stdin=subprocess.PIPE,
                cwd=PROJECT_ROOT)
            input_bytes = p.communicate(input=input_bytes)[0]

        return input_bytes.decode('utf-8')

    return reformat_as_pipe


@include_patterns('*.cpp', '*.hpp')
def fix_license_header(text, path_to_file):
    if LICENSE in text:
        return text

    # determine if an old license is already present
    existing_license_present = bool(re.search(RE_LICENSE, text, re.MULTILINE))

    if existing_license_present:
        # replace the contents of the license
        return re.sub(RE_LICENSE, LICENSE, text)
    elif fnmatch.fnmatch(basename(path_to_file), '*.cpp'):
        # add license to the top of the file
        return LICENSE + text
    elif fnmatch.fnmatch(basename(path_to_file), '*.hpp'):
        if has_include_guard(text):
            # add the license after the header guard
            return re.sub(r'{}\s+'.format(INCLUDE_GUARD), '{}\n{}'.format(INCLUDE_GUARD, LICENSE), text)
        else:
            return LICENSE + text

    return text


@include_patterns('*.py')
def format_python(text, path_to_file):
    if text.strip():
        return autopep8.fix_code(text)

    return text.strip()


@include_patterns('*.hpp')
def fix_missing_include_guards(text, path_to_file):
    if not has_include_guard(text):
        return '{}\n{}'.format(INCLUDE_GUARD, text)

    return text


@include_patterns('CMakeLists.txt')
def fix_cmake_version_requirements(text, path_to_file):
    lines = text.splitlines()
    counter = 0
    for line in lines:
        if not line.startswith('#') and line.startswith(CMAKE_VERSION_REQUIREMENT):
            return text
        elif not line.startswith('#') and not line.startswith(CMAKE_VERSION_REQUIREMENT):
            break
        counter = counter + 1

    lines.insert(counter, CMAKE_VERSION_REQUIREMENT)

    return '\n'.join(lines)


@include_patterns('*')
def fix_terminal_newlines(text, path_to_file):
    if len(text) and text[-1] != '\n':
        return text + '\n'

    return text


def check_for_headers_with_non_hpp_extension(file_paths):
    output('Checking for header files with incorrect extensions ...')

    invalid_file_groups = [fnmatch.filter(
        file_paths, pattern) for pattern in DISALLOWED_HEADER_FILE_EXTENSIONS]

    invalid_files = [file for group in invalid_file_groups for file in group]
    if invalid_files:
        output("Error: Fetch header files should have the extension '.hpp':")
        list_counter = 1
        for path in invalid_files:
            output('  {}. {}'.format(list_counter, path))
            list_counter = list_counter + 1

        sys.exit(1)


def is_checked_in_binary_file(file_name):
    return any([fnmatch.fnmatch(file_name, pattern) for pattern in CHECKED_IN_BINARY_FILE_PATTERNS])


def get_changed_paths_from_git(commit):
    relative_paths = subprocess.check_output(
        ['git', 'diff', '--name-only', commit]).splitlines()

    return [abspath(join(PROJECT_ROOT, rel_path.decode('utf-8'))) for rel_path in relative_paths]


def files_of_interest(commit):
    global TOTAL_FILES_TO_PROCESS

    output('Composing list of files ...')
    ret = []
    changes = None if commit is None else get_changed_paths_from_git(commit)

    for root, _, files in walk_source_directories(PROJECT_ROOT, EXCLUDED_DIRS):
        for file_name in files:
            if not is_checked_in_binary_file(file_name):
                absolute_path = abspath(join(root, file_name))
                if isfile(absolute_path):
                    if commit is None or absolute_path in changes:
                        ret.append(absolute_path)

    TOTAL_FILES_TO_PROCESS = len(ret)

    return ret


def print_diff_and_fail():
    diff = subprocess.check_output(
        ['git', 'diff'], cwd=PROJECT_ROOT).decode().strip()
    if diff:
        output('*' * 80)
        output(diff)
        output('*' * 80)
        sys.exit(1)


def parse_commandline():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '-c',
        '--commit',
        nargs=1,
        default=None,
        help='scan and fix only files that changed between Git\'s HEAD and the '
             'given commit or ref. If not set, process all files in the project')
    parser.add_argument(
        '-d',
        '--diff',
        action='store_true',
        help='if deviations from the style are found, print the diff and exit '
             'with error. Useful for automated checks')
    parser.add_argument(
        '-j',
        '--jobs',
        type=int,
        default=min(multiprocessing.cpu_count(), 7),
        help='the maximum number of parallel jobs')

    args = parser.parse_args()

    return args.commit if args.commit is None else args.commit[0], \
        args.diff, \
        args.jobs


TRANSFORMATIONS = [
    fix_cmake_version_requirements,
    fix_license_header,
    fix_missing_include_guards,
    fix_terminal_newlines,
    external_formatter(**SUPPORTED_LANGUAGES['cpp']),
    external_formatter(**SUPPORTED_LANGUAGES['cmake']),
    format_python
]


def apply_transformations_to_file(path_to_file):
    try:
        with open(path_to_file, 'r+', encoding='utf-8') as f:
            original_text = f.read()
            text = original_text

            for transformation in TRANSFORMATIONS:
                text = transformation(text, path_to_file)

            if text != original_text:
                f.seek(0)
                f.truncate(0)
                f.write(text)

        increment_file_count()
    except:
        output(traceback.format_exc())


def process_in_parallel(files_to_process, jobs):
    output('Processing {} file(s) with {} job(s) ...'
           .format(TOTAL_FILES_TO_PROCESS, jobs))
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        pool.map(apply_transformations_to_file, files_to_process)

    output('Done.')


def main():
    commit, fail_if_changes, jobs = parse_commandline()

    files_to_process = files_of_interest(commit)
    check_for_headers_with_non_hpp_extension(files_to_process)
    process_in_parallel(files_to_process, jobs)

    if fail_if_changes:
        print_diff_and_fail()


if __name__ == '__main__':
    main()
