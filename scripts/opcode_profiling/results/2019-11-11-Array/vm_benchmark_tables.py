#!/usr/bin/env python3
# -*- coding: utf-8 -*-


def linear_fit_table(lfits, bm_opcodes)


string_fit_table = []
for bm in pfits.keys():
    if 'String' in bm:
        string_fit_table.append([bm, pfits[bm][0], pfits[bm][1], param_bm_opcodes[bm],
                                 param_baseline_bms[bm], param_net_opcodes[bm]])

print('\n', tabulate(string_fit_table, headers=['Benchmark (' + str(num_reps) + ' reps)',
                                                'Slope (ns/char)', 'Intercept (ns)', 'Opcodes',
                                                'Baseline', 'Net opcodes'], floatfmt=".3f"))
