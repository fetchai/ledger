#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import matplotlib.pyplot as pl


def plot_linear_fit(name, bm, fig=1, savefig=False, savepath='', imgformat='png'):

    x = np.array(bm['param_vals'])
    y = np.array(bm['adj_net_means'])
    z = np.array(bm['adj_net_medians'])
    err = np.array(bm['net_stderrs'])
    lfits = np.array(bm['lfit'])

    pl.figure(fig)
    pl.clf()
    pmed, = pl.plot(x, z, 'g+')
    pm = pl.errorbar(x, y, err, linestyle='None', marker='o', fillstyle='none')
    leg = [pm, pmed]
    leg_str = ['mean \u00B1 std err', 'median']

    for (i, lfit) in enumerate(lfits):
        pf, = pl.plot(x, np.polyval(lfit, x))
        leg.append(pf)
        leg_str.append('y = {:0.2f}x + {:0.2f}'.format(lfit[0], lfit[1]))

    pl.grid()
    pl.xlabel('size')
    pl.ylabel('cpu time (ns)')
    pl.title(name + ' ' + str(bm['adj_net_opcodes']))
    pl.legend(leg, leg_str)
    pl.show()

    if savefig:
        pl.savefig(savepath + name + '.' + imgformat)
