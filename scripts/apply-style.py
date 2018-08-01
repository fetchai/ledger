#!/usr/bin/env python3
import os
import sys
import argparse
import fnmatch
import subprocess
import difflib
from concurrent.futures import ThreadPoolExecutor

SOURCE_FOLDERS = ('apps', 'libs')
SOURCE_EXT = ('*.cpp', '*.hpp')


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('-w', '--warn-only', dest='fix', action='store_false', help='Only display warnings')
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
    with open(source_path, 'r') as source_file:
        original = source_file.read()

    out = list(difflib.context_diff(original.splitlines(), reformated.splitlines()))

    changes = False
    if len(out) != 0:
        print('Style mismatch in:', rel_path)
        print('\n'.join(out))
        changes = True

    return changes


def main():
    project_root = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

    args = parse_commandline()

    # generate the 
    cmd_prefix = [
        'clang-format',
        '-style=file',
    ]

    if args.fix:
        cmd_prefix += ['-i']

    def apply_style_to_file(source_path):

        # apply twice to allow the changes to "settle"
        subprocess.check_call(cmd_prefix + [source_path], cwd=project_root)
        subprocess.check_call(cmd_prefix + [source_path], cwd=project_root)

        return False

    def diff_style_to_file(source_path):
        output = subprocess.check_output(cmd_prefix + [source_path], cwd=project_root).decode()
        rel_path = os.path.relpath(source_path, project_root)
        return compare_against_original(output, source_path, rel_path)

    if args.fix:
        handler = apply_style_to_file
        verb = 'Applying'
    else:
        handler = diff_style_to_file
        verb = 'Checking'

    print('{} style...'.format(verb))

    # process all the files
    failure = any(map(handler, project_sources(project_root)))

    print('{} style...complete'.format(verb))

    if failure:
        sys.exit(1)


if __name__ == '__main__':
    main()
