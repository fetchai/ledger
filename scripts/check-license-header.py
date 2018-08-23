#!/usr/bin/env python3
import argparse
import fnmatch
import os
import re
import sys

LICENSE = """//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

"""

PROJECT_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
APP_FOLDERS = ('apps', 'libs')
FILE_PATTERS = ('*.cpp', '*.hpp')


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fix', action='store_true', help='Enable updating of source files')
    return parser.parse_args()


def project_source_files():
    for folder in APP_FOLDERS:
        folder_root = os.path.join(PROJECT_ROOT, folder)
        for root, _, files in os.walk(folder_root):
            for pattern in FILE_PATTERS:
                for filename in fnmatch.filter(files, pattern):
                    file_path = os.path.abspath(os.path.join(root, filename))
                    yield file_path


def read_file(path):
    with open(path, 'r', encoding='utf-8') as input_file:
        return input_file.read()


def check_source_file(path):
    contents = read_file(path)

    if LICENSE not in contents:
        print('License missing from:', os.path.relpath(path, PROJECT_ROOT))
        return False

    return True


def update_source_file(path):

    contents = read_file(path)

    # do not bother processing files which have the license
    if LICENSE in contents:
        return

    # update the contents of the file
    if path.endswith('.cpp'):
        contents = LICENSE + contents

    elif path.endswith('.hpp'):
        contents = re.sub(r'#pragma once\s+', '#pragma once\n' + LICENSE, contents)
        #contents = contents.replace('#pragma once\n', '#pragma once\n' + LICENSE)

    else:
        print('Unable to update file: ', os.path.relpath(path, PROJECT_ROOT))
        return False

    if LICENSE not in contents:
        print('Unable to apply update to file:', os.path.relpath(path, PROJECT_ROOT))

    # update the contents of the file
    with open(path, 'w', encoding='utf-8') as output_file:
        output_file.write(contents)

    return True


def main():
    args = parse_commandline()

    if args.fix:
        routine = update_source_file
    else:
        routine = check_source_file

    results = list(map(routine, project_source_files()))

    if not all(results):
        sys.exit(1)

if __name__ == '__main__':
    main()
