import os
import sys
import subprocess
import argparse
import matplotlib.pyplot as plt
import pandas as pd
import csv


def remove_sysinfo(csv_file):
    new_csv_file = '{}_wo_sysinfo.csv'.format(os.path.splitext(csv_file)[0])

    with open(csv_file, 'r') as original:
        lines = original.read().splitlines(True)
    with open(new_csv_file, 'w') as new:
        counter = 0
        for line in lines:
            if line[0:4] == "name":
                new.writelines(lines[counter:])
                break
            counter += 1
    return new_csv_file


def compute_time_charge_corr(csv_file_path, stats_file_path):
    df = pd.read_csv(csv_file_path)

    correlation_coef = df['cpu_time'].corr(df['charge'])
    time_charge_factors = df['charge'].divide(df['cpu_time'])
    time_charge_factor_avg = time_charge_factors.mean()

    pd.options.display.max_rows = sys.maxsize

    with open(stats_file_path, 'w') as out:
        print(correlation_coef)
        out.write(str(correlation_coef)+os.linesep)
        print(time_charge_factor_avg)
        out.write(str(time_charge_factor_avg)+os.linesep)
        print(time_charge_factors)
        out.writelines(str(time_charge_factors))


def plot_time_charge_corr(csv_file_path, output_img_path):
    df = pd.read_csv(csv_file_path)

    figure = plt.figure(figsize=(12, 12), dpi=100)
    axe = figure.add_subplot(111)

    plt.title('Average correlation factor -- 1 CHARGE_UNIT = {} unit of time'.format(
        df['charge'].divide(df['cpu_time']).mean()))

    axe.plot(df['cpu_time'])
    axe.set_ylabel('cpu_time')
    axe.set_yscale("log")
    axe.set_xticklabels(df['name'], rotation='vertical')

    axe2 = axe.twinx()
    axe2.plot(df['charge'], '-r')
    axe2.set_ylabel('charge_amount', color='r')
    axe2.set_yscale("log")

    plt.tight_layout()
    plt.savefig(output_img_path)


def run_benchmark(bench_binary, bench_name, output_dir):

    csvfile = os.path.join(output_dir, 'bm_{}.csv'.format(bench_name))
    pngfile = os.path.join(output_dir, 'bm_{}.png'.format(bench_name))
    statsfile = os.path.join(output_dir, 'bm_{}.stats'.format(bench_name))

    cmd = [
        bench_binary,
        '--benchmark_counters_tabular=true',
        '--benchmark_out_format=csv',
        '--benchmark_out={}'.format(csvfile),
        '--benchmark_filter={}'.format(bench_name)
    ]

    # run benchmark
    process = subprocess.Popen(cmd)
    process_status = process.wait()

    # process csv file
    csvfile = remove_sysinfo(csvfile)
    plot_time_charge_corr(csvfile, pngfile)
    compute_time_charge_corr(csvfile, statsfile)


def verify_file(filename):
    if not os.path.isfile(filename):
        print("Couldn't find expected file: {}".format(filename))
        sys.exit(1)


def verify_dir(dirname):
    if not os.path.isdir(dirname):
        print("Couldn't find expected directory: {}".format(dirname))
        sys.exit(1)


def parse_command_line():
    parser = argparse.ArgumentParser(
        description='Run Google Benchmarks of ML Etch operation charges and execution time, report stats')

    parser.add_argument('build_dir', type=str,
                        help='Location of ledger build directory')
    parser.add_argument('output_dir', type=str,
                        help='Output directory for VM execution Charge/Time statistics')

    return parser.parse_args()


def main():
    args = parse_command_line()

    # cli args
    build_dir = os.path.abspath(args.build_dir)
    output_dir = os.path.abspath(args.output_dir)

    # benchmark config
    bench_binary = os.path.join(
        build_dir, './libs/vm-modules/benchmark/benchmark_vm_modules_tensor')

    verify_file(bench_binary)
    verify_dir(output_dir)

    benchmark_functions = [
        'BM_Construct',
        'BM_Fill',
        'BM_FillRandom',
        'BM_Reshape',
        'BM_Transpose',
        'BM_At',
        'BM_SetAt',
        'BM_ToString',
        'BM_FromString',
        'BM_Min',
        'BM_Max',
        'BM_Sum',
        'BM_ArgMax',
        'BM_ArgMaxNoIndices',
        'BM_Dot',
        'BM_IsEqual',
        'BM_IsNotEqual',
        'BM_Negate',
        'BM_Add',
        'BM_Multiply',
        'BM_Divide',
        'BM_InplaceAdd',
        'BM_InplaceSubtract',
        'BM_InplaceMultiply',
        'BM_InplaceDivide',
        'BM_Copy',

        # , ''  # all
    ]

    for benchname in benchmark_functions:
        run_benchmark(bench_binary, benchname, output_dir)


if __name__ == '__main__':
    main()
