#!/usr/bin/env python3

#
# Generate coverage html for any binary (must be build in debug mode)
#
# Refer to generate_coverage.py for detailed instructions

from generate_coverage import *

def main():

    if(len(sys.argv) < 3):
        print("Usage: ")
        print(sys.argv[0] + " ", end='')
        print("./build ./build/path/test_name")
        sys.exit(1)

    build_directory = sys.argv[1]
    executable      = "../" + sys.argv[2]
    executable_name = executable.split(R"/")[-1]

    targets = {}
    targets[executable_name] = executable

    pp.pprint(targets)

    os.chdir(build_directory)

    # create empty coverage dir
    create_empty_dir("coverage")

    get_coverage_reports(targets)

if __name__ == '__main__':
    main()
