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