# coding: utf-8

import os                                           # checking for already existing files
import gzip                                         # downloading/extracting mnist data
import random                                       # normal distirbution sampling
from tqdm import tqdm                               # visualising progress
import numpy as np                                  # loading data from buffer
import sys

# from fetch.math.linalg import MatrixDouble          # our matrices
from fetch.math.linalg import MatrixDouble
from fetch.ml import Layer, Variable, Session
from fetch.ml import CrossEntropyLoss
# from fetch.ml.layers import Layer          # our matrices

# TODO: Validation size must be same as batch size for the moment

class MnistLearner():

    def __init__(self):

        self.data_url = 'http://yann.lecun.com/exdb/mnist/'

        self.x_tr_filename = 'train-images-idx3-ubyte.gz'
        self.y_tr_filename = 'train-labels-idx1-ubyte.gz'
        self.x_te_filename = 't10k-images-idx3-ubyte.gz'
        self.y_te_filename = 't10k-labels-idx1-ubyte.gz'


        self.training_size = 1000
        self.validation_size = 10

        self.n_epochs = 30
        self.batch_size = 10
        self.alpha = 0.2

        self.mnist_input_size = 784         # pixels in 28 * 28 mnist images
        self.mnist_output_size = 10         # 10 possible characters to recognise

        self.activation_fn = 'relu'
        self.layers = []
        self.net = [20, 10]
        self.sess = None
        self.initialise_network()

        self.X_batch = self.sess.Variable([self.batch_size, 784], "X_batch")
        self.Y_batch = self.sess.Variable([self.batch_size, 10], "Y_batch")




    def initialise_network(self):

        self.sess = Session()

        # definition of the network layers
        # for i in range(len(self.layers)):
        #     self.net.append(self.layers[i])
        self.net.append(self.mnist_output_size)

        self.layers.append(self.sess.Layer(self.mnist_input_size, self.net[0], "input_layer"))
        if len(self.net) > 2:
            for i in range(len(self.net) - 2):
                self.layers.append(self.sess.Layer(self.net[i], self.net[i + 1], "layer_" + str(i+1)))
        self.layers.append(self.sess.Layer(self.net[-1], self.mnist_output_size, "output_layer"))

        self.y_pred = self.layers[-1].Output()

        return

    def load_data(self, one_hot=True, reshape=None):
        x_tr = self.load_images(self.x_tr_filename, self.training_size, "X_train")
        y_tr = self.load_labels(self.y_tr_filename, self.training_size, "Y_train")
        x_te = self.load_images(self.x_te_filename, self.validation_size, "X_test")
        y_te = self.load_labels(self.y_te_filename, self.validation_size, "Y_test")

        if one_hot:
            y_tr_onehot = Session.Zeroes(self.sess, [y_tr.size(), self.mnist_output_size])
            y_te_onehot = Session.Zeroes(self.sess, [y_te.size(), self.mnist_output_size])

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

    def load_images(self, filename, data_size, name):
        self.download(filename)
        with gzip.open(filename, 'rb') as f:
            data = np.frombuffer(f.read(), np.uint8, offset=16)
            data = data.reshape(-1, 28 * 28) / 256
            nd_data = self.sess.Variable([data_size, 784], name)
            nd_data.FromNumpy(data[:data_size, :])

        return nd_data

    def load_labels(self, filename, data_size, name):
        self.download(filename)
        with gzip.open(filename, 'rb') as f:
            data = np.frombuffer(f.read(), np.uint8, offset=8)
            data.reshape(np.shape(data)[0], -1)

            nd_data = self.sess.Variable([data_size, 1], name)
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

    def assign_batch(self, cur_rep):
        # assign X batch
        for k in range(self.batch_size):
            for l in range(28 * 28):
                self.X_batch[k, l] = self.x_tr[cur_rep + k, l]

        # assign Y batch
        for k in range(self.batch_size):
            for l in range(10):
                self.Y_batch[k, l] = self.y_tr[cur_rep + k, l]
        return

    def calculate_loss(self, X, Y):

        loss = CrossEntropyLoss(X, Y, self.sess)
        return loss

    def train(self):

        # TODO: Check if we need to re-call setinput for  each batch
        self.sess.SetInput(self.layers[0], self.X_batch)
        for i in range(len(self.layers) - 1):
            self.sess.SetInput(self.layers[i + 1], self.layers[i].Output())

        # epochs
        for i in range(self.n_epochs):
            print("epoch ", i, ": ")

            # training batches
            for j in tqdm(range(0, self.x_tr.shape()[0] - self.batch_size, self.batch_size)):

                # assign fresh data batch
                self.assign_batch(j)

                # loss calculation
                loss = self.calculate_loss(self.layers[-1].Output(), self.Y_batch)

                # back propagate
                self.sess.BackProp(self.X_batch, loss, self.alpha, 1)

                print("\nCEL data: ")
                for i in range(self.validation_size):
                    print("\n")
                    for j in range(1):
                        sys.stdout.write('{:0.13f}'.format(loss.data()[i, j]) + "\t")
                print("\nCEL grad: ")
                for i in range(self.validation_size):
                    print("\n")
                    for j in range(10):
                        sys.stdout.write('{:0.13f}'.format(loss.Grads()[i, j]) + "\t")

                print("\ndot_output: ")
                for i in range(self.validation_size):
                    print("\n")
                    for j in range(10):
                        sys.stdout.write('{:0.13f}'.format(self.layers[0].DotOutput().data()[i, j]) + "\t")

                print("\npredictions: ")
                print("\n")
                for i in range(200):
                    sys.stdout.write('{:0.13f}'.format(self.layers[0].Output().data()[i]) + "\n")



            print("Getting accuracy: ")
            print("\t getting feed forward predictions..")
            # cur_pred = self.sess.Predict(self.x_te, self.y_pred)
            cur_pred = self.sess.Predict(self.x_te, self.layers[-1].Output())


            print("\nweights: ")
            for i in range(self.validation_size):
                print("\n")
                for j in range(10):
                    sys.stdout.write('{:0.13f}'.format(self.layers[-1].weights().data()[i, j]) + "\t")

            print("\nweight grads: ")
            for i in range(self.validation_size):
                print("\n")
                for j in range(10):
                    sys.stdout.write('{:0.13f}'.format(self.layers[-1].weights().Grads()[i, j]) + "\t")

            print("\ndot_output: ")
            for i in range(self.validation_size):
                print("\n")
                for j in range(10):
                    sys.stdout.write('{:0.13f}'.format(self.layers[-1].DotOutput().data()[i, j]) + "\t")

            print("\ndot_output_grads: ")
            for i in range(self.validation_size):
                print("\n")
                for j in range(10):
                    sys.stdout.write('{:0.13f}'.format(self.layers[-1].DotOutput().Grads()[i, j]) + "\t")

            print("\noutput: ")
            for i in range(self.validation_size):
                print("\n")
                for j in range(10):
                    sys.stdout.write('{:0.13f}'.format(self.layers[-1].Output().data()[i, j]) + "\t")

            print("\noutput_grads: ")
            for i in range(self.validation_size):
                print("\n")
                for j in range(10):
                    sys.stdout.write('{:0.13f}'.format(self.layers[-1].Output().Grads()[i, j]) + "\t")

            print("\t calculating argmaxes")
            max_pred = cur_pred.ArgMax(1)
            gt = self.y_te.data().ArgMax(1)

            print("DEBUG- DEBUG - DEBUG")
            print("DEBUG- DEBUG - DEBUG")

            print("\npredictions: ")
            for i in range(self.validation_size):
                print("\n")
                for j in range(10):
                    sys.stdout.write('{:0.13f}'.format(self.layers[-1].Output().data()[i, j]) + "\t")

            print("\nweights: ")
            for i in range(self.validation_size):
                print("\n")
                for j in range(10):
                    sys.stdout.write('{:0.13f}'.format(self.layers[-1].weights().data()[i, j]) + "\t")

            print("\nweights grads: ")
            for i in range(self.validation_size):
                print("\n")
                for j in range(10):
                    sys.stdout.write('{:0.13f}'.format(self.layers[-1].weights().Grads()[i, j]) + "\t")

            for i in range(3):

                print("Cur Pred: ")
                for j in range(10):
                    print(cur_pred[i, j])

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

# run_mnist()


import numpy as np
import matplotlib.pyplot as plt

# N is batch size(sample size); D_in is input dimension;
# H is hidden dimension; D_out is output dimension.
N, D_in, H, D_out = 4, 2, 30, 1

# Create random input and output data
x = np.array([[0, 0], [0, 1], [1, 0], [1, 1]])
y = np.array([[0], [1], [1], [0]])

# Randomly initialize weights
w1 = np.random.randn(D_in, H)
w2 = np.random.randn(H, D_out)

learning_rate = 0.002
loss_col = []
for t in range(200):
    # Forward pass: compute predicted y
    h = x.dot(w1)
    h_relu = np.maximum(h, 0)  # using ReLU as activate function
    y_pred = h_relu.dot(w2)

    # Compute and print loss
    loss = np.square(y_pred - y).sum() # loss function
    loss_col.append(loss)
    print(t, loss, y_pred)

    # Backprop to compute gradients of w1 and w2 with respect to loss
    grad_y_pred = 2.0 * (y_pred - y) # the last layer's error
    grad_w2 = h_relu.T.dot(grad_y_pred)
    grad_h_relu = grad_y_pred.dot(w2.T) # the second laye's error
    grad_h = grad_h_relu.copy()
    grad_h[h < 0] = 0  # the derivate of ReLU
    grad_w1 = x.T.dot(grad_h)

    # Update weights
    w1 -= learning_rate * grad_w1
    w2 -= learning_rate * grad_w2

plt.plot(loss_col)
plt.show()