#!/usr/bin/env python3
import argparse
import json
import struct
import hashlib
import base64

import binascii

TOTAL_SUPPLY = 11529975750000000000


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('address', help='The initial staker address')
    parser.add_argument('stake', nargs='?', type=int, default=TOTAL_SUPPLY // 100, help='The initial stake amount')
    parser.add_argument('-o', '--output', default='snapshot.json', help='Path to generated file')
    return parser.parse_args()


def calc_resource_id(resource_address):
    hasher = hashlib.sha256()
    hasher.update(resource_address.encode())
    return base64.b64encode(hasher.digest()).decode()


def main():
    args = parse_commandline()

    # calculate what the current balance is
    stake = args.stake
    balance = TOTAL_SUPPLY - stake

    # calculate the entry in the state database
    resource_id = calc_resource_id('fetch.token.state.' + args.address)
    resource_value = base64.b64encode(struct.pack('<QQB', balance, stake, 0)).decode()

    # form the snapshot data
    snapshot = {
        'version': 1,
        'stake': {
            'committeeSize': 1,
            'stakes': [{
                'address': args.address,
                'amount': stake,
            }]
        },
        'state': {
            resource_id: resource_value,
        }
    }

    # dump the file
    with open(args.output, 'w') as output_file:
        json.dump(snapshot, output_file)


if __name__ == '__main__':
    main()
