#!/usr/bin/env python3
import os
import sys
import argparse
import fnmatch
import subprocess
import difflib
import threading
import multiprocessing
import codecs
from concurrent.futures import ThreadPoolExecutor
from distutils.spawn import find_executable

SOURCE_FOLDERS = ('apps', 'libs')
SOURCE_EXT = ('*.cpp', '*.hpp')


output_lock = threading.Lock()

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
    return parser.parse_args()


def project_sources(project_root):

    # process all the files
    for path in SOURCE_FOLDERS:
        for root, _, files in os.walk(os.path.join(project_root, path)):
            for ext in SOURCE_EXT:
                for file in fnmatch.filter(files, ext):
                    source_path = os.path.join(root, file)
                    yield source_path


def compare_against_original(reformated, source_path, rel_path):

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

    out = list(difflib.context_diff(original.splitlines(), reformated.splitlines()))

    success = True
    if len(out) != 0:
        output_lock.acquire()
        output('Style mismatch in: {}'.format(rel_path))
        output()
        output('\n'.join(out[3:])) # first 3 elements are garbage
        success = False
        output_lock.release()

    return success


def main():
    project_root = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

    args = parse_commandline()

    clang_format = find_executable('clang-format')

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

        return True

    def diff_style_to_file(source_path):
        formatted_output = subprocess.check_output(cmd_prefix + [source_path], cwd=project_root).decode()
        rel_path = os.path.relpath(source_path, project_root)
        return compare_against_original(formatted_output, source_path, rel_path)

    if args.fix:
        handler = apply_style_to_file
        verb = 'Applying'
    else:
        handler = diff_style_to_file
        verb = 'Checking'

    output('{} style...'.format(verb))

    # process all the files
    success = False
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        result = map(handler, project_sources(project_root))

        if args.all:
            result = list(result)

        success = all(result)

    output('{} style...complete'.format(verb))

    if not success:
        sys.exit(1)


if __name__ == '__main__':
    main()
