#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import csv
import subprocess
import numpy as np
from glob import glob
from shutil import copy
from datetime import date
import vm_least_squares

# Location of files to be generated by vm_benchmarks
vm_benchmark_path = '../../cmake-build-release/libs/vm-modules/opcode/benchmark/'
vm_benchmark_file = 'vm_benchmark.csv'
opcode_list_file = 'opcode_lists.csv'
opcode_def_file = 'opcode_defs.csv'

# Path to save results files and plots
output_path = 'results/' + date.today().strftime("%Y-%m-%d") + '/'

# Number of benchmark repetitions
n_reps = 100

# Benchmark categories to run
bm_filter = 'Basic|Prim|Math|Object|Array|Tensor|Crypto'

run_benchmarks = True
make_plots = True
make_tables = True
save_results = True
imgformat = 'png'

# Specify maximum parameter value to use in linear fit for bm names that contain these strings
bm_fit_ranges = {'Array': [[0, 6000], [8000, 1e20]],
                 'FromStr': [[0, 100000], [120000, 1e20]]}
slope_thresh = 0.01


def index(row):
    """Return index of benchmark from name listed in vm_benchmark_file"""
    return int(row[0].split('_')[0].split('/')[1])


def get_net_opcodes(ops, base_ops):
    """Get the list of opcodes that are in ops but not in base_ops"""
    base_copy = base_ops.copy()
    return [x for x in ops if x not in base_copy or base_copy.remove(x)]


def aggregate_stats(bms, bm_type, base_inds):
    """Compute combined stats treating parameterized benchmarks as a single benchmark. Mostly useful
    in the case that cpu times do not depend on parameter value."""
    inds = [ind for (ind, bm) in bms.items() if bm['type'] == bm_type]
    n_type = len(inds)
    all_cpu_times = np.hstack(
        [np.array(bms[ind]['cpu_times']) for ind in inds])
    all_base_times = np.hstack(
        [np.array(bms[base_inds[ind]]['cpu_times']) for ind in inds])
    agg_net_mean = np.mean(all_cpu_times - all_base_times)
    agg_net_median = np.median(all_cpu_times - all_base_times)
    agg_net_stderr = np.sqrt((np.std(all_cpu_times) ** 2) / (n_reps*n_type) +
                             (np.std(all_base_times) ** 2) / (n_reps*n_type))
    return agg_net_mean, agg_net_median, agg_net_stderr


# Delete old file if it exists
if run_benchmarks and os.path.exists(opcode_list_file):
    os.remove(opcode_list_file)

# Run benchmarks
if run_benchmarks:
    stdout = subprocess.call([vm_benchmark_path + 'vm-benchmarks',
                              '--benchmark_out=' + vm_benchmark_file,
                              '--benchmark_out_format=csv',
                              '--benchmark_repetitions=' + str(n_reps),
                              '--benchmark_report_aggregates_only=false',
                              '--benchmark_display_aggregates_only=false',
                              '--benchmark_filter=' + bm_filter + ''])

# Read opcode lists and baseline bms
with open(opcode_list_file) as csvfile:
    csvreader = csv.reader(csvfile)
    opcode_rows = [row for row in csvreader]

# Create dicts from benchmark indices to names, baseline indices, and opcode lists
bm_names = {int(row[0]): row[1] for row in opcode_rows}
bm_inds = {row[1]: int(row[0]) for row in opcode_rows}
baseline_inds = {bm_inds[row[1]]: bm_inds[row[2]] for row in opcode_rows}
opcodes = {int(row[0]): [int(op) for (i, op) in enumerate(row[3:])]
           for row in opcode_rows}

# Read results in text format
with open(vm_benchmark_file) as csvfile:
    csvreader = csv.reader(csvfile)
    bm_rows = [row for row in csvreader if 'Benchmark' in row[0]]

# Read opcode definitions
with open(opcode_def_file) as csvfile:
    csvreader = csv.reader(csvfile, delimiter='\t')
    opcode_defs = {int(row[0]): row[1] for row in csvreader}

data_rows = [row for row in bm_rows if '_' not in row[0]]
cpu_times = {ind: [float(row[2]) for row in data_rows if int(row[0].split('/')[1]) == ind]
             for ind in bm_inds.values()}

# Get cpu time stats (in ns) for each bm index
means = {index(row): float(row[3]) for row in bm_rows if 'mean' in row[0]}
medians = {index(row): float(row[3]) for row in bm_rows if 'median' in row[0]}
stddevs = {index(row): float(row[3]) for row in bm_rows if 'stddev' in row[0]}

bm_classes = ['Basic', 'String', 'Prim', 'Math', 'Array', 'Tensor', 'Sha256']
prim_bm_classes = ['Prim', 'Math']
param_bm_classes = ['String', 'Array', 'Sha256', 'Tensor']

# Primitives used in primitive operation benchmarks
op_prims = {'Int8', 'Int16', 'Int32', 'Int64', 'UInt8', 'UInt16', 'UInt32', 'UInt64',
            'Float32', 'Float64', 'Fixed32', 'Fixed64', 'Fixed128'}

# Primitives used in math function benchmarks
math_prims = {'Float32', 'Float64', 'Fixed32', 'Fixed64', 'Fixed128'}

# Collect benchmark data and stats
benchmarks = {ind: {'name': name,
                    'mean': means[ind],
                    'median': medians[ind],
                    'stddev': stddevs[ind],
                    'baseline': bm_names[baseline_inds[ind]],
                    'opcodes': opcodes[ind],
                    'net_mean': means[ind] - means[baseline_inds[ind]],
                    'net_median': medians[ind] - medians[baseline_inds[ind]],
                    'net_stderr': (stddevs[ind] ** 2 + stddevs[baseline_inds[ind]] ** 2) ** 0.5 / n_reps ** 0.5,
                    'net_opcodes': get_net_opcodes(opcodes[ind], opcodes[baseline_inds[ind]]),
                    'ext_opcodes': get_net_opcodes(opcodes[baseline_inds[ind]], opcodes[ind]),
                    'cpu_times': cpu_times[ind],
                    'type': ''
                    } for (ind, name) in bm_names.items()}

for (ind, bm) in benchmarks.items():
    bm_class = [cl for cl in bm_classes if cl in bm['name']]
    if len(bm_class) == 0:
        bm['class'] = 'Basic'
    else:
        bm['class'] = bm_class[0]
    if bm['class'] == 'Tensor':
        bm['type'] = bm['name'].split('-')[0]
        dim_size = bm['name'].split('_')[1]
        bm['dim'] = int(dim_size.split('-')[0])
        bm['param_val'] = int(dim_size.split('-')[1])**bm['dim']
    elif bm['class'] in param_bm_classes and '_' in bm['name']:
        bm['type'] = bm['name'].split('_')[0]
        bm['param_val'] = int(bm['name'].split('_')[1])
    elif bm['class'] in prim_bm_classes:
        bm['primitive'] = bm['name'].split('_')[1]

# Delete 'Break' and 'Continue' benchmarks for the purposes of least-squares fitting
benchmarks_fit = benchmarks.copy()
for key in benchmarks.keys():
    if benchmarks[key]['name'] in {'Break', 'Continue'}:
        del benchmarks_fit[key]

# Solve linear least-squares problem to estimate individual opcode times
optimes = {'Base': dict()}
vm_least_squares.opcode_times(benchmarks_fit, optimes['Base'], {'Basic'})

# For each primitive, build on already computed base opcodes
for prim in op_prims:
    optimes[prim] = optimes['Base'].copy()
    vm_least_squares.opcode_times(benchmarks_fit, optimes[prim], {
                                  'Prim'}, prim_type=prim)
for prim in math_prims:
    vm_least_squares.opcode_times(benchmarks_fit, optimes[prim], {
                                  'Math'}, prim_type=prim)

# Add individual opcode times to relevant benchmarks
for bm in benchmarks_fit.values():
    if bm['class'] == 'Basic':
        bm['opcode_times'] = {op: round(optimes['Base'][op], 2)
                              for op in bm['opcodes']}
    elif bm['class'] in prim_bm_classes:
        bm['opcode_times'] = {
            op: round(optimes[bm['primitive']][op], 2) for op in bm['opcodes']}

# Collect parameterized benchmark data
param_bm_types = {bm['type'] for bm in benchmarks.values(
) if bm['class'] in param_bm_classes and '_' in bm['name']}

param_bms = {type_name: {'inds': [ind for (ind, bm) in benchmarks.items() if bm['type'] == type_name]}
             for type_name in param_bm_types}

for (bm_type, param_bm) in param_bms.items():
    ind0 = param_bm['inds'][0]
    agg_stats = aggregate_stats(benchmarks, bm_type, baseline_inds)
    param_bm['opcodes'] = benchmarks[ind0]['opcodes']
    param_bm['baseline'] = benchmarks[ind0]['baseline']
    param_bm['net_opcodes'] = benchmarks[ind0]['net_opcodes']
    param_bm['param_vals'] = [benchmarks[i]['param_val']
                              for i in param_bm['inds']]
    param_bm['net_means'] = [benchmarks[ind]['net_mean']
                             for ind in param_bm['inds']]
    param_bm['net_medians'] = [benchmarks[ind]['net_median']
                               for ind in param_bm['inds']]
    param_bm['net_stderrs'] = [benchmarks[ind]['net_stderr']
                               for ind in param_bm['inds']]
    param_bm['agg_net_mean'] = agg_stats[0]
    param_bm['agg_net_median'] = agg_stats[1]
    param_bm['agg_net_stderr'] = agg_stats[2]

    # Subtract known opcode times from totals
    const_opcodes = [op for op in param_bm['net_opcodes']
                     if op in optimes['Int32']]
    new_opcodes = [op for op in param_bm['net_opcodes']
                   if op not in optimes['Int32']]
    const_cpu_time = sum([optimes['Int32'][op] for op in const_opcodes])
    param_bm['adj_net_opcodes'] = new_opcodes
    param_bm['adj_net_means'] = [
        x - const_cpu_time for x in param_bm['net_means']]
    param_bm['adj_net_medians'] = [
        x - const_cpu_time for x in param_bm['net_medians']]

    # Compute least-squares linear fit through the first data point up to given range
    fit_ranges = [[0, 1e20]]
    for key in bm_fit_ranges:
        if key in benchmarks[ind0]['name']:
            fit_ranges = bm_fit_ranges[key]
    param_bm['lfit'] = [[]]*len(fit_ranges)
    for (i, fit_range) in enumerate(fit_ranges):
        param_bm['lfit'][i] = vm_least_squares.linear_fit(param_bm, fit_range)

# Delete base opcodes from primitive opcode lists
for prim in op_prims:
    for op in optimes['Base']:
        del optimes[prim][op]

# Select parameterized benchmarks whose fit slopes satisfy the given threshold
param_dep_bms = {name: bm for (
    name, bm) in param_bms.items() if bm['lfit'][0][0] > slope_thresh}

if save_results:

    if not os.path.exists('results'):
        os.mkdir('results')

    if not os.path.exists(output_path):
        os.mkdir(output_path)
        os.mkdir(output_path + 'plots/')

    # Copy output of benchmark runs
    copy(vm_benchmark_file, output_path + vm_benchmark_file)
    copy(opcode_list_file, output_path + opcode_list_file)

    # Copy source code
    for file in glob(r'*.py'):
        copy(file, output_path)

if make_tables:

    import vm_benchmark_tables as vmt

    for bm_cls in bm_classes:
        print('\n' + vmt.benchmark_table(benchmarks, n_reps, bm_cls))

    print('\n' + vmt.benchmark_opcode_table(benchmarks_fit, n_reps, 'Basic'))

    for bm_cls in prim_bm_classes:
        print('\n' + vmt.benchmark_opcode_table(benchmarks_fit, n_reps, bm_cls))
        tab_int, tab_fp = vmt.primitive_table(benchmarks, n_reps, bm_cls)
        print('\n' + tab_int)
        print('\n' + tab_fp)

    for bm_cls in param_bm_classes:
        print('\n' + vmt.linear_fit_table(param_bms, n_reps, bm_cls))

    for optype in optimes:
        print('\n' + vmt.opcode_time_table(optimes, optype, opcode_defs))

if make_plots:

    from vm_benchmark_plots import plot_linear_fit

    fig = 0
    for (name, param_bm) in param_dep_bms.items():
        plot_linear_fit(name, param_bm, fig=fig, savefig=save_results,
                        savepath=output_path+'plots/', imgformat=imgformat)
        fig += 1
