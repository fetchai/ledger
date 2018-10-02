# coding: utf-8

import os                                           # checking for already existing files
import gzip                                         # downloading/extracting mnist data
import random                                       # normal distirbution sampling
from tqdm import tqdm                               # visualising progress
import numpy as np                                  # loading data from buffer

# from fetch.math.linalg import MatrixDouble          # our matrices
from fetch.math.linalg import MatrixDouble
from fetch.ml import Layer, Variable, Session
from fetch.ml import CrossEntropyLoss
# from fetch.ml.layers import Layer          # our matrices

class MnistLearner():

    def __init__(self):

        self.data_url = 'http://yann.lecun.com/exdb/mnist/'

        self.x_tr_filename = 'train-images-idx3-ubyte.gz'
        self.y_tr_filename = 'train-labels-idx1-ubyte.gz'
        self.x_te_filename = 't10k-images-idx3-ubyte.gz'
        self.y_te_filename = 't10k-labels-idx1-ubyte.gz'


        self.training_size = 100
        self.validation_size = 10000

        self.n_epochs = 30
        self.batch_size = 50
        self.alpha = 0.2

        self.mnist_input_size = 784         # pixels in 28 * 28 mnist images
        self.mnist_output_size = 10         # 10 possible characters to recognise

        self.activation_fn = 'relu'
        self.layers = [20]

        self.net = []
        # self.weights = []
        self.layers = []
        self.sess = None
        self.initialise_network()

    def initialise_network(self):

        self.sess = Session()

        # definition of the network layers
        for i in range(len(self.layers)):
            self.net.append(self.layers[i])
        self.net.append(self.mnist_output_size)

        self.layers.append(Layer(self.sess, self.mnist_input_size, self.net[0]))
        if len(self.net) > 2:
            for i in range(len(self.net) - 2):
                self.layers.append(Layer(self.sess, self.net[i], self.net[i + 1]))
        self.layers.append(Layer(self.sess, self.net[-1], self.mnist_output_size))

        return

    def load_data(self, one_hot=True, reshape=None):
        x_tr = self.load_images(self.x_tr_filename, self.training_size)
        y_tr = self.load_labels(self.y_tr_filename, self.training_size)
        x_te = self.load_images(self.x_te_filename, self.validation_size)
        y_te = self.load_labels(self.y_te_filename, self.validation_size)

        if one_hot:
            y_tr_onehot = Variable.zeroes(self.sess, [y_tr.size(), self.mnist_output_size])
            y_te_onehot = Variable.zeroes(self.sess, [y_te.size(), self.mnist_output_size])

            for i in range(y_tr.size()):
                y_tr_onehot[i, int(y_tr[i])] = 1

            for i in range(y_te.size()):
                y_te_onehot[i, int(y_te[i])] = 1
            self.y_tr = y_tr_onehot
            self.y_te = y_te_onehot
        else:
            self.y_tr = y_tr
            self.y_te = y_te

        if reshape:
            x_tr, x_te = [x.reshape(*reshape) for x in (x_tr, x_te)]
        self.x_tr = x_tr
        self.x_te = x_te

    def load_images(self, filename, data_size):
        self.download(filename)
        with gzip.open(filename, 'rb') as f:
            data = np.frombuffer(f.read(), np.uint8, offset=16)
            data = data.reshape(-1, 28 * 28) / 256
            # nd_data = Variable(self.sess, data_size, 28 * 28)
            nd_data = Variable(self.sess)
            nd_data.FromNumpy(data[:data_size, :])

        return nd_data

    def load_labels(self, filename, data_size):
        self.download(filename)
        with gzip.open(filename, 'rb') as f:
            data = np.frombuffer(f.read(), np.uint8, offset=8)
            data.reshape(np.shape(data)[0], -1)

            nd_data = Variable(self.sess)
            nd_data.FromNumpy(data[:data_size].reshape(data_size, -1))
            # nd_data = Layer(data_size, 1, self.sess)
            # nd_data.data().FromNumpy(data[:data_size].reshape(data_size, -1))
        # return nd_data
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
        activate = True
        for idx in range(len(self.layers)):
            if (idx == (len(self.layers) -1)):
                activate = False
            a.append(self.layers[idx].Forward(a[-1], activate))
        return a

    def calculate_loss(self, X, Y):
        return CrossEntropyLoss(X, Y, self.sess)

    def backprop(self, a):
        self.sess.BackwardGraph(a)
        for i in range(len(self.layers)):
            self.layers[i].Step(self.alpha)
        return

    def train(self):

        # X = Variable(self.sess, self.batch_size, self.mnist_input_size)
        # Y = Variable(self.sess, self.batch_size, self.mnist_output_size)
        X = Variable(self.sess)
        Y = Variable(self.sess)
        X.Reshape(self.batch_size, self.mnist_input_size)
        Y.Reshape(self.batch_size, self.mnist_output_size)

        # epochs
        for i in range(self.n_epochs):
            print("epoch ", i, ": ")

            # training batches
            for j in tqdm(range(0, self.x_tr.shape()[0] - self.batch_size, self.batch_size)):

                # assign X batch
                for k in range(self.batch_size):
                    for l in range(28*28):
                        X[k, l] = self.x_tr[j + k, l]

                # assign Y batch
                for k in range(self.batch_size):
                    for l in range(10):
                        Y[k, l] = self.y_tr[j + k, l]

                # update weights
                #
                # temp = Layer(self.sess, a[-1].InputSize(), self.weights[idx].OutputSize())
                # temp = a[-1].Dot(self.sess, self.weights[idx])

                # temp = X.Dot(self.sess, self.weights[0])
                # self.sess.BackwardGraph(temp)

                a = self.feed_forward(X)

                delta = self.calculate_loss(a[-1], Y)

                self.backprop(delta)





            # temp = self.weights[0].data().Copy()
            # temp.Abs()

            print("Getting accuracy: ")
            print("\t getting feed forward predictions..")
            cur_pred = self.feed_forward(self.x_te)[-1]

            print("\t calculating argmaxes")
            max_pred = cur_pred.data().ArgMax(1)
            gt = self.y_te.data().ArgMax(1)

            print("DEBUG- DEBUG - DEBUG")
            print("DEBUG- DEBUG - DEBUG")

            for i in range(3):

                print("Cur Pred: ")
                for j in range(10):
                    print(cur_pred.data()[i, j])

                print("GT: ")
                for j in range(10):
                    print(self.y_te.data()[i, j])

                print("Cur Pred Argmax: " + str(max_pred[i]))
                print("GT Argmax: " + str(gt[i]))


            print("DEBUG- DEBUG - DEBUG")
            print("DEBUG- DEBUG - DEBUG")

            print("\t comparing Y & Y^")
            sum_acc = 0
            for i in range(self.y_te.shape()[0]):
                sum_acc += (gt[i] == max_pred[i])
            sum_acc /= self.y_te.data().size()

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
