# coding: utf-8

import os                                           # checking for already existing files
import gzip                                         # downloading/extracting mnist data
import random                                       # normal distirbution sampling
from tqdm import tqdm                               # visualising progress
import numpy as np                                  # loading data from buffer

from fetch.math.linalg import MatrixDouble          # our matrices
from utils import *                                 # activation functions

class MnistLearner():

    def __init__(self):

        self.data_url = 'http://yann.lecun.com/exdb/mnist/'

        self.x_tr_filename = 'train-images-idx3-ubyte.gz'
        self.y_tr_filename = 'train-labels-idx1-ubyte.gz'
        self.x_te_filename = 't10k-images-idx3-ubyte.gz'
        self.y_te_filename = 't10k-labels-idx1-ubyte.gz'


        self.training_size = 10000
        self.validation_size = 10000

        self.n_epochs = 30
        self.batch_size = 50
        self.alpha = 0.2

        self.mnist_input_size = 784         # pixels in 28 * 28 mnist images
        self.mnist_output_size = 10         # 10 possible characters to recognise

        self.activation_fn = 'relu'
        self.layers = [20]

        self.initialise_network()

    def initialise_network(self):

        # definition of the network layers
        self.net = []
        for i in range(len(self.layers)):
            self.net.append(self.layers[i])
        self.net.append(self.mnist_output_size)

        # instantiate the network weights once
        self.weights = [self.make_new_layer(self.mnist_input_size, self.net[0])]
        if len(self.net) > 2:
            for i in range(len(self.net) - 2):
                self.weights.append(self.make_new_layer(self.net[i], self.net[i + 1]))
        self.weights.append(self.make_new_layer(self.net[-2], self.net[-1]))

        # instantiate the gradients container once (and a temporary storage for updates
        self.grads = []
        for i in range(len(self.weights)):
            self.grads.append(MatrixDouble(self.weights[i].height(), self.weights[i].width()))
        self.temp_grads = []
        for i in range(len(self.weights)):
            self.temp_grads.append(MatrixDouble(self.weights[i].height(), self.weights[i].width()))

        # pre-instantiate constant set of zeroed matrices for relu comparisons

        self.const_zeros = []
        for idx in range(len(self.weights)):
            if idx == 0:
                self.const_zeros.append(MatrixDouble.Zeros(self.batch_size, self.weights[idx].width()))
            else:
                self.const_zeros.append(MatrixDouble.Zeros(self.const_zeros[idx - 1].height(), self.weights[idx].width()))

        return

    def make_new_layer(self, in_size, out_size, mode='normal'):
        '''
        makes a new MLP layer
        :param in_size:
        :param out_size:
        :param mode:
        :return:
        '''
        denom = np.sqrt(in_size)
        layer = MatrixDouble(in_size, out_size)
        for i in range(layer.size()):
            if mode == 'constant': # constant values - for debugging
                layer[i] = 1.0 / denom
            if mode == 'uniform': # random uniform - our library
                layer[i] = (random.uniform(-1.0, 1.0)) / denom
            if mode == 'normal': # random normal distribution
                layer[i] = np.random.normal(0, 1) / denom
        return layer

    def load_data(self, one_hot=True, reshape=None):
        x_tr = self.load_images(self.x_tr_filename, self.training_size)
        y_tr = self.load_labels(self.y_tr_filename, self.training_size)
        x_te = self.load_images(self.x_te_filename, self.validation_size)
        y_te = self.load_labels(self.y_te_filename, self.validation_size)

        if one_hot:
            y_tr_onehot = MatrixDouble.Zeros(y_tr.height(), self.mnist_output_size)
            y_te_onehot = MatrixDouble.Zeros(y_te.height(), self.mnist_output_size)

            for i in range(len(y_tr)):
                y_tr_onehot[i, int(y_tr[i])] = 1
            for i in range(len(y_te)):
                y_te_onehot[i, int(y_te[i])] = 1

        if reshape:
            x_tr, x_te = [x.reshape(*reshape) for x in (x_tr, x_te)]

        if one_hot:
            self.y_tr = y_tr_onehot
            self.y_te = y_te_onehot
        else:
            self.y_tr = y_tr
            self.y_te = y_te
        self.x_tr = x_tr
        self.x_te = x_te

    def load_images(self, filename, data_size):
        self.download(filename)
        with gzip.open(filename, 'rb') as f:
            data = np.frombuffer(f.read(), np.uint8, offset=16)
            data = data.reshape(-1, 28 * 28) / 256
            nd_data = MatrixDouble(data_size, 28 * 28)
            nd_data.FromNumpy(data[:data_size, :])
        return nd_data

    def load_labels(self, filename, data_size):
        self.download(filename)
        with gzip.open(filename, 'rb') as f:
            data = np.frombuffer(f.read(), np.uint8, offset=8)
            data.reshape(np.shape(data)[0], -1)
            nd_data = MatrixDouble(data_size, 1)
            nd_data.FromNumpy(data[:data_size].reshape(data_size, -1))
        return nd_data

    def download(self, filename):
        if not os.path.exists(filename):
            from urllib.request import urlretrieve
            print("Downloading %s" % filename)
            urlretrieve(self.data_url + filename, filename)

        return

    # feed forward pass of a network
    # take X input and the network (defined as a list of weights)
    # no biases?
    def feed_forward(self, X):

        a = [X]
        for idx in range(len(self.weights)):
            temp = MatrixDouble(a[-1].height(), self.weights[idx].width())
            temp = temp.Dot(a[-1], self.weights[idx])
            if self.activation_fn == 'relu':
                if ((self.const_zeros[idx].height() == temp.height()) and (self.const_zeros[idx].width() == temp.width())):
                    temp = relu(temp, self.const_zeros[idx])
                else:
                    temp = relu(temp, MatrixDouble.Zeros(temp.height(), temp.width()))

            elif self.activation_fn == 'sigmoid':
                temp = sigmoid(temp)
            else:
                print("unspecified activation functions!!")
                raise ValueError()
            a.append(temp)
        return a


    # get the gradients of the network
    def update_weights(self, X, Y):

        # run a forward pass to get delta
        a = self.feed_forward(X)
        last_delta = a[-1] - Y  # cross-entropy

        # calculate grads
        self.grads[-1] = self.grads[-1].TransposeDot(a[-2], last_delta)
        for i in range(len(a) - 2, 0, -1):
            # TODO: This dotTranspose gives a different answer from numpy; probably because of Array Major Order
            new_delta = MatrixDouble(last_delta.height(), self.weights[i].height())
            new_delta.DotTranspose(last_delta, self.weights[i])
            if self.activation_fn == 'sigmoid':
                new_delta *= d_sigmoid(a[i])
            elif self.activation_fn == 'relu':
                new_delta *= (a[i] >= self.const_zeros[i - 1])
            else:
                raise ValueError()
            self.grads[i - 1] = self.grads[i - 1].TransposeDot(a[i - 1], new_delta)

            last_delta = new_delta



        # divide grads by batch size
        for i in range(len(self.grads)):
            self.grads[i] /= X.height()

        for i in range(len(self.weights)):
            self.weights[i] -= (self.grads[i] * self.alpha)

        return

    def train(self):

        X = MatrixDouble(self.batch_size, self.mnist_input_size)
        Y = MatrixDouble(self.batch_size, self.mnist_output_size)

        # epochs
        for i in range(self.n_epochs):
            print("epoch ", i , ": ")

            # training batches
            for j in tqdm(range(0, self.x_tr.height() - self.batch_size, self.batch_size)):

                # assign X batch
                for k in range(self.batch_size):
                    for l in range(28*28):
                        X[k, l] = self.x_tr[j + k, l]

                # assign Y batch
                for k in range(self.batch_size):
                    for l in range(10):
                        Y[k, l] = self.y_tr[j + k, l]

                # update weights
                self.update_weights(X, Y)

            temp = self.weights[0].Copy()
            temp.Abs()

            print("Getting accuracy: ")
            print("\t getting feed forward predictions..")
            cur_pred = self.feed_forward(self.x_te)[-1]

            print("\t calculating argmaxes")
            max_pred = cur_pred.ArgMax(1)
            gt = self.y_te.ArgMax(1)

            print("\t comparing Y & Y^")
            sum_acc = 0
            for i in range(self.y_te.height()):
                sum_acc += (gt[i] == max_pred[i])
            sum_acc /= self.y_te.height()

            print("\taccuracy: ", sum_acc)

        return


def run_mnist():

    mlearner = MnistLearner()

    # load the data
    mlearner.load_data(one_hot=True)

    # being training
    mlearner.train()



# import cProfile
# cProfile.run('run_mnist()')
run_mnist()
