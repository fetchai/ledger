import os
import sys
import subprocess
import argparse
import matplotlib.pyplot as plt
import pandas as pd
import csv


def new_file_wo_system_info(original_csv_file):
    # TOFIX should use os join
    # processed_csv_file = os.path.join(os.path.splitext(original_csv_file)[
    #    0], "_wosysinfo.csv")
    processed_csv_file = os.path.splitext(original_csv_file)[
        0]+"_wo_sysinfo.csv"
    with open(original_csv_file, 'r') as original:
        lines = original.read().splitlines(True)
    with open(processed_csv_file, 'w') as processed:
        counter = 0
        for line in lines:
            if line[0:4] == "name":
                processed.writelines(lines[counter:])
                break
            #print('skipping line {}'.format(line))
            counter += 1
    return processed_csv_file


def print_exec_time_charge_corr(csv_file_path, stats_file_path):
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


def plot_exec_time_charge(csv_file_path, output_img_path):
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
    # name = os.path.join(root, os.path.splitext(
    #    csv_file_path)[0]+"_time_charge.png")
    plt.savefig(output_img_path)


def run_benchmark(bench_binary, bench_name, output_dir):
    print('{} {} {}'.format(bench_binary, bench_name, output_dir))

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
    # cmd, stdout=subprocess.STDOUT, stderr=subprocess.STDOUT)
    process_status = process.wait()

    # process csv file
    csvfile = new_file_wo_system_info(csvfile)
    plot_exec_time_charge(csvfile, pngfile)
    print_exec_time_charge_corr(csvfile, statsfile)


def parse_command_line():
    parser = argparse.ArgumentParser(
        description='Run Google Benchmarks of ML Etch operation charges and execution time, report stats')

    parser.add_argument('root_dir', type=str,
                        help='Location of ledger root directory')
    parser.add_argument('build_dir', type=str,
                        help='Location of ledger build directory')
    parser.add_argument('output_dir', type=str,
                        help='Output directory for GBench statistics')

    return parser.parse_args()


def main():
    args = parse_command_line()

    # cli args
    root_dir = os.path.abspath(args.root_dir)
    build_dir = os.path.abspath(args.build_dir)
    output_dir = os.path.abspath(args.output_dir)

    # bench config
    bench_binary = os.path.join(
        build_dir, './libs/vm-modules/benchmark/benchmark_vm_modules_model_charge')

    benchmarks = [
        'BM_AddLayer',
        'BM_Predict',
        'BM_Compile',
        'BM_Fit',
        'BM_SerializeToString',
        'BM_DeserializeFromString',
        ''
    ]

    for bench in benchmarks:
        run_benchmark(bench_binary, bench, output_dir)


if __name__ == '__main__':
    main()
