#!/usr/bin/env python3
import argparse
import json
import base64
import sys
import time

from fetchai.ledger.crypto import Identity, Address


TOTAL_SUPPLY = 1152997575
DEFAULT_STAKE = 1000
TEN_ZERO = 10000000000
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
    parser.add_argument(
        '-o', '--output', default='genesis_file.json', help='Path to generated file')
    parser.add_argument('-w', '--when-start', required=True, type=int,
                        help='The genesis block has a time for the blockchain to start.  Specify how far from now in seconds it should be')
    return parser.parse_args()


def main():
    args = parse_commandline()

    max_cabinet = len(args.addresses)

    # build up the configuration for all the stakers
    accounts = []
    stakers = []
    for n, address in enumerate(args.addresses):
        identity = Identity.from_base64(address)
        if n == 0:
            balance = TOTAL_SUPPLY - (max_cabinet * DEFAULT_STAKE)
        else:
            balance = 0

        accounts.append({
            'address': str(Address(identity)),
            'balance': balance,
            'stake': DEFAULT_STAKE,
        })
        stakers.append({
            'identity': identity.public_key,
            'amount': DEFAULT_STAKE * TEN_ZERO,
        })

    genesis = {
        'version': 4,
        'accounts': accounts,
        'consensus': {
            'aeonOffset': 100,
            'aeonPeriodicity': 25,
            'cabinetSize': max_cabinet,
            'entropyRunahead': 2,
            'minimumStake': DEFAULT_STAKE,
            'startTime': int(time.time()) + args.when_start,
            'stakers': stakers,
        }
    }

    # create the genesis configuration
    with open(args.output, 'w') as genesis_file:
        json.dump(genesis, genesis_file)


if __name__ == '__main__':
    main()
