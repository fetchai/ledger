# coding: utf-8
from mlearner import MLearner
def run_mnist():

    mlearner = MLearner()

    # load the data
    mlearner.load_data(one_hot=True)

    # being training
    mlearner.train()



# if __name__ == "__main__":
#     import cProfile
#     cProfile.runctx('run_mnist()', globals(), locals())
run_mnist()
