#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import matplotlib.pyplot as pl


def plot_linear_fit(name, bm, fig=1, savefig=False, savepath='', imgformat='png'):

    x = np.array(bm['param_vals'])
    y = np.array(bm['net_means'])
    err = np.array(bm['net_stderrs'])
    lfit = np.array(bm['lfit'])

    pl.figure(fig)
    pl.clf()
    pm = pl.errorbar(x, y, err, linestyle='None', marker='o')
    pf, = pl.plot(x, np.polyval(lfit, x))
    pl.grid()
    pl.xlabel('size')
    pl.ylabel('time (ns)')
    pl.title(name)
    pl.legend(
        (pm, pf), ('measured', 'y = {:0.2f}x + {:0.2f}'.format(lfit[0], lfit[1])))
    pl.show()

    if savefig:
        pl.savefig(savepath + name + '.' + imgformat)
