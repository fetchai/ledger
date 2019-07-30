#!/usr/bin/env python3

import sys
import os
import argparse

from fetchai.ledger.api import LedgerApi
from fetchai.ledger.crypto import Entity


def output(*args):
    text = ' '.join(map(str, args))
    if text != '':
        sys.stdout.write(text)
        sys.stdout.write('\n')
        sys.stdout.flush()


def create_files(identities_to_create):

    this_file_dir = os.path.dirname(os.path.realpath(__file__))
    file_info = ""

    for index in range(identities_to_create):
        entity = Entity()

        with open(this_file_dir+"/{}.key".format(index), 'wb+') as f:
            f.write(entity.private_key_bytes)

        file_info += "{} : {}\n".format(index, entity.public_key)

    with open(this_file_dir+"/info.txt", 'w+') as f:
        f.write(file_info)


def parse_commandline():
    parser = argparse.ArgumentParser(
        description='Create input files used for the end to end tests')

    # Required argument
    parser.add_argument(
        'identities', type=int, default=200,
        help='Number of identities to create')

    return parser.parse_args()


def main():
    args = parse_commandline()

    return create_files(args.identities)


if __name__ == '__main__':
    main()
