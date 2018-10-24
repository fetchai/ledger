<<<<<<< HEAD
import numpy as np
import mnist

# ACTIVATION_FN = 'relu'
ACTIVATION_FN = 'sigmoid'


def feed_forward(X, weights):
    a = [X]

    if ACTIVATION_FN == 'relu':
        for w in weights:
            temp = a[-1].dot(w)
            temp = np.maximum(temp, 0)
            a.append(temp)

    elif ACTIVATION_FN == 'sigmoid':
        for w in weights:
            temp = a[-1].dot(w)
            temp = sigmoid(temp)
            a.append(temp)

    else:
        print("activation fn not specified")
        raise ValueError()

    return a

def grads(X, Y, weights):
    grads = np.empty_like(weights)
    a = feed_forward(X, weights)
    delta = a[-1] - Y # cross-entropy
    delta *= d_sigmoid(a[2])
    grads[-1] = np.dot(a[-2].T, delta)
    # for i in range(len(delta[0])):
    #     print("delta: {}".format(delta[0][i]))
    # print("-- GAP -- ")
    # for i in range(len(delta[1])):
    #     print("delta: {}".format(delta[1][i]))
    # for i in range(len(grads[-1][0])):
    #     print("grads[1][0][i]: {}".format(grads[1][0][i]))
    # print("-- GAP -- ")
    # for i in range(len(grads[-1][1])):
    #     print("grads[1][0][i]: {}".format(grads[1][1][i]))

    if ACTIVATION_FN == 'sigmoid':
        for i in range(len(a)-2, 0, -1):
            delta = np.dot(delta, weights[i].T)
            # print("delta: {}".format(delta[0][:2]))
            delta *= d_sigmoid(a[i])
            # print("delta: {}".format(delta[0][:2]))
            grads[i-1] = np.dot(a[i-1].T, delta)
            # print("delta: {}".format(delta[0][:2]))
            # print("grads[-1]: {}".format(grads[0][0][:2]))
    elif ACTIVATION_FN == 'relu':
        for i in range(len(a)-2, 0, -1):
            delta = (a[i] > 0) * delta.dot(weights[i].T)
            grads[i-1] = a[i-1].T.dot(delta)

    else:
        raise ValueError()

    grads = grads / len(X)
    print("grads[-1]: {}".format(grads[1][0][:2]))
    # print("grads[-1]: {}".format(grads[0][0][:2]))

    return grads



def sigmoid(z):

    z = z * -1
    z = np.exp(z)
    x = 1.0 / (1.0 + z)

    return x

# sigmoid = lambda x: 1 / (1 + np.exp(-x))
d_sigmoid = lambda y: y * (1 - y)

trX, trY, teX, teY = mnist.load_data(one_hot=True, training_size=2000, validation_size=50)

h_lay_size = 10

# weights = [
#     np.random.randn(28*28, h_lay_size) / np.sqrt(28*28),
#     np.random.randn(h_lay_size, 10) / np.sqrt(h_lay_size)]
#
# load premade random initialised weights
weights = np.load('weights.npy')

# weights = [
#     np.random.randn(28*28, h_lay_size) / np.sqrt(28*28),
#     np.random.randn(10, 11) / np.sqrt(10),
#     np.random.randn(11, 10) / np.sqrt(11)]

# weights = [
#     np.random.rand(28*28, h_lay_size) / np.sqrt(28*28),
#     np.random.rand(h_lay_size, 10) / np.sqrt(h_lay_size)]
# weights = [
#     np.ones([28*28, h_lay_size]) / np.sqrt(28*28),
#     np.ones([h_lay_size, 10]) / np.sqrt(h_lay_size)]

num_epochs, batch_size, learn_rate = 30, 50, 0.2

for i in range(num_epochs):
    counter = 0
    for j in range(0, len(trX), batch_size):
        X, Y = trX[j:j+batch_size], trY[j:j+batch_size]
        weights -= learn_rate * grads(X, Y, weights)

        counter += 1
        print("counter: {}".format(counter))

    prediction = np.argmax(feed_forward(teX, weights)[-1], axis=1)
    print (i, np.mean(prediction == np.argmax(teY, axis=1)))
=======
import os
import gzip
import numpy as np

class NumpyMnist:

    def __init__(self, num_epochs = 30, batch_size = 50, learn_rate = 0.2, hidden_size=10, load_fixed_weights = True, training_size = 2000, validation_size = 50):

        self.activation_fn = 'sigmoid'
        # self.activation_fn = 'relu'

        self.data_url = 'http://yann.lecun.com/exdb/mnist/'

        self.num_epochs = num_epochs
        self.batch_size = batch_size
        self.learn_rate = learn_rate

        # set up weights
        if load_fixed_weights:
            # load premade random initialised weights - good for comparing!
            self.weights = np.load('weights.npy')
        else:
            self.h_lay_size = hidden_size
            self.weights = [
                np.random.randn(28*28, self.h_lay_size) / np.sqrt(28*28),
                np.random.randn(self.h_lay_size, 10) / np.sqrt(self.h_lay_size)]

        # load mnist data
        self.trX, self.trY, self.teX, self.teY = self.load_data(one_hot=True, training_size=training_size, validation_size=validation_size)

    def load_data(self, one_hot=True, reshape=None, training_size=10000, validation_size=50):
        x_tr = self.load_images('train-images-idx3-ubyte.gz')
        y_tr = self.load_labels('train-labels-idx1-ubyte.gz')
        x_te = self.load_images('t10k-images-idx3-ubyte.gz')
        y_te = self.load_labels('t10k-labels-idx1-ubyte.gz')

        if training_size is None:
            x_tr = x_tr[:-validation_size]
            y_tr = y_tr[:-validation_size]
        else:
            x_tr = x_tr[:training_size]
            y_tr = y_tr[:training_size]

        if validation_size is None:
            x_te = x_te[:validation_size]
            y_te = y_te[:validation_size]
        else:
            x_te = x_te[:validation_size]
            y_te = y_te[:validation_size]

        if one_hot:
            y_tr, y_te = [self.to_one_hot(y) for y in (y_tr, y_te)]

        if reshape:
            x_tr, x_te = [x.reshape(*reshape) for x in (x_tr, x_te)]

        return x_tr, y_tr, x_te, y_te

    def load_images(self, filename):
        self.maybe_download(filename)
        with gzip.open(filename, 'rb') as f:
            data = np.frombuffer(f.read(), np.uint8, offset=16)
        return data.reshape(-1, 28 * 28) / np.float32(256)

    def load_labels(self, filename):
        self.maybe_download(filename)
        with gzip.open(filename, 'rb') as f:
            data = np.frombuffer(f.read(), np.uint8, offset=8)
        return data

    # Download the file, unless it's already here.
    def maybe_download(self, filename):
        if not os.path.exists(filename):
            from urllib.request import urlretrieve
            print("Downloading %s" % filename)
            urlretrieve(self.data_url + filename, filename)

    # Convert class labels from scalars to one-hot vectors.
    def to_one_hot(self, labels, num_classes=10):
        return np.eye(num_classes)[labels]

    def feed_forward(self, X, weights):
        a = [X]

        if self.activation_fn == 'relu':
            for w in weights:
                temp = a[-1].dot(w)
                temp = np.maximum(temp, 0)
                a.append(temp)

        elif self.activation_fn == 'sigmoid':
            for w in weights:
                temp = a[-1].dot(w)
                temp = self.sigmoid(temp)
                a.append(temp)

        else:
            print("activation fn not specified")
            raise ValueError()

        return a

    def grads(self, X, Y, weights):
        grads = np.empty_like(weights)
        a = self.feed_forward(X, weights)
        print("output: " + str(a[-1][0]))
        delta = a[-1] - Y # cross-entropy
        delta *= self.d_sigmoid(a[2])
        grads[-1] = np.dot(a[-2].T, delta)

        if self.activation_fn == 'sigmoid':
            for i in range(len(a)-2, 0, -1):
                delta = np.dot(delta, weights[i].T)
                delta *= self.d_sigmoid(a[i])
                grads[i-1] = np.dot(a[i-1].T, delta)
        elif self.activation_fn == 'relu':
            for i in range(len(a)-2, 0, -1):
                delta = (a[i] > 0) * delta.dot(weights[i].T)
                grads[i-1] = a[i-1].T.dot(delta)

        else:
            raise ValueError()

        grads = grads / len(X)

        return grads

    def sigmoid(self, z):

        z = z * -1
        z = np.exp(z)
        x = 1.0 / (1.0 + z)

        return x

    def d_sigmoid(self, y):
        return y * (1 - y)

    def train(self, train_epochs = None):

        if not train_epochs:
            train_epochs = self.num_epochs


        for i in range(train_epochs):
            for j in range(0, len(self.trX), self.batch_size):
                X, Y = self.trX[j:j+self.batch_size], self.trY[j:j+self.batch_size]
                self.weights -= self.learn_rate * self.grads(X, Y, self.weights)


            prediction = np.argmax(self.feed_forward(self.teX, self.weights)[-1], axis=1)
            print (i, np.mean(prediction == np.argmax(self.teY, axis=1)))

np_mnist = NumpyMnist()
np_mnist.train()
>>>>>>> 0d9f5a8842a1e8f8cd1cf8fc23164d04ddd5a87f
