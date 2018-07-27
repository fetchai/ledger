#!/usr/bin/env python3
import os
import sys
import fnmatch
import argparse
import subprocess
from concurrent.futures import ThreadPoolExecutor

PROJECT_FOLDERS = ('libs', 'apps')


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fix', action='store_true', help='Apply suggested fixes to files')
    parser.add_argument('build_path', type=os.path.abspath, help='Path to build directory')
    return parser.parse_args()


def main():
    args = parse_commandline()

    project_root = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

    cmd = [
        'clang-tidy',
        '-p', args.build_path,
        '-warnings-as-errors=.*'
    ]

    def analyse_file(source_path):
        print('Analysing {} ...'.format(os.path.relpath(source_path, project_root)))
        exit_code = subprocess.call(cmd + [source_path])
        return exit_code != 0

    def project_source_files():
        for folder in PROJECT_FOLDERS:
            for root, _, files in os.walk(os.path.join(project_root, folder)):
                for path in fnmatch.filter(files, '*.cpp'):
                    source_path = os.path.join(root, path)
                    yield source_path

    with ThreadPoolExecutor(max_workers=4) as e:
        results = e.map(analyse_file, project_source_files())
        if any(results):
            sys.exit(1)


if __name__ == '__main__':
    main()
