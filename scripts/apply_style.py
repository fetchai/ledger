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
import shutil
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
NUM_PROCESSED_FILES = 0
TOTAL_FILES_TO_PROCESS = 0


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


def find_clang_format():
    name = 'clang-format'

    # try and find the executable
    path = shutil.which(name)
    if path is not None:
        return path

    output('Unable to find clang-format using \'which\' attempting manual search...')

    # try and manually perform the search
    for prefix in ('/usr/bin', '/usr/local/bin'):
        potential_path = join(prefix, name)
        if isfile(potential_path):
            output('Found potential candidate: {}'.format(potential_path))
            if os.access(potential_path, os.X_OK):
                output('Found candidate: {}'.format(potential_path))
                return potential_path

    output('Unable to locate clang-format tool')
    sys.exit(1)


SUPPORTED_LANGUAGES = {
    'cpp': {
        'cmd_prefix': [
            find_clang_format(),
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

output_lock = threading.Lock()
file_count_lock = threading.Lock()


def output(text):
    with output_lock:
        sys.stdout.write('{}\n'.format(text))
        sys.stdout.flush()


def walk_source_directories(project_root, excluded_dirs):
    """Walk directory tree, skip listed subtrees"""
    for root, _, files in os.walk(project_root):
        if any([os.path.commonpath([root, excluded_dir]) == excluded_dir for excluded_dir in excluded_dirs]):
            continue

        yield root, _, files


def format_language(cmd_prefix, filename_patterns):
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


@include_patterns('*.py')
def format_python(text, path_to_file):
    if text.strip():
        return autopep8.fix_code(text)

    return text.strip()


def get_diff():
    return subprocess.check_output(['git', 'diff'], cwd=PROJECT_ROOT).decode().strip()


@include_patterns('*.hpp')
def fix_missing_include_guards(text, path_to_file):
    if not has_include_guard(text):
        return '{}\n{}'.format(INCLUDE_GUARD, text)

    return text


@include_patterns('*')
def fix_terminal_newlines(text, path_to_file):
    if len(text) and text[-1] != '\n':
        return text + '\n'

    return text


TRANSFORMATIONS = [
    fix_license_header,
    fix_missing_include_guards,
    fix_terminal_newlines,
    format_language(**SUPPORTED_LANGUAGES['cpp']),
    format_language(**SUPPORTED_LANGUAGES['cmake']),
    format_python
]


def apply_transformations_to_file(path_to_file):
    global NUM_PROCESSED_FILES
    global TOTAL_FILES_TO_PROCESS

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

        with file_count_lock:
            NUM_PROCESSED_FILES += 1
            if NUM_PROCESSED_FILES % 100 == 0:
                output('Processed {}\t of {} files'.format(
                    NUM_PROCESSED_FILES, TOTAL_FILES_TO_PROCESS))

    except:
        output(traceback.format_exc())


def check_for_headers_with_non_hpp_extension(file_paths):
    invalid_file_groups = [fnmatch.filter(
        file_paths, pattern) for pattern in DISALLOWED_HEADER_FILE_EXTENSIONS]

    invalid_files = [file for group in invalid_file_groups for file in group]
    if invalid_files:
        output("Error: Fetch header files should have the extension hpp")
        for path in invalid_files:
            output(path)
        sys.exit(1)


def is_binary_file(file_name):
    return any([fnmatch.fnmatch(file_name, pattern) for pattern in CHECKED_IN_BINARY_FILE_PATTERNS])


def get_changed_paths_from_git(commit):
    relative_paths = subprocess.check_output(
        ['git', 'diff', '--name-only', commit]).splitlines()

    return [abspath(join(PROJECT_ROOT, rel_path.decode('utf-8'))) for rel_path in relative_paths]


def files_of_interest(commit):
    ret = []
    changes = [] if commit is None else get_changed_paths_from_git(commit)

    for root, _, files in walk_source_directories(PROJECT_ROOT, EXCLUDED_DIRS):
        for file_name in files:
            if not is_binary_file(file_name):
                absolute_path = abspath(join(root, file_name))
                if isfile(absolute_path):
                    if commit is None or absolute_path in changes:
                        ret.append(absolute_path)

    return ret


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


def main():
    global TOTAL_FILES_TO_PROCESS

    commit_arg, diff_arg, jobs_arg = parse_commandline()

    output('Composing list of files ...')
    files_to_process = files_of_interest(commit_arg)
    TOTAL_FILES_TO_PROCESS = len(files_to_process)

    output('Checking for header files with incorrect extensions ...')
    check_for_headers_with_non_hpp_extension(files_to_process)

    output('Processing {} file(s) with {} job(s) ...'
           .format(TOTAL_FILES_TO_PROCESS, jobs_arg))
    with ThreadPoolExecutor(max_workers=jobs_arg) as pool:
        pool.map(apply_transformations_to_file, files_to_process)

    output('Done.')

    if diff_arg:
        diff = get_diff()
        if diff:
            output('*' * 80)
            output(diff)
            output('*' * 80)
            sys.exit(1)


if __name__ == '__main__':
    main()
