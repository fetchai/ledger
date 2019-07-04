#!/usr/bin/env python3
import os
import sys
import fnmatch
import argparse
import subprocess
import tempfile

SCRIPT_ROOT = os.path.dirname(os.path.abspath(__file__))
TESTS_ROOT = os.path.join(SCRIPT_ROOT, 'etch-language-tests')


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('build_folder', type=os.path.abspath,
                        help='The path to the build folder')
    return parser.parse_args()


def main():
    args = parse_commandline()

    etch_cli = os.path.join(args.build_folder, 'apps', 'etch', 'etch')
    assert os.path.isfile(etch_cli), 'etch executable not found at {}'.format(etch_cli)

    tests = []

    # discover all the tests
    for root, _, files in os.walk(TESTS_ROOT):
        for path in fnmatch.filter(files, '*.etch'):
            full_path = os.path.join(root, path)
            name, _ = os.path.splitext(os.path.relpath(full_path, TESTS_ROOT))
            tests.append((name, full_path))

    max_test_size = max([len(n) for n, _ in tests]) + 1

    print()
    print('=== ETCH Language Tests ===')
    print()

    # loop through and execute each one of them
    failures = []
    for name, path in sorted(tests, key=lambda x: x[0]):

        # create a temp file and call the executable inside it
        output_file, output_filename = tempfile.mkstemp(
            prefix='etch-lang-tests-', suffix='.tmp')
        exit_code = subprocess.call(
            [etch_cli, path], stdout=output_file, stderr=subprocess.STDOUT)
        os.close(output_file)

        result = 'success' if exit_code == 0 else 'failed'
        padding = ' ' * (max_test_size - len(name))
        print('{}{}: {}'.format(name, padding, result))

        # on error print the contents of the generated file
        if exit_code != 0:
            failures.append(name)

            print('\n------------------ TEST OUTPUT ------------------')
            with open(output_filename, 'r') as log_file:
                print(log_file.read())
            print()

        # remove the temporary file
        os.remove(output_filename)

    # print the summary
    if len(failures) > 0:
        print('The following tests were unsuccessful:')
        for name in failures:
            print('-', name)
        sys.exit(1)
    else:
        print()
        print('All tests passed!')

    print()


if __name__ == '__main__':
    main()
