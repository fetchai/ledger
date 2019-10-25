#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import csv
import subprocess, os
from tabulate import tabulate

# csv file to be generated by vm_benchmarks
vm_benchmark_path = '../../cmake-build-release/libs/vm/benchmark/'
vm_benchmark_file = 'vm_benchmark.csv'
opcode_list_file = 'opcode_lists.csv'

# delete old file if it exists
if os.path.exists(opcode_list_file):
  os.remove(opcode_list_file)

# number of benchmark repetitions
run_benchmarks = True
num_reps = 2

# run benchmarks
if run_benchmarks:
  stdout = subprocess.call([vm_benchmark_path + 'vm-benchmarks',
                  '--benchmark_out=' + vm_benchmark_file,
                  '--benchmark_out_format=csv',
                  '--benchmark_repetitions=' + str(num_reps),
                  '--benchmark_report_aggregates_only=true',
                  '--benchmark_display_aggregates_only=true'])

# read opcode lists and baseline bms
with open(opcode_list_file) as csvfile:
  csvreader = csv.reader(csvfile)
  oprows = [oprow for oprow in csvreader]  
  
baselines = {oprow[0] : oprow[1] for oprow in oprows} 
opcodes = {oprow[0] : [int(opcode) for (i,opcode) in enumerate(oprow[2:])] for oprow in oprows} 
  
# read results in text format
with open(vm_benchmark_file) as csvfile:
  csvreader = csv.reader(csvfile)
  rows = [row for row in csvreader if 'OpcodeBenchmark' in row[0]]  
     
# parse benchmark labels
labels = [row[0] for row in rows]
benchmarks = [label.split('_',1) for label in labels]    
fullnames = [bm[0] for (ind,bm) in enumerate(benchmarks) if ind % 3 == 0]
bm_names = [fullname.split('/',1)[1] for fullname in fullnames]
bm_inds = {bm_name : i for (i,bm_name) in enumerate(bm_names)}
num_benchmarks = len(bm_names)

# cpu time stats (in ns) 
means = [float(row[3]) for row in rows if 'mean' in row[0]]
medians = [float(row[3]) for row in rows if 'median' in row[0]]
stddevs = [float(row[3]) for row in rows if 'stddev' in row[0]]    

# make table out of net benchmark times, standard errors, and (net) opcodes
table = []
for bm in range(0,num_benchmarks-2):
  base_bm = bm_inds[baselines[bm_names[bm]]]
  
  # net stats
  bm_mean = means[bm] - means[base_bm]
  bm_stderr = (stddevs[bm]**2 + stddevs[base_bm]**2)**0.5/num_reps**0.5
  
  # net opcodes 
  bm_opcodes = opcodes[bm_names[bm]]
  base_opcodes = opcodes[bm_names[base_bm]].copy()
  net_opcodes = [x for x in bm_opcodes if not x in base_opcodes or base_opcodes.remove(x)]

  bm_opcodes_copy = bm_opcodes.copy()
  ext_opcodes = [x for x in base_opcodes if not x in bm_opcodes_copy or bm_opcodes_copy.remove(x)]
  
  table.append([bm_names[bm], bm_mean, bm_stderr, bm_opcodes, bm_names[base_bm], net_opcodes]) #, ext_opcodes])

print('\n')
print(tabulate(table, headers = ['Benchmark (' + str(num_reps) + ' reps)'
                                 ,'Mean (ns)','Std error (ns)','Opcodes',
                                 'Baseline','Net opcodes'], floatfmt=".2f"))

