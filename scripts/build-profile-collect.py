#!/usr/bin/env python3
import argparse
import os
import fnmatch
import json

from pprint import pprint


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('build_folder', type=os.path.abspath,
                        help='The build folder to scan')
    parser.add_argument('-o', '--output-path',
                        default='build-profile-summary.csv', help='Output path for CSV file')
    return parser.parse_args()


def main():
    args = parse_commandline()

    profile_points = []
    for root, _, files in os.walk(args.build_folder):
        for path in fnmatch.filter(files, '*.prof-json'):
            file_path = os.path.join(root, path)
            with open(file_path, 'r') as profile_file:
                profile = json.load(profile_file)

                ouput_file_path = os.path.join(
                    profile['workingDir'], profile['outputPath'])
                ouput_file_path = os.path.relpath(
                    ouput_file_path, args.build_folder)

                if profile['exitCode'] != 0:
                    print('Skipping: {} (process failed)')
                    continue

                if ouput_file_path.endswith('.o'):
                    operation = 'compile'
                else:
                    operation = 'link'

                # determine the library
                output_path_tokens = ouput_file_path.split(os.sep)

                if output_path_tokens[0] == 'vendor':
                    class_type = 'vendor'
                    class_instance = '/'.join(output_path_tokens[:2])
                elif output_path_tokens[0] == 'libs':
                    class_type = 'library'
                    class_instance = output_path_tokens[1]
                elif output_path_tokens[0] == 'lib' or output_path_tokens[0] == 'bin':
                    continue  # this is generally the openssl / MCL build art

                profile_points.append(
                    (operation, class_type, class_instance, ouput_file_path, profile['duration']))

    with open(args.output_path, 'w') as output_file:
        headers = (
            'operation',
            'class_type',
            'class_instance',
            'output_path',
            'duration_us',
        )

        output_file.write(','.join(map(str, headers)) + '\n')
        for point in profile_points:
            output_file.write(','.join(map(str, point)) + '\n')


if __name__ == '__main__':
    main()
