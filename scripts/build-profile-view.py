#!/usr/bin/env python3
import argparse
import sys
import os
import pandas as pd


def parse_commandline():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()

    parser_summary = subparsers.add_parser('summary')
    parser_summary.set_defaults(handler=run_summary)

    parser_files = subparsers.add_parser('files')
    parser_files.add_argument('--gt', type=float, help='Filter the set to files greater than a specified duration')
    parser_files.add_argument('--lt', type=float, help='Filter the set to files less than a specified duration')
    parser_files.add_argument('-L', '--library', help='The specified library to filter on')
    parser_files.add_argument('--count', action='store_true', help='Count my class instance')
    parser_files.set_defaults(handler=run_files)

    return parser, parser.parse_args()


def load_summary():
    df = pd.read_csv('build-profile-summary.csv')
    df['duration'] = df['duration_us'] / 1e6
    del df['duration_us']

    return df


def run_summary(args):
    df = load_summary()
    summ = df.groupby(['class_type', 'class_instance'])['duration'].sum().reset_index().sort_values('duration')

    print(summ[['class_instance', 'duration']].to_string(index=False))


def run_files(args):
    df = load_summary()

    if args.gt:
        df = df[df['duration'] > args.gt]
    if args.lt:
        df = df[df['duration'] < args.lt]
    if args.library:
        df = df[df['class_instance'] == args.library]

    if args.count:
        print(df.groupby(['class_instance'])['duration'].count().to_string())

    else:
        # sort values
        df = df.sort_values(['duration'])[['output_path', 'duration']]
        for _, data in df.iterrows():
            print('{:10} {}'.format(data['duration'], data['output_path']))


def main():
    exit_code = 1
    parser, args = parse_commandline()

    if not os.path.exists('build-profile-summary.csv'):
        print('Please run in the same folder as the build summary')
        sys.exit(1)


    try:
        if hasattr(args, 'handler'):
            args.handler(args)
            exit_code = 0
        else:
            parser.print_usage()    
    except:
        pass

    if exit_code != 0:
        sys.exit(exit_code)


if __name__ == '__main__':
    main()
