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

import os
import sys
import argparse
import fnmatch
import subprocess
import difflib
import threading
import multiprocessing
import codecs
import shutil
import re
from concurrent.futures import ThreadPoolExecutor

PROJECT_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

SOURCE_FOLDERS = ('apps', 'libs')
SOURCE_EXT = ('*.cpp', '*.hpp')

output_lock = threading.Lock()


def find_clang_format():
    name = 'clang-format'

    # try and find the executable
    path = shutil.which(name)
    if path is not None:
        return path

    output('Unable to find clang-format using which attempting manual search...')

    # try and manually perform the search
    for prefix in ('/usr/bin', '/usr/local/bin'):
        potential_path = os.path.join(prefix, name)
        if os.path.isfile(potential_path):
            output('Found potential candidate: {}'.format(potential_path))
            if os.access(potential_path, os.X_OK):
                output('Found candidate: {}'.format(potential_path))
                return potential_path


def output(text):
    with output_lock:
        sys.stdout.write('{}\n'.format(text))
        sys.stdout.flush()


def parse_commandline():
    parser = argparse.ArgumentParser()

    parser.add_argument('-w', '--warn-only', dest='fix',
                        action='store_false', help='Only display warnings')
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


def project_sources(project_root):
    # process all the files
    for path in SOURCE_FOLDERS:
        for root, _, files in os.walk(os.path.join(project_root, path)):
            for ext in SOURCE_EXT:
                for file in fnmatch.filter(files, ext):
                    source_path = os.path.join(root, file)
                    yield source_path


def compare_against_original(reformatted, source_path, rel_path, names_only):
    # read the contents of the original file
    original = None
    with codecs.open(source_path, 'r', encoding='utf8') as source_file:
        try:
            original = source_file.read()
        except UnicodeDecodeError as ex:
            output('Unable to read contents of file: {}'.format(rel_path))
            output(ex)
            sys.exit(1)

    # handle the read error
    if original is None:
        return False

    out = list(difflib.context_diff(
        original.splitlines(), reformatted.splitlines()))

    success = True
    if len(out) != 0:
        if names_only:
            output(rel_path)
        else:
            output('Style mismatch in: {}\n'.format(rel_path))
            output('\n'.join(out[3:])) # first 3 elements are garbage
            success = False

    return success


def format_cpp(args):
    clang_format = find_clang_format()
    if clang_format is None:
        output('Unable to locate clang-format tool')
        sys.exit(1)

    cmd_prefix = [
        clang_format,
        '-style=file',
    ]

    if args.fix:
        cmd_prefix += ['-i']

    def apply_style_to_file(source_path):
        # apply twice to allow the changes to "settle"
        subprocess.check_call(cmd_prefix + [source_path], cwd=PROJECT_ROOT)
        subprocess.check_call(cmd_prefix + [source_path], cwd=PROJECT_ROOT)

        if args.dont_goto_fail:
            postprocess_file(source_path)

        return True

    def diff_style_to_file(source_path):
        formatted_output = subprocess.check_output(
            cmd_prefix + [source_path], cwd=PROJECT_ROOT).decode()

        if args.dont_goto_fail:
            formatted_output = postprocess_contents(formatted_output)

        rel_path = os.path.relpath(source_path, PROJECT_ROOT)
        return compare_against_original(
            formatted_output, source_path, rel_path, args.names_only)

    if args.fix:
        handler = apply_style_to_file
        output('Applying style...')
    else:
        handler = diff_style_to_file
        if args.names_only:
            output('Files to reformat:')
        else:
            output('Checking style...')

    # process all the files
    success = False

    processed_files = args.filename or project_sources(PROJECT_ROOT)

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        result = pool.map(handler, processed_files)

        if args.all:
            result = list(result)

        success = all(result)

    if args.fix:
        output('Applying style complete!')
    else:
        output('Checking style complete!')

    if not success:
        sys.exit(1)


def format_python(args):
    fix_or_diff_arg = ['--in-place' if args.fix else '--diff']
    # Parallel jobs requires '--in-place' arg
    jobs_arg = ['-j {}'.format(args.jobs)] if args.fix else []

    autopep8_cmd = ['autopep8', '.', '--exit-code', '--recursive',
                    '--exclude', 'vendor'] + fix_or_diff_arg + jobs_arg

    exit_code = subprocess.call(autopep8_cmd, cwd=PROJECT_ROOT)
    if exit_code != 0:
        sys.exit(1)


def main():
    args = parse_commandline()

    # TODO(WK) Make multilanguage reformatting concurrent
    format_cpp(args)
    format_python(args)


if __name__ == '__main__':
    main()
