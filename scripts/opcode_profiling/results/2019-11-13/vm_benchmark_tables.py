#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from tabulate import tabulate


def benchmark_table(benchmarks, n_reps, bm_class=''):

    headers = ['Benchmark (' + str(n_reps) + ' reps)', 'Mean (ns)', 'Std error (ns)',
               'Opcodes', 'Baseline', 'Net opcodes']

    table = [[bm['name'], bm['net_mean'], bm['net_stderr'], bm['opcodes'],
              bm['baseline'], bm['net_opcodes']]
             for (i, bm) in benchmarks.items() if bm_class in bm['class']]

    print('\n', tabulate(table, headers=headers, floatfmt=".2f"))


def primitive_table(benchmarks, n_reps, bm_class):

    prim_bms = {ind: bm for (ind, bm) in benchmarks.items()
                if bm['class'] == bm_class}
    prim_types = list({bm['name'].split('_')[1]
                       for bm in prim_bms.values()} - {''})
    bm_types = list({bm['name'].split('_')[0] for bm in prim_bms.values()})
    prim_types.sort()
    bm_types.sort()

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
        print('\n', tabulate(table_int, headers=headers_int, stralign='left'))

    if len(table_fp[0]) > 1:
        print('\n', tabulate(table_fp, headers=headers_fp, stralign='left'))


def linear_fit_table(param_bms, n_reps, bm_cls):

    lfit_table = []
    for (name, bm) in param_bms.items():
        if bm_cls in name:
            lfit_table.append([name, bm['lfit'][0], bm['lfit'][1], bm['opcodes'],
                               bm['baseline'], bm['net_opcodes']])

    headers = ['Benchmark (' + str(n_reps) + ' reps)', 'Slope (ns/char)',
               'Intercept (ns)', 'Opcodes', 'Baseline', 'Net opcodes']

    print('\n', tabulate(lfit_table, headers=headers, floatfmt=".3f"))


# def tensor_table(tensor_bms, n_reps):
#
#    # tabulate tensor benchmarks
#    tensor_bm_types = list({bm.split('_')[0] for bm in tensor_bms.values()})
#    dim_sizes = list({bm.split('_')[1] for bm in tensor_bms.values()})
#    tensor_bm_types.sort()
#    dim_sizes.sort()
#    dims = {int(ds.split('-')[0]) for ds in dim_sizes}
#
#    tensor_bm_table = []
#    tensor_pfits = {}
#    tensor_bm_opcodes = {}
#    tensor_base_opcodes = {}
#    tensor_net_opcodes = {}
#    tensor_baseline_bms = {}
#    for bm_type in tensor_bm_types:
#
#        type_bms = {ind : name for (ind,name) in tensor_bms.items() if bm_type in name}
#        type_inds = list(type_bms.keys())
#
#        dims = [int(x.split('_')[1].split('-')[0]) for x in type_bms.values()]
#        sizes = [int(x.split('-')[1]) for x in type_bms.values()]
#        n_elems = [s**dims[i] for (i,s) in enumerate(sizes)]
#
#        type_means = [net_means[ind] for ind in type_bms.keys()]
#        type_stderrs = [net_stderrs[ind] for ind in type_bms.keys()]
#
#        # get opcodes
#        tensor_bm_opcodes[bm_type] = opcodes[type_inds[0]]
#        tensor_base_opcodes[bm_type] = opcodes[base_inds[type_inds[0]]]
#        tensor_net_opcodes[bm_type] = net_opcodes[type_inds[0]]
#        tensor_baseline_bms[bm_type] = bm_names[base_inds[type_inds[0]]].split('_')[0]
#
#        if make_plots:
#            import matplotlib.pyplot as pl
#            pl.figure(fig) #, figsize = (10,8))
#        leg = []
#        leg_str = []
#        for dim in set(dims):
#            ne_dim = [n_elems[i] for (i,d) in enumerate(dims) if dim==d]
#            m_dim = [type_means[i] for (i,d) in enumerate(dims) if dim==d]
#            se_dim = [type_stderrs[i] for (i,d) in enumerate(dims) if dim==d]
#            t_pfit = np.polyfit(np.array(ne_dim), np.array(m_dim), 1).tolist()
#            tensor_pfits[bm_type + '-' + str(dim)] = t_pfit
#            if make_plots:
#                pf, = pl.plot(ne_dim, np.polyval(t_pfit, ne_dim), ':', linewidth=0.5)
#                p = pl.errorbar(ne_dim, m_dim,se_dim,linestyle='-', linewidth=0.5, marker='o',color=pf.get_color())
#
#                leg.extend([p,pf])
#                leg_str.extend([dim, 'y = {:0.2f}x + {:0.2f}'.format(t_pfit[0], t_pfit[1])])
#
#        if make_plots:
#            pl.legend(leg,leg_str)
#            pl.title(bm_type)
#            pl.xlabel('tensor size (# of elements)')
#            pl.ylabel('time (ns)')
#            pl.grid()
#            pl.show()
#            fig += 1
#            if save_results:
#                pl.savefig(output_path + 'plots/' + bm_type + '.' + imgformat)
#
#        type_row = [bm_type]
#        for dim_size in dim_sizes:
#            tab_bms = [x for x in tensor_bms.values() if [bm_type,dim_size] == x.split('_')]
#            if len(tab_bms) == 1:
#                tab_ind = bm_inds[tab_bms[0]]
#                type_row.append("{:.2f} \u00B1 {:.2f}".format(net_means[tab_ind], net_stderrs[tab_ind]))
#            else:
#                type_row.append('--')
#        tensor_bm_table.append(type_row)
#
#
#    headers = ['Benchmark (' + str(n_reps) + ' reps)'] + [h + ' (ns)' for h in dim_sizes]
#    print('\n', tabulate([row[:9] for row in tensor_bm_table], headers=headers[:9], stralign='left'))
#    print('\n', tabulate([[row[0]] + row[9:17] for row in tensor_bm_table], headers=[headers[0]]+headers[9:17], stralign='left'))
#    print('\n', tabulate([[row[0]] + row[17:25] for row in tensor_bm_table], headers=[headers[0]] + headers[17:25], stralign='left'))
#
