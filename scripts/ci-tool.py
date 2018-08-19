#!/usr/bin/env python3

#
# CI SCRIPT
#
# Helper script used to help make CI based build simpler.
#

import os
import argparse
import subprocess


BUILD_TYPES = ('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')


def build_type(text):
    if text not in BUILD_TYPES:
        raise RuntimeError('Invalid build type {}. Choices: {}'.format(text, ','.join(BUILD_TYPES)))
    return text


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('build_type', metavar='TYPE', type=build_type, help='The type of build to be used')
    parser.add_argument('-p', '--build-path-prefix', default='build-', help='The prefix to be used for the naming of the build folder')
    parser.add_argument('-B', '--build', action='store_true', help='Build the project')
    parser.add_argument('-T', '--test', action='store_true', help='Test the project')
    return parser.parse_args()


def build_project(project_root, build_root, options):
    print('Source.:', project_root)
    print('Build..:', build_root)
    print('Options:')
    for key, value in options.items():
        print(' - {} = {}'.format(key, value))
    print('\n')

    # ensure the build directory exists
    os.makedirs(build_root, exist_ok=True)

    # run cmake
    cmd = ['cmake']
    cmd += ['-D{}={}'.format(k, v) for k, v in options.items()]
    cmd += [project_root]
    subprocess.check_call(cmd, cwd=build_root)

    # make the project
    subprocess.check_call(['make', '-j'], cwd=build_root)


def test_project(build_root):
    if not os.path.isdir(build_root):
        raise RuntimeError('Build Root doesn\'t exist, unable to test project')

    subprocess.check_call(['ctest', '--no-compress-output', '-T', 'Test'], cwd=build_root)


def main():

    # parse the options from the command line
    args = parse_commandline()

    # define all the build roots
    project_root = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
    build_root = os.path.join(project_root, '{}{}'.format(args.build_path_prefix, args.build_type.lower()))

    options = {
        'CMAKE_BUILD_TYPE': args.build_type
    }

    if args.build:
        build_project(project_root, build_root, options)

    if args.test:
        test_project(build_root)


if __name__ == '__main__':
    main()
