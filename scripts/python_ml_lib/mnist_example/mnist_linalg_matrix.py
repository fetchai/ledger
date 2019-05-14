from mlearner import MLearner
from line_profiler import LineProfiler


def run_mnist():
    mlearner = MLearner()

    # load the data
    mlearner.load_data(one_hot=True)

    # being training
    mlearner.train()


run_mnist()
