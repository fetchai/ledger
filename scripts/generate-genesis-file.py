#!/usr/bin/env python3
import argparse
import json
import struct
import hashlib
import base64
import sys
import binascii
import base58
import time

from fetchai.ledger.crypto import Identity
from fetchai.ledger.genesis import GenesisFile


TOTAL_SUPPLY = 11529975750000000000
MUDDLE_ADDDRESS_RAW_LENGTH = 64


def _muddle_address(text):
    raw = base64.b64decode(text)
    if len(raw) != MUDDLE_ADDDRESS_RAW_LENGTH:
        print('Incorrect identity length')
        sys.exit(1)
    return text


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('addresses', nargs='+', type=_muddle_address,
                        help='The initial set of base64 encoded muddle addresses')
    parser.add_argument('-s', '--stake-percentage', nargs='?', type=int,
                        default=1, help='The percentage of tokens to be staked')
    parser.add_argument(
        '-o', '--output', default='genesis_file.json', help='Path to generated file')
    parser.add_argument('-t', '--threshold', type=float,
                        default=0.6, help='The required threshold')
    parser.add_argument('-m', '--max-cabinet', type=int,
                        help='The maximum cabinet size allowed')
    parser.add_argument('-n', '--no-formatting', action='store_true',
                        help='Whether to format the output file for readability')
    parser.add_argument('-b', '--block-interval', type=int,
                        default=8000, help='The block interval for the chain')
    parser.add_argument('-w', '--when-start', required=True, type=int,
                        help='The genesis block has a time for the blockchain to start.  Specify how far from now in seconds it should be')
    return parser.parse_args()


def main():
    args = parse_commandline()

    # build up the stake information
    individual_balance = TOTAL_SUPPLY // len(args.addresses)
    individual_stake = (min(args.stake_percentage, 100)
                        * individual_balance) // 100

    max_cabinet = 0
    if not args.max_cabinet:
        max_cabinet = len(args.addresses)
    else:
        max_cabinet = args.max_cabinet

    # build up the configuration for all the stakers
    members = []
    for address in args.addresses:
        members.append((
            Identity.from_base64(address),
            individual_balance,
            individual_stake,
        ))

    # create the genesis configuration
    genesis = GenesisFile(
        members,
        max_cabinet,
        args.when_start,
        args.block_interval
    )

    # flush the file to disk
    genesis.dump_to_file(args.output, args.no_formatting)


if __name__ == '__main__':
    main()
