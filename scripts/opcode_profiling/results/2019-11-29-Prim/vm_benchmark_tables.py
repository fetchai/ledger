#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from tabulate import tabulate

tablefmt = 'github'


def benchmark_table(benchmarks, n_reps, bm_class=''):

    headers = ['Benchmark (' + str(n_reps) + ' reps)', 'Net mean (ns)', 'Net median (ns)',
               'Std error (ns)', 'Opcodes', 'Baseline', 'Net opcodes']

    table = [[bm['name'], bm['net_mean'], bm['net_median'], bm['net_stderr'],
              bm['opcodes'], bm['baseline'], bm['net_opcodes']]
             for (i, bm) in benchmarks.items() if bm_class in bm['class']]

    if len(table) > 0:
        print('\n', tabulate(table, headers=headers,
                             tablefmt=tablefmt, floatfmt=".2f"))


def benchmark_opcode_table(benchmarks, n_reps, bm_class=''):

    headers = ['Benchmark (' + str(n_reps) + ' reps)', 'Mean (ns)', 'Std. error (ns)',
               'Opcodes: estimated times (ns)']

    table = [[bm['name'], bm['mean'], bm['net_stderr'], bm['opcode_times']]
             for (i, bm) in benchmarks.items() if bm_class in bm['class']]

    if len(table) > 0:
        print('\n', tabulate(table, headers=headers,
                             tablefmt=tablefmt, floatfmt=".2f"))


def primitive_table(benchmarks, n_reps, bm_class):

    prim_bms = {ind: bm for (ind, bm) in benchmarks.items()
                if bm['class'] == bm_class}
    prim_types = list({bm['name'].split('_')[1]
                       for bm in prim_bms.values()} - {''})
    bm_types = list({bm['name'].split('_')[0] for bm in prim_bms.values()})
    prim_types.sort()
    bm_types.sort()

    if len(prim_bms) == 0:
        return

    prim_bm_table = []
    for bm_type in bm_types:
        type_row = [bm_type]
        for prim in prim_types:
            tab_bms = [bm for bm in prim_bms.values() if [bm_type, prim]
                       == bm['name'].split('_')]
            if len(tab_bms) == 1:
                tab_bm = tab_bms[0]
                type_row.append("{:.2f} \u00B1 {:.2f}".format(
                    tab_bm['net_mean'], tab_bm['net_stderr']))
            else:
                type_row.append('--')
        prim_bm_table.append(type_row)

    int_prim_types = [typ for typ in prim_types if 'Int' in typ]
    fp_prim_types = [typ for typ in prim_types if 'Int' not in typ]

    headers_int = ['Benchmark (' + str(n_reps) + ' reps)'] + \
        [h + ' (ns)' for h in int_prim_types]
    table_int = [[row[0]] + [elem for (i, elem) in enumerate(
        row[1:]) if prim_types[i] in int_prim_types] for row in prim_bm_table]

    headers_fp = ['Benchmark (' + str(n_reps) + ' reps)'] + \
        [h + ' (ns)' for h in fp_prim_types]
    table_fp = [[row[0]] + [elem for (i, elem) in enumerate(
        row[1:]) if prim_types[i] in fp_prim_types] for row in prim_bm_table]

    if len(table_int[0]) > 1:
        print('\n', tabulate(table_int, headers=headers_int,
                             tablefmt=tablefmt, stralign='left'))

    if len(table_fp[0]) > 1:
        print('\n', tabulate(table_fp, headers=headers_fp,
                             tablefmt=tablefmt, stralign='left'))


def linear_fit_table(param_bms, n_reps, bm_cls):

    lfit_table = []
    for (name, bm) in param_bms.items():
        if bm_cls in name:
            lfit_table.append([name, bm['lfit'][0], bm['lfit'][1], bm['agg_net_mean'],
                               bm['agg_net_median'], bm['agg_net_stderr'], bm['opcodes'],
                               bm['baseline'], bm['net_opcodes']])

    headers = ['Benchmark (' + str(n_reps) + ' reps)', 'Slope (ns/char)',
               'Intercept (ns)', 'Mean (ns)', 'Median (ns)', 'Std error (ns)', 'Opcodes',
               'Baseline', 'Net opcodes']

    if len(lfit_table) > 0:
        print('\n', tabulate(lfit_table, headers=headers,
                             tablefmt=tablefmt, floatfmt=".3f"))


def opcode_time_table(optimes, opcode_defs):

    for optype in sorted(optimes):
        headers = ['Opcode (' + optype + ')', 'Name', 'Estimated time (ns)']

        table = [[op, opcode_defs[op], optimes[optype][op]]
                 for op in sorted(optimes[optype].keys())]

        if len(table) > 0:
            print('\n', tabulate(table, headers=headers,
                                 tablefmt='github', floatfmt=".2f"))
