#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from tabulate import tabulate


def make_benchmark_table(names, means, stderrs, opcodes, base_inds, net_opcodes, n_reps, name_str=''):

    headers = ['Benchmark (' + str(n_reps) + ' reps)', 'Mean (ns)', 'Std error (ns)',
               'Opcodes', 'Baseline', 'Net opcodes']

    table = [[name, means[i], stderrs[i], opcodes[i], names[base_inds[i]],
              net_opcodes[i]] for (i, name) in names.items() if name_str in name]

    print('\n', tabulate(table, headers=headers, floatfmt=".2f"))

# def linear_fit_table(lfits, bm_opcodes)
#string_fit_table = []
# for bm in pfits.keys():
#    if 'String' in bm:
#        string_fit_table.append([bm, pfits[bm][0], pfits[bm][1], param_bm_opcodes[bm],
#                                param_baseline_bms[bm], param_net_opcodes[bm]])
#
# print('\n', tabulate(string_fit_table, headers=['Benchmark (' + str(num_reps) + ' reps)',
#                                                'Slope (ns/char)', 'Intercept (ns)','Opcodes',
#                                                'Baseline', 'Net opcodes'], floatfmt=".3f"))
