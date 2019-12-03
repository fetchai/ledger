#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import matplotlib.pyplot as pl


def plot(size, time, err=None, fig=1, xlabel='length', ylabel='time (ns)', title='', savefig=False, savepath='', imgformat='png'):

    pfit = np.polyfit(np.array(size), np.array(time), 1)

    pl.figure(fig)
    # pl.clf()
    if err is None:
        pm, = pl.plot(size, time, 'o')
    else:
        pm = pl.errorbar(size, time, err, linestyle='None', marker='o')
    pf, = pl.plot(size, np.polyval(pfit, size))
    pl.grid()
    pl.xlabel(xlabel)
    pl.ylabel(ylabel)
    pl.title(title)
    pl.legend(
        (pm, pf), ('measured', 'y = {:0.2f}x + {:0.2f}'.format(pfit[0], pfit[1])))
    pl.show()

    if savefig:
        pl.savefig(savepath + title + '.' + imgformat)
