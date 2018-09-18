#!/usr/bin/env python3

#
# Generate coverage html for unit tests.
#
# Best done in the docker containter.
#
# Before running this file, create a build folder and build the tests you want coverage of, in debug
# mode
#
# On successful completion, you should be able to view ./build/coverage/coverage_XXX/index.html in
# a browser

import os
import sys
import subprocess
import pdb
import re
import pprint

pp = pprint.PrettyPrinter(indent=4)

def create_empty_dir(dirname):
    try:
        os.rmdir(dirname)
    except:
        pass

    try:
        os.mkdir(dirname)
    except:
        pass

def get_coverage_reports(targets : map):
    for key, val in targets.items():
        os.system("rm -f default.profraw")
        print("Executing: " + key)
        print(val)
        if not (os.system(val) == 0):
            print("Failed when executing: "+val)

        raw_file_name     = key +".profraw"
        indexed_file_name = key +".profdata"
        directory_name    = R"coverage/"+key +"_coverage"

        if not (os.system("mv default.profraw " + raw_file_name) == 0):
            print("Failed to find output file default.profraw - what's your LLVM_PROFILE_FILE?")
            print("Did you build in debug mode?")

        os.system("llvm-profdata merge -sparse  "+raw_file_name+" -o " + indexed_file_name)
        os.system("llvm-cov show -Xdemangler c++filt "+val+" -instr-profile="+indexed_file_name + " -format=html -o " +directory_name)

# This isn't enabled yet since it will take too long
def get_all_coverage_reports(targets : map):
    all_profs = ""
    for key, val in targets.items():
        raw_file_name     = key +".profraw"
        indexed_file_name = key +".profdata"
        all_profs += indexed_file_name + " "

def main():

    if(len(sys.argv) < 2):
        print("Usage: ")
        print(sys.argv[0] + " ", end='')
        print("./build [test_name]")
        sys.exit(1)

    build_directory   = sys.argv[1]
    test_name         = sys.argv[2] if len(sys.argv) > 2 else None
    current_directory = os.getcwd()

    os.chdir(build_directory)

    # Run
    print("Checking ctest for targets at: "+os.getcwd())
    popen = subprocess.Popen(["ctest", "-N", "--debug"],  stdout=subprocess.PIPE, cwd=os.getcwd())

    # Note a wait here will deadlock a docker container. arg 0 = stdout
    output = popen.communicate()[0]
    output = output.decode("utf-8")

    error_code = popen.returncode

    if not (error_code == 0):
        print("Failed to run cmake.")
        sys.exit(1)
    else:
        print("Successfully ran cmake")

    # Determine the targets and their directories
    targets = {}

    for line in output.split('\n'):
        search = re.search("Test command: ", line)

        if not search == None:
            command = line.split("Test command: ")[1]
            target = command.split(R"/")[-1]
            targets[target] = command

    if len(targets) == 0:
        print("failed to find targets in output of cmake command")
        sys.exit(1)

    # If we have specified a target, only generate a profile for that one
    if test_name != None and test_name not in targets:
        print(test_name + " not found in targets: ")
        pp.pprint(targets)
        sys.exit(1)

    if test_name != None:
        temp = targets[test_name]
        targets.clear()
        targets[test_name] = temp
        pp.pprint(targets)

    # create empty coverage dir
    create_empty_dir("coverage")

    get_coverage_reports(targets)
    get_all_coverage_reports(targets)

if __name__ == '__main__':
    main()
