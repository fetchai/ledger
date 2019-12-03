#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import numpy as np


def linear_fit(bms, bm_type, max_len=1e20):
    """Compute the least-squares linear fit of mean-cpu-time to parameter value (e.g. array length).
    Fit is anchored to the first data point to ensure reliable results for small parameter values.
    Since some benchmarks become non-linear for large values, allow restriction of the fit to some
    maximum parameter value."""

    x = np.array([bm['param_val']
                  for bm in bms.values() if bm['type'] == bm_type])
    y = np.array([bm['net_mean']
                  for bm in bms.values() if bm['type'] == bm_type])

    # Shift data so that first data point is at the origin (anchor to first data point)
    y0 = y[0]
    y = y - y0
    x = x - x[0]

    # Cut data to specified range (if given)
    x = x[np.where(x < max_len)]
    y = y[0:len(x)]

    # Compute least-squares linear fit
    A = np.reshape(x, (len(x), 1))
    slope, = np.linalg.lstsq(A, y, rcond=None)[0]
    intercept = y0-slope*x[0]

    return [slope, intercept]


def opcode_times(benchmarks, n_reps, optimes, bm_cls, prim_type=''):
    """Let A be a matrix where each row corresponds to a benchmark and A[i,j]=k if
    benchmark i uses opcode j a total of k times. Let b be a vector where b[i] is
    the mean cpu time to run benchmark i. Then the solution x = argmin(||Ax-b||)
    provides an estimate of the invidual opcode times."""

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
    A = np.zeros((n_reps*len(select_bms), len(new_ops)))
    Ak = np.zeros((n_reps*len(select_bms), len(known_ops)))
    b = np.zeros(n_reps*len(select_bms))

    # Generate matrices from opcode usage and times
    for (ind, bm) in enumerate(select_bms):
        for rep in range(0, n_reps):
            i = ind*n_reps + rep
            for op in bm['opcodes']:
                b[i] = bm['cpu_times'][rep]  # /bm['stddev']
                if op in known_ops:
                    j = known_op2ind[op]
                    Ak[i, j] += 1
                else:
                    j = new_op2ind[op]
                    A[i, j] += 1  # /bm['stddev']

    # Subtract known opcode times from measured to isolate time for new opcodes
    if len(Ak) > 0:
        xk = np.array([optimes[op] for op in known_ops])
        b = b - Ak@xk

    # Solve least-squares problem
    x = np.linalg.lstsq(A, b, rcond=None)[0]

    # Update optimes with new results
    optimes.update({op: x[i] for (i, op) in enumerate(new_ops)})
