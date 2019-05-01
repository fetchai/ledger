#!/usr/bin/env python3

#
# CI SCRIPT
#
# Helper script used to help make CI based build simpler.

import os
import re
import sys
import argparse
import subprocess
import fnmatch
import shutil
import multiprocessing
import xml.etree.ElementTree as ET
import run_integration_test

BUILD_TYPES = ('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')
MAX_CPUS = 7 # as defined by CI workflow
AVAILABLE_CPUS = multiprocessing.cpu_count()
CONCURRENCY = min(MAX_CPUS, AVAILABLE_CPUS)

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
        execution_time = float(test.find("./Results/NamedMeasurement[@name='Execution Time']/Value").text)
        status = test.attrib['Status']
        if status != 'passed':
            status = test.find("./Results/NamedMeasurement[@name='Exit Code']/Value").text.lower()

        output = test.find("./Results/Measurement/Value").text

        # extract the test library
        match  = re.match(r'./libs/(\w+)/tests', test_path)
        test_library = 'misc'
        if match is not None:
            test_library = match.group(1)

        # store the data
        testcases = data.get(test_library, [])
        testcases.append((test_name, test_path, test_library, execution_time, status, output))
        data[test_library] = testcases

    return data


def create_junit_format(output_path, data):
    testsuite_names = data.keys()

    testsuites = ET.Element("testsuites")

    all_tests = 0
    all_time = 0
    all_failures = 0

    for group in sorted(testsuite_names):
        group_data = data[group]

        group_size = len(group_data)
        group_time = sum(map(lambda x: x[3], group_data))
        group_failures = sum(map(lambda x: 0 if x[4] == 'passed' else 1, group_data))

        all_tests += group_size
        all_time += group_time
        all_failures ++ group_failures

        # get all the test data for this suite
        testsuite = ET.SubElement(testsuites, "testsuite", id='0', name=group, tests=str(group_size), failures=str(group_failures), time=str(group_time))

        # iterate through test cases
        for name, path, _, time, status, output in group_data:
            testcase = ET.SubElement(testsuite, "testcase", id='0', name=name, time=str(time))
            if status != 'passed':

                # extract more status
                failure = ET.SubElement(testcase, 'failure', type=status, message="Test {}".format(status))
                stdout = ET.SubElement(testcase, 'system-out').text = output

    # update the final aggregations
    testsuites.set('tests', str(all_tests))
    testsuites.set('failures', str(all_failures))
    testsuites.set('time', str(all_time))

    # generate the output file
    with open(output_path, 'w') as output_file:
        output_file.write('<?xml version="1.0" encoding="UTF-8" ?>')
        output_file.write(ET.tostring(testsuites, encoding="utf-8").decode())


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
    parser.add_argument('-I', '--integration-tests', action='store_true', help='Run the integration tests for the project')
    parser.add_argument('-f', '--force-build-folder', help='Specify the folder directly that should be used for the build / test')
    parser.add_argument('-m', '--metrics', action='store_true', help='Store the metrics.')
    return parser.parse_args()


def build_project(project_root, build_root, options):
    output('Source.:', project_root)
    output('Build..:', build_root)
    output('Options:')
    for key, value in options.items():
        output(' - {} = {}'.format(key, value))
    output('\n')

    # determine if this is the first time that we are building the project
    new_build_folder = not os.path.exists(build_root)

    # ensure the build directory exists
    os.makedirs(build_root, exist_ok=True)

    # run cmake
    cmd = ['cmake']

    # determine if this system has the ninja build system
    if new_build_folder and shutil.which('ninja') is not None:
    	cmd += ['-G', 'Ninja']

   	# add all the configuration options
    cmd += ['-D{}={}'.format(k, v) for k, v in options.items()]
    cmd += [project_root]

    # execute the cmake configurations
    exit_code = subprocess.call(cmd, cwd=build_root)
    if exit_code != 0:
        output('Failed to configure cmake project')
        sys.exit(exit_code)

    # make the project
    if os.path.exists(os.path.join(build_root, "build.ninja")):
        cmd = ["ninja"]

    else:
        # manually specifying the number of cores is required because make automatic detection is
        # flakey inside docker.
        cmd = ['make', '-j', str(CONCURRENCY)]

    output('Building project with command: {} (detected cpus: {})'.format(' '.join(cmd), AVAILABLE_CPUS))
    exit_code = subprocess.call(cmd, cwd=build_root)
    if exit_code != 0:
        output('Failed to make the project')
        sys.exit(exit_code)

# Clean files that are likely to interfere with testing
def clean_files(build_root):
    # clear all the data files which might be hanging around
    for root, _, files in os.walk(build_root):
        for path in fnmatch.filter(files, '*.db'):
            data_path = os.path.join(root, path)
            print('Removing file:', data_path)
            os.remove(data_path)

def test_project(build_root, label):
    TEST_NAME = 'Test'

    if not os.path.isdir(build_root):
        raise RuntimeError('Build Root doesn\'t exist, unable to test project')

    clean_files(build_root)

    # Python 3.7+ support need to have explicit path to application
    ctest_executable = shutil.which('ctest')

    cmd = [
        ctest_executable,
        '--no-compress-output',
        '-T', TEST_NAME,
        '-L', str(label),
    ]
    env = {"CTEST_OUTPUT_ON_FAILURE":"1"}
    exit_code = subprocess.call(cmd, cwd=build_root, env=env)

    # load the test format
    test_tag_path = os.path.join(build_root, 'Testing', 'TAG')
    if not os.path.isfile(test_tag_path):
        output('Unable to detect ctest output path:', test_tag_path)
        sys.exit(1)

    # load the tag
    tag_folder = open(test_tag_path, 'r').read().splitlines()[0]
    tag_folder_path = os.path.join(build_root, 'Testing', tag_folder, '{}.xml'.format(TEST_NAME))

    if not os.path.isfile(tag_folder_path):
        output('Unable to locate CTest summary XML:', tag_folder_path)
        sys.exit(1)

    # parse the incoming ctest xml format
    test_data = parse_ctest_format(tag_folder_path)

    # convert the data to the Jenkins Junit
    test_results = os.path.join(build_root, 'TestResults.xml')
    create_junit_format(test_results, test_data)

    if exit_code != 0:
        output('Test unsuccessful')
        sys.exit(exit_code)

def test_integration(project_root, build_root):
    yaml_file = os.path.join(project_root, "scripts/integration_test.yaml")

    # Check that the YAML file does exist
    if not os.path.exists(yaml_file):
        output('Failed to find yaml file for integration testing:')
        output(yaml_file)
        sys.exit(1)

    # should be the location of constellation exe - if not the test will catch
    constellation_exe = os.path.join(build_root, "apps/constellation/constellation")

    run_integration_test.run_test(build_root, yaml_file, constellation_exe)

def main():

    # parse the options from the command line
    args = parse_commandline()

    # define all the build roots
    project_root = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
    build_root = os.path.join(project_root, '{}{}'.format(args.build_path_prefix, args.build_type.lower()))
    if args.force_build_folder:
        build_root = os.path.abspath(args.force_build_folder)

    options = {
        'CMAKE_BUILD_TYPE': args.build_type
    }
    if args.metrics:
        options['FETCH_ENABLE_METRICS'] = 1

    if args.build:
        build_project(project_root, build_root, options)

    if args.test:
        test_project(build_root, 'Normal')

    if args.integration_tests:
        test_project(build_root, 'Slow')
        test_project(build_root, 'Integration')
        test_integration(project_root, build_root)

if __name__ == '__main__':
    main()

