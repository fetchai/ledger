#!/usr/bin/env python3
import argparse
import requests
import sys


LEVELS = ('trace', 'debug', 'info', 'warning', 'error', 'critical')


def _level(text):
    if text is None:
        return None

    candidates = []
    for candidate in LEVELS:
        if text.lower() in candidate:
            candidates.append(candidate)

    if len(candidates) != 1:
        print('Unable to determine level')
        sys.exit(1)

    return candidates[0]


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('logger', nargs='?', help='The target logger')
    parser.add_argument('level', nargs='?', type=_level,
                        help='The level to be set')
    parser.add_argument('-H', '--host', default='127.0.0.1',
                        help='The target host')
    parser.add_argument('-P', '--port', type=int,
                        default=8000, help='The tareget port')
    return parser.parse_args()


def main():
    args = parse_commandline()
    s = requests.session()

    print(args)
    print()

    # define the URL
    url = 'http://{}:{}/api/logging/'.format(args.host, args.port)

    # get the logging map
    logging_map = s.get(url).json()
    logger_width = max([len(x) for x in logging_map.keys()])

    if args.level is None:
        for logger, level in logging_map.items():
            if args.logger and args.logger.lower() not in logger.lower():
                continue

            padding = ' ' * (logger_width - len(logger))
            print('{}{} : {}'.format(logger, padding, level))

    else:
        assert args.logger is not None

        updates = {}
        for logger in logging_map.keys():
            if args.logger.lower() in logger.lower():
                updates[logger] = args.level

                padding = ' ' * (logger_width - len(logger))
                print('{}{} = {}'.format(logger, padding, args.level))

        s.patch(url, json=updates).json()


if __name__ == '__main__':
    main()
