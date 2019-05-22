#!/usr/bin/env python3

#
# CODE STYLE SCRIPT
#
# This script is used to run the clang-format based code style checks on the project.
#
# It can be run simply with the following command:
#
# ./scripts/apply_style.py
#
# By default the script will "fix" all style issues that it finds. However, if the user
# only requires warning of the style issues then it is recommended to use the `-w` and
# `-a` options.
#

import argparse
import fnmatch
import multiprocessing
import os
import re
import shutil
import subprocess
import sys
import threading
from concurrent.futures import ThreadPoolExecutor
from os.path import abspath, dirname, join

PROJECT_ROOT = abspath(dirname(dirname(__file__)))

EXCLUDED_DIRS = [
    abspath(join(PROJECT_ROOT, directory))
    for directory in ('.git', 'build', 'vendor')
]


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
        if os.path.isfile(potential_path):
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
            '-style=file',
            '-i'
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
            '--in-place'
        ],
        'filename_patterns': ('*.cmake', 'CMakeLists.txt')
    }
}

output_lock = threading.Lock()


def output(text):
    with output_lock:
        sys.stdout.write('{}\n'.format(text))
        sys.stdout.flush()


def parse_commandline():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '-d',
        '--diff',
        action='store_true',
        help='Exit with error and print diff if there are changes')
    parser.add_argument(
        '-j',
        dest='jobs',
        type=int,
        default=multiprocessing.cpu_count(),
        help='The number of jobs to do in parallel')
    parser.add_argument(
        '-a', '--all', dest='all', action='store_true',
        help='Evaluate all files, do not stop on first failure')
    parser.add_argument(
        '-b',
        '--embrace',
        dest='dont_goto_fail',
        action='store_true',
        help='Put single-statement then/else clauses and loop bodies in braces')
    parser.add_argument(
        '-n',
        '--names-only',
        dest='names_only',
        action='store_true',
        help='In warn-only mode, only list names of files to be formatted')
    parser.add_argument(
        'filename',
        metavar='<filename>',
        action='store',
        nargs='*',
        help='process only <filename>s (default: the whole project tree)')

    return parser.parse_args()


blocks = re.compile(r'( *)(?:if|while|for|else)\b')
indentation = re.compile(r' *')
is_long = re.compile(r'(.*)\\$')
is_empty = re.compile(r'\s*(?://.*)?$')


def opening(level):
    return ' ' * level + '{'


def closing(level):
    return ' ' * level + '}'


def block_entry(level):
    return re.compile(opening(level) + '.*')


def extra_line(line_text, padding):
    if padding:
        return line_text + ' ' * (padding - len(line_text)) + '\\'
    else:
        return line_text


def postprocess(lines):
    """ Scans an array of clang-formatted strings and tries to turn any single statement
    that is either a then/else clause of an if-statement or a for/while loop body
    into a braced block."""

    # These two arrays are tightly coupled.
    levels = []
    unbraced = []

    empties = []
    in_long = None

    for line in lines:
        if is_empty.match(line):
            empties.append(line)
        else:
            if levels:
                recent_level = levels[-1]
                if block_entry(recent_level).match(line):
                    unbraced[-1] = False
                else:
                    level = len(indentation.match(line)[0])
                    if level == recent_level + 2:
                        if unbraced[-1]:
                            yield extra_line(' ' * recent_level + '{', in_long)
                    elif level <= recent_level:
                        while unbraced and level <= recent_level:
                            levels.pop()
                            if unbraced.pop():
                                yield extra_line(closing(recent_level), in_long)
                            if levels:
                                recent_level = levels[-1]
            match = blocks.match(line)
            if match:
                levels.append(len(match[1]))
                unbraced.append(True)
            if empties:
                for e in empties:
                    yield e
                empties.clear()
            yield line
            match = is_long.match(line)
            in_long = match and len(match[1])

    if empties:
        for e in empties:
            yield e
        empties.clear()


def postprocess_contents(contents):
    """ Applies postprocess() to source code in a string."""

    return '\n'.join(postprocess(contents.split('\n')))


def postprocess_file(filename):
    """ Applies postprocess() to file's content in-place."""

    with open(filename, 'r') as source:
        contents = source.read()
    with open(filename, 'w') as destination:
        destination.writelines(postprocess_contents(contents))


def project_sources(patterns):
    for root, _, files in os.walk(PROJECT_ROOT):
        if any([os.path.commonpath([root, excluded_dir]) == excluded_dir for excluded_dir in EXCLUDED_DIRS]):
            continue

        for pattern in patterns:
            for file in fnmatch.filter(files, pattern):
                source_path = join(root, file)
                yield source_path


def format_language(args, cmd_prefix, filename_patterns):
    def apply_style_to_file(source_path):
        # apply twice to allow the changes to "settle"
        subprocess.check_call(cmd_prefix + [source_path], cwd=PROJECT_ROOT)
        subprocess.check_call(cmd_prefix + [source_path], cwd=PROJECT_ROOT)

        if args.dont_goto_fail:
            postprocess_file(source_path)

    if args.names_only:
        output('Files to reformat:')

    processed_files = args.filename or project_sources(filename_patterns)

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        result = pool.map(apply_style_to_file, processed_files)

        if args.all:
            result = list(result)

        all(result)


def format_python(args):
    jobs_arg = ['-j {}'.format(args.jobs)]

    autopep8_cmd = ['autopep8', '.', '--in-place', '--recursive',
                    '--exclude', 'vendor'] + jobs_arg

    subprocess.check_call(autopep8_cmd, cwd=PROJECT_ROOT)


def get_diff():
    return subprocess.check_output(['git', 'diff'], cwd=PROJECT_ROOT).decode().strip()


def main():
    args = parse_commandline()

    # TODO(WK) Make multilanguage reformatting concurrent
    output('Formatting C/C++ ...')
    format_language(args, **SUPPORTED_LANGUAGES['cpp'])
    output('Formatting Python ...')
    format_python(args)
    output('Formatting CMake ...')
    format_language(args, **SUPPORTED_LANGUAGES['cmake'])
    output('Done.')

    if args.diff:
        diff = get_diff()
        if diff:
            print('*' * 80)
            print(diff)
            print('*' * 80)
            sys.exit(1)


if __name__ == '__main__':
    main()
