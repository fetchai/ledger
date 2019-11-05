import argparse
import json
import struct
import hashlib
import base64
import sys
import binascii
import base58
import time

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
    parser.add_argument('-t', '--threshold', type=float, default=0.6,
                        help='The required threshold')
    parser.add_argument('-m', '--max-cabinet', type=int,
                        help='The maximum cabinet size allowed')
    parser.add_argument('-n', '--no-formatting', dest='no_formatting', action='store_true',
                        help='Whether to format the output file for readability')
    parser.set_defaults(no_formatting=False)
    parser.add_argument('-w', '--when-start', type=int,
                        help='The genesis block has a time for the blockchain to start.  Specify how far from now in seconds it should be')
    return parser.parse_args()


def calc_resource_id(resource_address):
    hasher = hashlib.sha256()
    hasher.update(resource_address.encode())
    return base64.b64encode(hasher.digest()).decode()


def generate_token_address(muddle_address):
    raw_muddle_address = base64.b64decode(muddle_address)

    hasher1 = hashlib.sha256()
    hasher1.update(raw_muddle_address)
    token_address_base = hasher1.digest()

    hasher2 = hashlib.sha256()
    hasher2.update(token_address_base)
    token_address_checksum = hasher2.digest()[:4]

    final_address = token_address_base + token_address_checksum

    return base58.b58encode(final_address).decode()


def create_record(address, balance, stake):
    resource_id = calc_resource_id('fetch.token.state.' + address)
    resource_value = {"balance": balance, "stake": stake}
    return resource_id, resource_value

def create_genesis_file(stake_percentage, max_cabinet_size, addresses):
    # build up the stake information
    individual_balance = TOTAL_SUPPLY // len(args.addresses)
    individual_stake = (min(stake_percentage, 100)
                        * individual_balance) // 100

    max_cabinet = 0

    stakes = []
    state = []
    cabinet = []

    # build up the configuration for all the stakers
    for address in args.addresses:
        token_address = generate_token_address(address)

        # update the stake list
        stakes.append({
            'identity': address,
            'amount': individual_stake,
        })

        # update the initial state
        key, value = create_record(
            token_address, individual_balance, individual_stake)
        state.append({
            "key": key,
            **value
        })

        # update the random beacon config
        cabinet.append(address)

    start_time = 0

    if args.when_start:
        start_time = int(time.time()) + args.when_start

    # form the genesis data
    genesis_file = {
        'version': 3,
        'consensus': {
            'startTime': start_time,
            'cabinetSize': max_cabinet_size,
            'threshold': float(args.threshold),
            'stakers': stakes,
        },
        'accounts': state,
    }

    # dump the file
    with open(args.output, 'w') as output_file:
        if args.no_formatting:
            json.dump(genesis_file, output_file)
        else:
            json.dump(genesis_file, output_file, indent=4, sort_keys=True)

def main():
    args = parse_commandline()

    create_genesis_file(args.stake_percentage, args.max_cabinet, args.addresses, args.output, args.when_start)

if __name__ == '__main__':
    main()
