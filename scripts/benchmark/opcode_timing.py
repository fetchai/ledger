#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import csv

vm_benchmark_file = 'vm_benchmark.csv'

names = []
iters = []
real_times = []
cpu_times = []

with open(vm_benchmark_file) as csvfile:
  csvreader = csv.reader(csvfile)
  
  for row in csvreader:
    if 'AddInstruction' in row[0]:
      names.append(row[0])
      iters.append(int(row[1]))
      real_times.append(float(row[2]))
      cpu_times.append(float(row[3]))
      
add_obj_time = cpu_times[1] - cpu_times[0]
add_u32_time = cpu_times[3] - cpu_times[2]
sub_u32_time = cpu_times[4] - cpu_times[2]
mul_u32_time = cpu_times[5] - cpu_times[2]
div_u32_time = cpu_times[6] - cpu_times[2]

print('Add string:',add_obj_time)
print('Add uint32:',add_u32_time)
print('Subtract uint32:',sub_u32_time)
print('Multiply uint32:',mul_u32_time)
print('Divide uint32:',div_u32_time)