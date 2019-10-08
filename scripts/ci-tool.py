#!/usr/bin/env python3

# CI SCRIPT
#
# Helper script used to help make CI based build simpler.

import argparse
import fnmatch
import multiprocessing
import os
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
from os.path import abspath, dirname, exists, isdir, isfile, join

import fetchai_code_quality

BUILD_TYPES = ('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')
MAX_CPUS = 7  # as defined by CI workflow
AVAILABLE_CPUS = multiprocessing.cpu_count()
CONCURRENCY = min(MAX_CPUS, AVAILABLE_CPUS)

SCRIPT_ROOT = dirname(abspath(__file__))

SLOW_TEST_LABEL = 'Slow'
INTEGRATION_TEST_LABEL = 'Integration'

LABELS_TO_EXCLUDE_FOR_FAST_TESTS = [
    SLOW_TEST_LABEL,
    INTEGRATION_TEST_LABEL]

NINJA_IS_PRESENT = shutil.which('ninja') is not None


def output(*args):
    text = ' '.join(map(str, args))
    if text != '':
        sys.stdout.write(text)
        sys.stdout.write('\n')
        sys.stdout.flush()


def parse_ctest_format(path):
    tree = ET.parse(path)
    root = tree.getroot()

    # loop through all of the tests
    data = {}
    for test in root.findall('./Testing/Test'):

        # extract all the information required
        test_name = test.find('./Name').text
        test_path = test.find('./Path').text
        execution_time = float(test.find(
            "./Results/NamedMeasurement[@name='Execution Time']/Value").text)
        status = test.attrib['Status']
        if status != 'passed':
            status = test.find(
                "./Results/NamedMeasurement[@name='Exit Code']/Value").text.lower()

        output = test.find('./Results/Measurement/Value').text

        # extract the test library
        match = re.match(r'./libs/(\w+)/tests', test_path)
        test_library = 'misc'
        if match is not None:
            test_library = match.group(1)

        # store the data
        testcases = data.get(test_library, [])
        testcases.append((test_name, test_path, test_library,
                          execution_time, status, output))
        data[test_library] = testcases

    return data


def create_junit_format(output_path, data):
    testsuite_names = data.keys()

    testsuites = ET.Element('testsuites')

    all_tests = 0
    all_time = 0
    all_failures = 0

    for group in sorted(testsuite_names):
        group_data = data[group]

        group_size = len(group_data)
        group_time = sum(map(lambda x: x[3], group_data))
        group_failures = sum(
            map(lambda x: 0 if x[4] == 'passed' else 1, group_data))

        all_tests += group_size
        all_time += group_time
        all_failures += group_failures

        # get all the test data for this suite
        testsuite = ET.SubElement(
            testsuites,
            'testsuite',
            id='0',
            name=group,
            tests=str(group_size),
            failures=str(group_failures),
            time=str(group_time))

        # iterate through test cases
        for name, path, _, time, status, output in group_data:
            testcase = ET.SubElement(
                testsuite, 'testcase', id='0', name=name, time=str(time))
            if status != 'passed':
                # extract more status
                failure = ET.SubElement(
                    testcase,
                    'failure',
                    type=status,
                    message='Test {status}'.format(status=status))
                stdout = ET.SubElement(testcase, 'system-out').text = output

    # update the final aggregations
    testsuites.set('tests', str(all_tests))
    testsuites.set('failures', str(all_failures))
    testsuites.set('time', str(all_time))

    # generate the output file
    with open(output_path, 'w') as output_file:
        output_file.write('<?xml version="1.0" encoding="UTF-8" ?>')
        output_file.write(ET.tostring(testsuites, encoding='utf-8').decode())


def build_type(text):
    if text not in BUILD_TYPES:
        raise RuntimeError(
            'Invalid build type {text}. Choices: {build_types}'.format(text=text, build_types=", ".join(BUILD_TYPES)))
    return text


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('build_type', metavar='TYPE',
                        type=build_type, help='The type of build to be used')
    parser.add_argument(
        '-p', '--build-path-prefix', default='build-',
        help='The prefix to be used for the naming of the build folder')
    parser.add_argument('-B', '--build', action='store_true',
                        help='Build the project')
    parser.add_argument('-j', '--jobs', type=int, default=CONCURRENCY,
                        help=('The number of jobs to do in parallel. If \'0\' then number of '
                              'available CPU cores will be used. '
                              'Defaults to {CONCURRENCY}'.format(CONCURRENCY=CONCURRENCY)))
    parser.add_argument('-T', '--test', action='store_true',
                        help='Run unit tests. Skips tests marked with the following CTest '
                             'labels: {labels}'.format(labels=", ".join(LABELS_TO_EXCLUDE_FOR_FAST_TESTS)))
    parser.add_argument('-S', '--slow-tests', action='store_true',
                        help='Run tests marked with the \'{SLOW_TEST_LABEL}\' CTest label'.format(
                            SLOW_TEST_LABEL=SLOW_TEST_LABEL))
    parser.add_argument('-I', '--integration-tests', action='store_true',
                        help='Run tests marked with the \'{INTEGRATION_TEST_LABEL}\' CTest label'.format(
                            INTEGRATION_TEST_LABEL=INTEGRATION_TEST_LABEL))
    parser.add_argument('-E', '--end-to-end-tests', action='store_true',
                        help='Run the end-to-end tests for the project')
    parser.add_argument('-L', '--language-tests', action='store_true',
                        help='Run the etch language tests')
    parser.add_argument('--lint', action='store_true',
                        help='Run clang-tidy')
    parser.add_argument('--fix', action='store_true',
                        help='Run clang-tidy and attempt to fix lint errors')
    parser.add_argument('-c', '--commit', nargs=1, default=None,
                        help='when linting, including fixing, scan and fix only files that changed between Git\'s HEAD and the ' 'given commit or ref. \nUseful: HEAD will lint staged files')
    parser.add_argument('-A', '--all', action='store_true',
                        help='Run build and all tests')
    parser.add_argument(
        '-f', '--force-build-folder',
        help='Specify the folder directly that should be used for the build / test')
    parser.add_argument('-m', '--metrics',
                        action='store_true', help='Store the metrics')

    return parser.parse_args()


def cmake_configure(project_root, build_root, options, generator):
    output('Source.:', project_root)
    output('Build..:', build_root)
    output('Options:')
    for key, value in options.items():
        output(' - {key} = {value}'.format(key=key, value=value))
    output('\n')

    # ensure the build directory exists
    os.makedirs(build_root, exist_ok=True)

    cmake_cmd = ['cmake']
    cmake_cmd += ['-G', generator]

    # add all the configuration options
    cmake_cmd += ['-D{k}={v}'.format(k=k, v=v) for k, v in options.items()]
    cmake_cmd += [project_root]

    # execute the cmake configurations
    exit_code = subprocess.call(cmake_cmd, cwd=build_root)
    return exit_code == 0


def build_project(build_root, concurrency):
    build_cmd = ['ninja'] if exists(
        join(build_root, 'build.ninja')) else ['make']
    build_cmd += ['-j{concurrency}'.format(concurrency=concurrency)]

    output(
        'Building project with command: {cmd} (detected cpus: {AVAILABLE_CPUS})'.format(cmd=" ".join(build_cmd),
                                                                                        AVAILABLE_CPUS=AVAILABLE_CPUS))
    exit_code = subprocess.call(build_cmd, cwd=build_root)
    if exit_code != 0:
        output('Failed to make the project')
        sys.exit(exit_code)


def clean_files(build_root):
    """Clean files that are likely to interfere with testing"""
    for root, _, files in os.walk(build_root):
        for path in fnmatch.filter(files, '*.db'):
            data_path = join(root, path)
            output('Removing file:', data_path)
            os.remove(data_path)


def test_project(build_root, include_regex=None, exclude_regex=None):
    TEST_NAME = 'Test'

    if not isdir(build_root):
        raise RuntimeError('Build Root doesn\'t exist, unable to test project')

    clean_files(build_root)

    # Python 3.7+ support need to have explicit path to application
    ctest_executable = shutil.which('ctest')

    cmd = [
        ctest_executable,
        '--output-on-failure',
        '-T', TEST_NAME
    ]

    if include_regex is not None:
        cmd = cmd + ['-L', str(include_regex)]
    if exclude_regex is not None:
        cmd = cmd + ['-LE', str(exclude_regex)]

    exit_code = subprocess.call(cmd, cwd=build_root)

    # load the test format
    test_tag_path = join(build_root, 'Testing', 'TAG')
    if not isfile(test_tag_path):
        output('Unable to detect ctest output path:', test_tag_path)
        sys.exit(1)

    # load the tag
    tag_folder = open(test_tag_path, 'r').read().splitlines()[0]
    tag_folder_path = join(build_root, 'Testing',
                           tag_folder, '{TEST_NAME}.xml'.format(TEST_NAME=TEST_NAME))

    if not isfile(tag_folder_path):
        output('Unable to locate CTest summary XML:', tag_folder_path)
        sys.exit(1)

    # parse the incoming ctest xml format
    test_data = parse_ctest_format(tag_folder_path)

    # convert the data to the Jenkins Junit
    test_results = join(build_root, 'TestResults.xml')
    create_junit_format(test_results, test_data)

    if exit_code != 0:
        output('Test unsuccessful')
        sys.exit(exit_code)


def test_end_to_end(project_root, build_root):
    from end_to_end_test import run_end_to_end_test

    yaml_file = join(
        project_root, 'scripts/end_to_end_test/end_to_end_test.yaml')

    # Check that the YAML file does exist
    if not exists(yaml_file):
        output('Failed to find yaml file for end_to_end testing:')
        output(yaml_file)
        sys.exit(1)

    # should be the location of constellation exe - if not the test will catch
    constellation_exe = join(build_root, 'apps/constellation/constellation')

    clean_files(build_root)

    run_end_to_end_test.run_test(build_root, yaml_file, constellation_exe)


def test_language(build_root):
    LANGUAGE_TEST_RUNNER = join(SCRIPT_ROOT, 'run-language-tests.py')
    cmd = [LANGUAGE_TEST_RUNNER, build_root]
    subprocess.check_call(cmd)


def main():
    # parse the options from the command line
    args = parse_commandline()

    # manually specifying the number of cores is required because make automatic detection is
    # flakey inside docker.
    concurrency = AVAILABLE_CPUS if args.jobs == 0 else args.jobs

    # define all the build roots
    project_root = abspath(dirname(dirname(__file__)))
    build_root = join(
        project_root, '{prefix}{build_type}'.format(prefix=args.build_path_prefix, build_type=args.build_type.lower()))
    if args.force_build_folder:
        build_root = abspath(args.force_build_folder)

    options = {
        'CMAKE_BUILD_TYPE': args.build_type,
    }

    if args.build or args.lint or args.all:
        # choose the generater initially based on what already exists there
        if isdir(build_root):
            if isfile(join(build_root, 'Makefile')):
                generator = 'Unix Makefiles'
            elif isfile(join(build_root, 'build.ninja')):
                generator = 'Ninja'
            elif isfile(join(build_root, 'CMakeCache.txt')):
                # in the case of a failed build it might be necessary to interrogate the cache file
                with open(join(build_root, 'CMakeCache.txt'), 'r') as cache_file:
                    for line in cache_file:
                        match = re.match(
                            r'^CMAKE_GENERATOR:INTERNAL=(.*)', line.strip())
                        if match is not None:
                            generator = match.group(1)
                            break

            else:
                raise RuntimeError('Unable to detect existing generator type')
        else:

            # in the case of a new build prefer Ninja over make (because it is faster)
            generator = 'Ninja' if NINJA_IS_PRESENT else 'Unix Makefiles'

        # due to the version of the header dependency detection that is used when trying to
        # determine affected files, when running commit filtered lints must use make
        if generator == 'Ninja' and args.commit is not None:
            generator = 'Unix Makefiles'

        # configure the project
        if not cmake_configure(project_root, build_root, options, generator):
            output('\n😭 Failed to configure the cmake project. This is usually because of a mismatch between generators.\n\nTry removing the build folder: {} and try again'.format(build_root))
            sys.exit(1)

    if args.build or args.all or args.commit:
        build_project(build_root, concurrency)

    if args.test or args.all:
        test_project(
            build_root,
            exclude_regex='|'.join(LABELS_TO_EXCLUDE_FOR_FAST_TESTS))

    if args.language_tests or args.all:
        test_language(build_root)

    if args.slow_tests or args.all:
        test_project(
            build_root,
            include_regex=SLOW_TEST_LABEL)

    if args.integration_tests or args.all:
        test_project(
            build_root,
            include_regex=INTEGRATION_TEST_LABEL)

    if args.end_to_end_tests or args.all:
        test_end_to_end(project_root, build_root)

    if args.lint or args.all:
        fetchai_code_quality.static_analysis(
            project_root, build_root, args.fix, concurrency, args.commit, verbose=False)


if __name__ == '__main__':
    main()
