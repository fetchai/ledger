#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import numpy as np
from scipy.optimize import lsq_linear


def linear_fit(param_bm, fit_range=[0,1e20]):
    """Compute the least-squares linear fit of mean-cpu-time to parameter value (e.g. array length).
    Fit is anchored to the first data point to ensure reliable results for small parameter values.
    Since some benchmarks become non-linear for large values, allow restriction of the fit to some
    maximum parameter value."""

    x = np.array(param_bm['param_vals'])
    y = np.array(param_bm['adj_net_means'])

    # Shift data so that first data point is at the origin (anchor to first data point)
    y0 = y[0]
    x0 = x[0]
    y = y - y0
    x = x - x0

    # Cut data to specified range (if given)
    fit_ind = np.where((x >= fit_range[0]) & (x <= fit_range[1]))[0]
    x = x[fit_ind]
    y = y[fit_ind]

    # Compute least-squares linear fit through the origin
    A = np.reshape(x, (len(x), 1))
    slope, = np.linalg.lstsq(A, y, rcond=None)[0]
    intercept = y0-slope*x0

    return [slope, intercept]


def opcode_times(benchmarks, optimes, bm_cls, prim_type=''):
    """Let A be a matrix where each row corresponds to a benchmark and A[i,j]=k if
    benchmark i uses opcode j a total of k times. Let b be a vector where b[i] is
    the mean cpu time to run benchmark i. Then the solution x = argmin(||Ax-b||)
    provides an estimate of the individual opcode times."""

    # Get bms of the given class and primitive type (if specified)
    select_bms = [bm for (ind, bm) in benchmarks.items() if bm['class'] in bm_cls
                  and prim_type in bm['name']]

    # Set of all opcodes used in select bms
    ops = set().union(*[bm['opcodes'] for bm in select_bms])

    # Divide into already computed and new opcode times
    known_ops = set(optimes.keys()).intersection(ops)
    new_ops = ops - known_ops

    # Generate map from opcode to index in matrix or vector
    known_op2ind = {op: ind for (ind, op) in enumerate(known_ops)}
    new_op2ind = {op: ind for (ind, op) in enumerate(new_ops)}

    # Initialize least-squares matrices (A=new, Ak=known)
    A = np.zeros((len(select_bms), len(new_ops)))
    Ak = np.zeros((len(select_bms), len(known_ops)))
    b = np.zeros(len(select_bms))

    # Generate matrices from opcode usage and times
    for (i, bm) in enumerate(select_bms):
        for op in bm['opcodes']:
            b[i] = bm['mean'] / bm['stddev']
            if op in known_ops:
                j = known_op2ind[op]
                Ak[i, j] += 1 / bm['stddev']
            else:
                j = new_op2ind[op]
                A[i, j] += 1 / bm['stddev']

    # Subtract known opcode times from measured to isolate time for new opcodes
    if len(Ak) > 0:
        xk = np.array([optimes[op] for op in known_ops])
        b = b - Ak@xk

    # Solve least-squares problem
    sol = lsq_linear(A, b, bounds=(0, np.inf))
    x = sol.x

    # Update optimes with new results
    optimes.update({op: x[i] for (i, op) in enumerate(new_ops)})
