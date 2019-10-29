import os
import gzip
from fetch.math import NDArrayDouble
from tqdm import tqdm


import random
import numpy as np


DATA_URL = 'http://yann.lecun.com/exdb/mnist/'


def load_data(one_hot=True, reshape=None, training_size=50000,
              validation_size=10000):
    print("loading training image data")
    x_tr = load_images('train-images-idx3-ubyte.gz', training_size)
    print("loading training labels")
    y_tr = load_labels('train-labels-idx1-ubyte.gz', training_size)

    print("loading test image data")
    x_te = load_images('t10k-images-idx3-ubyte.gz', validation_size)
    print("loading test labels")
    y_te = load_labels('t10k-labels-idx1-ubyte.gz', validation_size)

    if one_hot:
        y_tr_onehot = NDArrayDouble.Zeros([y_tr.shape()[0], 10])
        y_te_onehot = NDArrayDouble.Zeros([y_te.shape()[0], 10])

        for i in range(len(y_tr)):
            y_tr_onehot[i, int(y_tr[i])] = 1
        for i in range(len(y_te)):
            y_te_onehot[i, int(y_te[i])] = 1

    if reshape:
        x_tr, x_te = [x.reshape(*reshape) for x in (x_tr, x_te)]

    return x_tr, y_tr_onehot, x_te, y_te_onehot


def load_images(filename, data_size):
    download(filename)
    with gzip.open(filename, 'rb') as f:
        data = np.frombuffer(f.read(), np.uint8, offset=16)
        data = data.reshape(-1, 28 * 28) / 256
        nd_data = NDArrayDouble([data_size, 28 * 28])
        nd_data.FromNumpy(data[:data_size, :])
    return nd_data


def load_labels(filename, data_size):
    download(filename)
    with gzip.open(filename, 'rb') as f:
        data = np.frombuffer(f.read(), np.uint8, offset=8)
        nd_data = NDArrayDouble(data_size)
        nd_data.FromNumpy(data[:data_size])
    return nd_data


def download(filename):
    if not os.path.exists(filename):
        from urllib.request import urlretrieve
        print("Downloading %s" % filename)
        urlretrieve(DATA_URL + filename, filename)


def sigmoid(x):
    z = x * -1.0
    z.Exp()
    return NDArrayDouble.__truediv__(float(1.0), (z + 1))


def d_sigmoid(y):
    for i in range(y.size()):
        y[i] = y[i] * (1.0 - y[i])
    # y = y * (1 - y)
    return y


# feed forward pass of a network
# take X input and the network (defined as a list of weights)
# no biases?
def feed_forward(X, weights):
    a = [X]
    for w in weights:
        temp = a[-1].dot(w)
        temp2 = sigmoid(temp)
        a.append(temp2)
    return a


def grads(X, Y, weights):
    """get the gradients of the network"""

    # define container
    grads = []
    for i in range(len(weights)):
        grads.append(NDArrayDouble(weights[i].shape()))
    # grads = np.empty_like(weights)

    # run a forward pass to get delta
    a = feed_forward(X, weights)
    delta = a[-1] - Y  # cross-entropy

    # calculate grads
    grads[-1] = NDArrayDouble.dot((a[-2].Copy()).transpose([1, 0]), delta)

    # grads[-1] = np.dot(a[-2].transpose([1, 0]), delta)
    for i in range(len(a) - 2, 0, -1):
        delta = NDArrayDouble.dot(
            delta, (weights[i].Copy()).transpose([1, 0])) * d_sigmoid(a[i])
        # delta = np.dot(delta, weights[i].transpose([1, 0])) * d_sigmoid(a[i])
        grads[i - 1] = NDArrayDouble.dot((a[i - 1].Copy()
                                          ).transpose([1, 0]), delta)
        # grads[i-1] = np.dot(a[i-1].transpose([1, 0]), delta)

    # divide grads by batch size
    for i in range(len(grads)):
        grads[i] /= len(X)

    return grads


def make_new_layer(in_size, out_size):
    layer = NDArrayDouble([in_size, out_size])

    for i in range(layer.size()):
        layer[i] = random.uniform(-0.1, 0.1) / np.sqrt(out_size)

    return layer


def get_initial_weights(net):
    weights = [make_new_layer(28 * 28, net[0])]
    if len(net) > 2:
        for i in range(len(net) - 2):
            weights.append(make_new_layer(net[i], net[i + 1]))
    weights.append(make_new_layer(net[-2], net[-1]))

    return weights


def get_accuracy(cur_epoch, cur_pred, Y_te):
    sum_acc = 0
    for k in range(batch_size):
        unfinished = True
        counter = 0
        while (unfinished):
            if counter >= 10:
                unfinished = False
            else:
                if ((cur_pred[k, counter] == 1) and (Y_te[k, counter] == 1)):
                    sum_acc += 1
                    unfinished = False
                counter += 1

    mean_acc = sum_acc / batch_size
    print("epoch: ", cur_epoch, "acc: ", mean_acc)
    return


def train(n_epochs, batch_size, alpha, weights, X_tr, Y_tr, X_te, Y_te):
    # epochs
    for i in range(n_epochs):

        # training batches
        for j in tqdm(range(0, X_tr.shape()[0] - batch_size, batch_size)):

            # get current batch
            X, Y = X_tr[j:j + batch_size, :], Y_tr[j:j + batch_size, :]

            # update weights
            temp_grads = grads(X, Y, weights)
            for i in range(len(weights)):
                weights[i] -= (temp_grads[i] * alpha)

        # get current prediction
        cur_pred = feed_forward(X_te, weights)[-1]
        pred_argmax = NDArrayDouble.Zeros([batch_size, 10])
        for j in range(batch_size):
            argmaxval = -1
            curmax = 0
            for k in range(10):
                if cur_pred[j, k] > curmax:
                    curmax = cur_pred[j, k]
                    argmaxval = k
            pred_argmax[j, argmaxval] = 1

        # print accuracy
        get_accuracy(i, pred_argmax, Y_te)

    return


# load the data
X_tr, Y_tr, X_te, Y_te = load_data(
    one_hot=True, reshape=None, training_size=100, validation_size=10000)
# X_tr, Y_tr, X_te, Y_te = load_data(one_hot=True, reshape=None, training_size=50000, validation_size=10000)

# initialise network
outputs = 10
network_architecture = [100, outputs]
weights = get_initial_weights(network_architecture)

# training constants
num_epochs = 30
batch_size = 10
alpha = 0.2

train(num_epochs, batch_size, alpha, weights, X_tr, Y_tr, X_te, Y_te)
