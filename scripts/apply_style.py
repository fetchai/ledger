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
import time
import traceback
from functools import wraps
from multiprocessing import Lock, Pool
from os.path import abspath, basename, commonpath, dirname, isdir, isfile, join

PROJECT_ROOT = abspath(dirname(dirname(__file__)))
SUPPORTED_TEXT_FILE_PATTERNS = (
    '*.cmake',
    '*.c',
    '*.cc',
    '*.cpp',
    '*.cxx',
    '*.etch',
    '*.h',
    '*.hh',
    '*.hpp',
    '*.hxx',
    '*.in',
    '*.json',
    '*.md',
    '*.py',
    '*.rst',
    '*.sh',
    '*.svg',
    '*.txt',
    '*.xml',
    '*.yaml',
    '*.yml')
INCLUDE_GUARD = '#pragma once'
DISALLOWED_HEADER_FILE_EXTENSIONS = ('*.h', '*.hxx', '*.hh')
CMAKE_VERSION_REQUIREMENT = 'cmake_minimum_required(VERSION 3.5 FATAL_ERROR)'

CLANG_FORMAT_REQUIRED_VERSION = '6.0'
CLANG_FORMAT_EXE_NAME = 'clang-format-{}'.format(CLANG_FORMAT_REQUIRED_VERSION)
CMAKE_FORMAT_EXE_NAME = 'cmake-format'
CMAKE_FORMAT_REQUIRED_VERSION = '0.5.1'
AUTOPEP8_REQUIRED_VERSION = '1.4.4'


def find_excluded_dirs():
    def is_cmake_build_tree_root(dir_path):
        return isfile(join(dir_path, 'CMakeCache.txt'))

    def is_git_dir(dir_path):
        return basename(dir_path) == '.git'

    def is_nested_git_submodule(dir_path):
        return isfile(join(dir_path, '.git'))

    def is_nested_git_repo(dir_path):
        return dir_path != PROJECT_ROOT and isdir(join(dir_path, '.git'))

    directories_to_exclude = [abspath(join(PROJECT_ROOT, name))
                              for name in ('vendor',)]

    for root, dirs, files in os.walk(PROJECT_ROOT):
        if is_cmake_build_tree_root(root) or \
                is_git_dir(root) or \
                is_nested_git_submodule(root) or \
                is_nested_git_repo(root):
            directories_to_exclude += [root]

    return sorted(set([dir_path
                       for dir_path in directories_to_exclude
                       if isdir(dir_path)]))


EXCLUDED_DIRS = find_excluded_dirs()

CURRENT_YEAR = int(time.localtime().tm_year)

LICENSE_TEMPLATE = """//------------------------------------------------------------------------------
//
//   Copyright 2018-{} Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 {}the "License"{};
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
//------------------------------------------------------------------------------{}"""

LICENSE = LICENSE_TEMPLATE.format(CURRENT_YEAR, '(', ')', '\n\n')
RE_LICENSE = LICENSE_TEMPLATE.format('(-\\d+)?', '\\(', '\\)', '([\\s\\n]*)')

SUPPORTED_LANGUAGES = {
    'cpp': {
        'cmd_prefix': [
            CLANG_FORMAT_EXE_NAME,
            '-style=file'
        ],
        'filename_patterns': ('*.cpp', '*.hpp')
    },
    'cmake': {
        'cmd_prefix': [
            CMAKE_FORMAT_EXE_NAME,
            '--separate-ctrl-name-with-space',
            '--line-width', '100',
            '-'
        ],
        'filename_patterns': ('*.cmake', 'CMakeLists.txt')
    }
}

num_files_processed_so_far = 0  # Global variable
total_files_to_process = 0  # Global variable
failures = []  # Global variable

output_lock = Lock()
file_count_lock = Lock()


def output(text):
    with output_lock:
        sys.stdout.write('{}\n'.format(text))
        sys.stdout.flush()


def increment_file_count(_):
    global num_files_processed_so_far

    with file_count_lock:
        num_files_processed_so_far += 1
        if num_files_processed_so_far % 100 == 0:
            output('Processed {}\t of {} files'.format(
                num_files_processed_so_far, total_files_to_process))


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


def is_excluded_path(absolute_path):
    return any([commonpath([absolute_path, excluded_dir]) == excluded_dir for excluded_dir in EXCLUDED_DIRS])


def walk_source_directories(project_root):
    """Walk directory tree, skip excluded subtrees"""
    for root, _, files in os.walk(project_root):
        if is_excluded_path(root):
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

            if p.returncode != 0:
                raise Exception('External formatter failed')

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
    return autopep8.fix_code(text)


@include_patterns('*.hpp')
def fix_missing_include_guards(text, path_to_file):
    if not has_include_guard(text):
        return '{}\n{}'.format(INCLUDE_GUARD, text)

    return text


@include_patterns('*')
def fix_trailing_whitespace(text, path_to_file):
    lines = text.split('\n')
    output = ''
    for line in lines:
        output += line.rstrip() + '\n'

    return output


@include_patterns('CMakeLists.txt')
def fix_cmake_version_requirements(text, path_to_file):
    lines = text.split('\n')
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
    return text.strip() + '\n'


def check_for_headers_with_non_hpp_extension(file_paths):
    output('Checking for header files with incorrect extensions ...')

    invalid_file_groups = [fnmatch.filter(
        file_paths, pattern) for pattern in DISALLOWED_HEADER_FILE_EXTENSIONS]

    invalid_files = [file for group in invalid_file_groups for file in group]
    if invalid_files:
        output("Error: Fetch header files should have the extension '.hpp':")
        list_counter = 1
        for path in sorted(invalid_files):
            output('  {}. {}'.format(list_counter, path))
            list_counter = list_counter + 1

        sys.exit(1)


def is_supported_text_file(absolute_path):
    return any([fnmatch.fnmatch(basename(absolute_path), pattern)
                for pattern in SUPPORTED_TEXT_FILE_PATTERNS])


def get_changed_paths_from_git(commit):
    raw_relative_paths = subprocess.check_output(['git', 'diff', '--name-only', commit]) \
        .strip() \
        .decode('utf-8') \
        .split('\n')

    relative_paths = [rel_path.strip()
                      for rel_path in raw_relative_paths]

    return [abspath(join(PROJECT_ROOT, rel_path)) for rel_path in relative_paths
            if not is_excluded_path(abspath(join(PROJECT_ROOT, rel_path)))]


def files_of_interest(commit):
    """Returns list of absolute paths to files"""
    global total_files_to_process

    output('Composing list of files ...')

    ret = []

    if commit is None:
        for root, _, files in walk_source_directories(PROJECT_ROOT):
            for file_name in files:
                absolute_path = abspath(join(root, file_name))
                ret.append(absolute_path)
    elif isinstance(commit, list):
        for file_name in commit:
            absolute_path = abspath(join(PROJECT_ROOT, file_name))
            ret.append(absolute_path)
    else:
        ret = get_changed_paths_from_git(commit)

    # Filter out unsupported files and symbolic links
    ret = [abs_path for abs_path in ret
           if is_supported_text_file(abs_path) and isfile(abs_path)]

    total_files_to_process = len(ret)

    return ret


def print_diff_and_fail():
    diff = subprocess.check_output(
        ['git', 'diff'], cwd=PROJECT_ROOT).decode().strip()
    if diff:
        output('*' * 80)
        output(diff)
        output('*' * 80)
        sys.exit(1)


def find_names(root, names):
    ret_val = []
    for name in names:
        file_path = join(root, name)
        if isfile(file_path):
            if os.access(file_path, os.R_OK | os.W_OK):
                ret_val.append(file_path)
            else:
                print('Cannot get RW access to', file_path, file=sys.stderr)
        elif isdir(file_path):
            ret_val.extend(find_names(file_path, os.listdir(file_path)))
        else:
            print('Unknown file_path', file_path, file=sys.stderr)
    return ret_val


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
    parser.add_argument(
        'files',
        nargs='*')

    args = parser.parse_args()

    return find_names('.', args.files) if args.commit is None else args.commit[0], \
        args.diff, \
        args.jobs

TRANSFORMATIONS = [
    fix_cmake_version_requirements,
    fix_license_header,
    fix_missing_include_guards,
    fix_trailing_whitespace,
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

            if text.strip():
                for transformation in TRANSFORMATIONS:
                    text = transformation(text, path_to_file)
            else:
                text = text.strip()

            if text != original_text:
                f.seek(0)
                f.truncate(0)
                f.write(text)
    except:
        output('Error: failed to process {}'.format(path_to_file))
        output(traceback.format_exc())
        raise


def failed(_):
    global failures

    failures.append(_)


def process_in_parallel(files_to_process, jobs):
    output('Processing {} file(s) with {} job(s) ...'
           .format(total_files_to_process, jobs))

    with Pool(processes=jobs) as pool:
        for file in files_to_process:
            pool.apply_async(apply_transformations_to_file, (file,),
                             callback=increment_file_count, error_callback=failed)

        pool.close()
        pool.join()

    if failures:
        output('Failure ({} errors):'.format(len(failures)))
        for err in failures:
            output(err)
        sys.exit(1)

    output('Done.')


def check_tool_versions():
    clang_format_cmd = [CLANG_FORMAT_EXE_NAME, '--version']
    try:
        clang_format_version = subprocess.check_output(
            clang_format_cmd).decode().strip()
    except:
        output(
            'Could not run "{}". Make sure {} is installed.'.format(' '.join(clang_format_cmd), CLANG_FORMAT_EXE_NAME))
        sys.exit(1)
    assert clang_format_version.startswith('clang-format version {}'.format(
        CLANG_FORMAT_REQUIRED_VERSION)), \
        'Unexpected version of clang-format:\n{}\nPlease use version {}.x'.format(
            clang_format_version,
            CLANG_FORMAT_REQUIRED_VERSION)

    cmake_format_cmd = [CMAKE_FORMAT_EXE_NAME, '--version']
    try:
        cmake_format_version = subprocess.check_output(
            cmake_format_cmd).decode().strip()
    except:
        output(
            'Could not run "{}". Make sure {} is installed.'.format(' '.join(cmake_format_cmd), CMAKE_FORMAT_EXE_NAME))
        sys.exit(1)
    assert cmake_format_version == CMAKE_FORMAT_REQUIRED_VERSION, \
        'Unexpected version of cmake-format:\n{}\nPlease use version {}'.format(
            cmake_format_version,
            CMAKE_FORMAT_REQUIRED_VERSION)

    try:
        autopep8_version = autopep8.__version__
    except:
        output('Could not read autopep8 version. Version {} is required.'.format(
            AUTOPEP8_REQUIRED_VERSION))
        sys.exit(1)
    assert autopep8_version == AUTOPEP8_REQUIRED_VERSION, \
        'Unexpected version of autopep8:\n{}\nPlease use version {}'.format(
            autopep8_version,
            AUTOPEP8_REQUIRED_VERSION)


def main():
    check_tool_versions()

    commit, fail_if_changes, jobs = parse_commandline()
    print("Commit:", commit)

    files_to_process = files_of_interest(commit)
    check_for_headers_with_non_hpp_extension(files_to_process)
    process_in_parallel(files_to_process, jobs)

    if fail_if_changes:
        print_diff_and_fail()


if __name__ == '__main__':
    main()
