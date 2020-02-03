#!/usr/bin/env python3
import sys
import os
import argparse
import time
import subprocess
import json
from datetime import datetime


def parse_commandline():
    if '--' in sys.argv:
        idx = sys.argv.index('--')
    else:
        idx = 0

    cmd = sys.argv[idx + 1:]

    # attempt to determine output file
    if '-o' in cmd:
        output_path = cmd[cmd.index('-o') + 1]
    else:
        output_path = cmd[-1]

    return None, cmd, output_path


def main():
    args, cmd, output_path = parse_commandline()

    start = datetime.now()
    exit_code = subprocess.call(cmd)
    finish = datetime.now()

    delta = int((finish - start).total_seconds() * 1e6)

    profile_path = output_path + '.prof-json'
    with open(profile_path, 'w') as profile_file:
        profile = {
            'cmd': cmd,
            'startTime': start.isoformat(),
            'endTime': finish.isoformat(),
            'outputPath': output_path,
            'duration': delta,
            'exitCode': exit_code,
            'workingDir': os.getcwd(),
        }
        json.dump(profile, profile_file)

    # print('\n*** Elapsed Time: {} us - {}'.format(delta, output_path))

    if exit_code != 0:
        sys.exit(exit_code)


if __name__ == '__main__':
    main()
