#!/usr/bin/env python3

#
# CODE STYLE SCRIPT
#
# This script is used to run the clang-format based code style checks on the project.
#
# It can be run simply with the following command:
#
# ./scripts/apply-style.py
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


def output(text=None):
    output_lock.acquire()
    if text is not None:
        sys.stdout.write(str(text))
    sys.stdout.write('\n')
    sys.stdout.flush()
    output_lock.release()


def parse_commandline():
    parser = argparse.ArgumentParser()

    parser.add_argument('-w', '--warn-only', dest='fix', action='store_false', help='Only display warnings')
    parser.add_argument('-j', dest='jobs', type=int, default=multiprocessing.cpu_count(), help='The number of jobs to do in parallel')
    parser.add_argument('-a', '--all', action='store_true', help='Evaluate all files, do not stop on first failure')
    parser.add_argument('filename', metavar='<filename>', action='store', nargs='*', help='process only <filename>s (default: the whole project tree)')

    return parser.parse_args()


def postprocess(lines):
    """ Scans an array of clang-formatted strings and turns any single statement 
    that is a for/while body or then/else clause of an if-statement
    into a braced block."""

    blocks = re.compile(r'( *)(?:if|while|for|else)\b')
    indentation = re.compile(' *')
    unbraced = []
    empties = 0

    for line in lines:
        if line:
            if unbraced:
                recent_level = len(unbraced) * 2
                if line == ' ' * recent_level + '{': unbraced[-1] = False
                else:
                    level = len(indentation.match(line)[0])
                    if level == recent_level + 2:
                        if unbraced[-1]: yield ' ' * recent_level + '{'
                    else:
                        while level <= recent_level:
                            if recent_level and unbraced.pop(): yield ' ' * recent_level + '}'
                            recent_level -= 2
            match = blocks.match(line)
            if match: unbraced.append(True)
            while empties:
                yield ''
                empties -= 1
            yield line
        else:
            empties += 1
    while empties:
        yield ''
        empties -= 1


def postprocess_contents(contents):
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


def compare_against_original(reformatted, source_path, rel_path):

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

    out = list(difflib.context_diff(original.splitlines(), reformatted.splitlines()))

    success = True
    if len(out) != 0:
        output('Style mismatch in: {}'.format(rel_path))
        output()
        output('\n'.join(out[3:])) # first 3 elements are garbage
        success = False

    return success


def main():
    project_root = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

    args = parse_commandline()

    clang_format = find_clang_format()
    if clang_format is None:
        output('Unable to locate clang-format tool')
        sys.exit(1)

    # generate the
    cmd_prefix = [
        clang_format,
        '-style=file',
    ]

    if args.fix:
        cmd_prefix += ['-i']

    def apply_style_to_file(source_path):
        # apply twice to allow the changes to "settle"
        subprocess.check_call(cmd_prefix + [source_path], cwd=project_root)
        subprocess.check_call(cmd_prefix + [source_path], cwd=project_root)
        postprocess_file(source_path)
        return True

    def diff_style_to_file(source_path):
        formatted_output = postprocess_contents(subprocess.check_output(cmd_prefix + [source_path], cwd=project_root).decode())
        rel_path = os.path.relpath(source_path, project_root)
        return compare_against_original(formatted_output, source_path, rel_path)

    if args.fix:
        handler = apply_style_to_file
        output('Applying style...')
    else:
        handler = diff_style_to_file
        output('Checking style...')

    # process all the files
    success = False
    processed_files = args.filename or project_sources(project_root)

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


if __name__ == '__main__':
    main()
